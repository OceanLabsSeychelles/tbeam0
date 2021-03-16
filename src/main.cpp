#include "main.h"
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */

const bool is_bouy = true;
const int sleep_time = 30; //3300 seconds = 10 minutes
const int post_delay = 50;
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 0;


void setup() {

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
    if(!is_bouy){
        WiFiInit();
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    }

    GPS.begin(9600, SERIAL_8N1, 34, 12); //17-TX 18-RX
    LoRa.setPins(SS, RST, DI0);
    if (!LoRa.begin(BAND)) {
        Serial.println("Starting LoRa failed!");
        while (1);
    }else{
        Serial.println("LoRa Ok.");
    }
    if (!bno.begin()) {
        Serial.println("BNO055 not detected!");
    } else {
        Serial.println("IMU Ok.");
        bno.setMode(bno.OPERATION_MODE_NDOF);
        bno.setExtCrystalUse(true);
    }
    pinMode(BATTERY_PIN, INPUT);

    //Adds current thread to watchdog timer
    if(!is_bouy){
        //esp_task_wdt_init(WDT_TIMEOUT, true);
        //esp_task_wdt_add(NULL); 
    }
}

void loop() {
    if (is_bouy){
        //Add IMU Calibration routine here
        while(gps.satellites.value() < 3){
            gps.encode(GPS.read());
            Serial.println(gps.satellites.value());
        }
        int k = 0;
        while(k<5){
            GPSUpdate();
            while(!gps.location.isUpdated()){
                 gps.encode(GPS.read());
            }
            k++;
        }
        int sample_count = 0;
        int count = 0;
        while(sample_count<NUM_PACKETS){
            while(!imu_buffer.isFull()){
                start_time = getTime();
                for(int j = 0; j< 10; j++){
                    imu_buffer.lockedPush(IMUUpdate());
                    Serial.println("Imu captured...");
                    int start = millis();
                    while (millis() - start < 100) {
                        gps.encode(GPS.read());
                    }
                    //delay(100);
                }
                while(!gps.location.isUpdated()&& gps.satellites.value()<3){
                    gps.encode(GPS.read());
                }
                GPS_DATA pushFrame;
                pushFrame = GPSUpdate();
                Serial.printf("Sats: %d  Batt:%f  Year:%d\n", pushFrame.sats, pushFrame.batt, pushFrame.time.year);
                gps_buffer.lockedPush(pushFrame);
                Serial.println("Gps captured...");
                Serial.println(count);
                count ++;
            }

            //Serial.println("Done capturing data");
            //axp.setPowerOutPut(AXP192_LDO3, AXP202_OFF); // GPS main power
            //Serial.println("GPS powered off.");
            long tx_start = millis();
            while(!imu_buffer.isEmpty()){
                IMU_DATA frame;
                uint8_t* frame_ptr = (uint8_t*)&frame;
                imu_buffer.lockedPop(frame);

                LoRa.beginPacket();
                LoRa.write(frame_ptr, sizeof(IMU_DATA));
                LoRa.endPacket();
                Serial.println("IMU Packet sent.");
                delay(post_delay);
            }

            while(!gps_buffer.isEmpty()){
                GPS_DATA frame;
                uint8_t* frame_ptr = (uint8_t*)&frame;
                gps_buffer.lockedPop(frame);

                LoRa.beginPacket();
                LoRa.write(frame_ptr, sizeof(GPS_DATA));
                LoRa.endPacket();
                Serial.println("GPS Packet sent.");
                delay(post_delay);
            }
            Serial.printf("Packet %d sent in %d millis.\n",count, int(millis()-tx_start));
            sample_count++;
        }

        Serial.println("Entering deep sleep.");
        axpPowerOff();
        Serial.flush();
        esp_sleep_enable_timer_wakeup(sleep_time * uS_TO_S_FACTOR);
        esp_deep_sleep_start();

    }else{
        while (true){
            //Kick the dog every (WDT_TIMEOUT - 1) seconds
            //wdt_handler.service();
            rx_handler.service();
            if ((!imu_buffer.isEmpty() || !gps_buffer.isEmpty()) && millis() - last_rx > 5000) {
                DynamicJsonDocument gpsJson(JSON_DOC_SIZE*GPS_BUFFER_LEN);
                DynamicJsonDocument smallGpsJson(JSON_DOC_SIZE);
                String gpsData;

                int j = 0;
                while (!gps_buffer.isEmpty()) {
                    GPS_DATA frame;
                    gps_buffer.lockedPop(frame);
                    String key = "measurement"+String(++j);
                    gps2json(smallGpsJson, frame);
                    gpsJson[key] = (smallGpsJson);
                    Serial.println(frame.sats);
                    Serial.println(frame.batt);
                    Serial.println(frame.time.year);
                    Serial.println();
                }
                serializeJson(gpsJson, gpsData);
                gpsPutLast(gpsData);
                gpsPatch(gpsData);

                DynamicJsonDocument imuJson(JSON_DOC_SIZE*IMU_BUFFER_LEN);
                DynamicJsonDocument smallImuJson(JSON_DOC_SIZE);
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
                imuPatch(imuData);
            }
        }
    }
}