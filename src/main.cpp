#include "main.h"

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */

const bool is_bouy = true;
const int sleep_time = 30; //3300 seconds = 10 minutes
const int post_delay = 10;


void setup() {
    pinMode(BATTERY_PIN, INPUT);
    SPI.begin(SCK, MISO, MOSI, SS);
    Serial.begin(115200);
    Wire.begin(21, 22);
    if (!axpPowerOn()) {
        Serial.println("AXP192 Begin FAIL");
        while (1);
    } else {
        Serial.println("Power Ok.");
    }

    FlashInit();
    if (!is_bouy) {
        WiFiInit();
    }

    GPS.begin(9600, SERIAL_8N1, 34, 12); //17-TX 18-RX
    LoRa.setPins(SS, RST, DI0);
    if (is_bouy) {
        dhtSensor.setup(DHT_PIN, DHTesp::DHT11);
        pinMode(BATTERY_PIN, INPUT);
        Serial.println(getBatteryVoltage());

        dht_frame = dhtSensor.getTempAndHumidity();
        Serial.println(dht_frame.temperature);
        Serial.println(dht_frame.humidity);
        delay(1000);
    }

    if (!LoRa.begin(BAND)) {
        Serial.println("Starting LoRa failed!");
        while (1);
    } else {
        Serial.println("LoRa Ok.");
    }
    //LoRa.setTxPower(12);


    if (!bno.begin()) {
        Serial.println("BNO055 not detected!");
    } else {
        Serial.println("IMU Ok.");
        bno.setMode(bno.OPERATION_MODE_NDOF);
        bno.setExtCrystalUse(true);
    }
    //Adds current thread to watchdog timer
    if (!is_bouy) {
        //esp_task_wdt_init(WDT_TIMEOUT, true);
        //esp_task_wdt_add(NULL); 
    }
}

void loop() {
    if (is_bouy) {
        
        //Add IMU Calibration routine here
        
        Serial.print("Waiting for GPS lock...");
        while (gps.satellites.value() < 3) {
            gps.encode(GPS.read());
        }
        Serial.println("done");
        //First readings always seem a bit off, so lets read in a few longer...
        int k = 0;
        while(k<5){
            GPSUpdate();
            while(!gps.location.isUpdated()){
                 gps.encode(GPS.read());
            }
            k++;
        }
        
        int count = 0;
        start_time = getTime();
        start_millis = millis();
        while (!imu_buffer.isFull()) {
            for (int j = 0; j < 10; j++) {
                //This is updating too fast
                imu_buffer.push(IMUUpdate());
                Serial.println("Imu captured...");
                long start = millis();
                //was going to delay here, might as well read gps
                while (millis() - start < 100) {
                    gps.encode(GPS.read());
                }
                //delay(100);
            }
            
            while (!gps.location.isUpdated()) {
                gps.encode(GPS.read());
            }
            gps_buffer.push(GPSUpdate());
            Serial.println("Gps captured...");
            Serial.println(count);
            count++;
        }
        Serial.println("Done capturing data");
        axp.setPowerOutPut(AXP192_LDO3, AXP202_OFF); // GPS main power
        Serial.println("GPS powered off.");

        while (!imu_buffer.isEmpty()) {
            Serial.println("Buffering IMU packet.");
            IMU_DATA frame;
            uint8_t* frame_ptr = (uint8_t*)&frame;
            imu_buffer.lockedPop(frame);
            Serial.println("Start IMU data transfer.");
            LoRa.beginPacket();
            LoRa.write(frame_ptr, sizeof(IMU_DATA));
            Serial.println("Packet written, not completed yet");
            LoRa.endPacket();
            Serial.println("IMU Packet sent.");
            delay(post_delay);
        }

        while (!gps_buffer.isEmpty()) {
            GPS_DATA frame;
            uint8_t *frame_ptr = (uint8_t *) &frame;
            gps_buffer.lockedPop(frame);

            LoRa.beginPacket();
            LoRa.write(frame_ptr, sizeof(GPS_DATA));
            LoRa.endPacket();
            Serial.println("GPS Packet sent.");
            delay(post_delay);
        }
        
        Serial.println("Entering deep sleep.");
        axpPowerOff();
        Serial.flush();
        esp_sleep_enable_timer_wakeup(sleep_time * uS_TO_S_FACTOR);
        esp_deep_sleep_start();
    } else {
        while (true) {
            //Kick the dog every (WDT_TIMEOUT - 1) seconds
            //wdt_handler.service();
            rx_handler.service();
            if ((!imu_buffer.isEmpty() || !gps_buffer.isEmpty()) && millis() - last_rx > 5000) {
                DynamicJsonDocument gpsJson(10240);
                DynamicJsonDocument smallGpsJson(1024);
                String gpsData;

                int j = 0;
                while (!gps_buffer.isEmpty()) {
                    GPS_DATA frame;
                    gps_buffer.lockedPop(frame);
                    String key = "measurement"+String(++j);
                    gps2json(smallGpsJson, frame);
                    gpsJson[key] = (smallGpsJson);
                }
                serializeJson(gpsJson, gpsData);
                gpsPutLast(gpsData);

                DynamicJsonDocument imuJson(50000);
                DynamicJsonDocument smallImuJson(1024);
                String imuData;
                j = 0;
                while (!imu_buffer.isEmpty()) {
                    IMU_DATA frame;
                    imu_buffer.lockedPop(frame);
                    String key = "measurement"+String(++j);
                    imu2json(smallImuJson, frame);
                    imuJson[key] = smallImuJson;
                }

                serializeJson(imuJson, imuData);
                imuPutLast(imuData);
            }
        }
    }
}