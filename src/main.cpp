#include "main.h"

void buoyHandler();
void relayHandler();

long last_scan = millis();
long last_send = millis();
long last_screen = millis();
int interval = random(500);
const bool is_bouy = false;
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

    //Adds current thread to watchdog timer
    if(!is_bouy){
        esp_task_wdt_init(WDT_TIMEOUT, true);
        esp_task_wdt_add(NULL); 
    }
    
}

const int measure_time = 300; //300 seconds = 5 minutes
const int sleep_time = 3300; //3300 seconds = 55 minutes

void loop() {
    if (is_bouy){
        while (true){
            axpPowerOn();
            /*
                These variables need to be assigned at the start of 
                every IMU burst reading
                long start_millis, DATE_TIME start_time
                need to wait for valid gps lock...
            */
            while(gps.satellites.value() < 5){
                gps.encode(GPS.read());
            }
            start_millis = millis();
            start_time = getTime();

            while((millis()-start_millis)<measure_time*1000){
                while (GPS.available()) {
                    gps.encode(GPS.read());
                }
                /*
                In addition, this does not transmit battery voltage or temperature...
                */
                if (gps.location.isUpdated()) {
                    GPSUpdate();
                }
                
                imu_handler.service();
                imu_cal_handler.service();
            }
            axpPowerOff();
        }
    }else{
        while (true){
        //Kick the dog every (WDT_TIMEOUT - 1) seconds
        wdt_handler.service();
        rx_handler.service();
        }
    }
}