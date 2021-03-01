#include "main.h"

long last_scan = millis();
long last_send = millis();
long last_screen = millis();
int interval = random(500);
bool is_bouy = false;
void setup() {
    SPI.begin(SCK, MISO, MOSI, SS);
    Serial.begin(115200);
    Wire.begin(21, 22);
    if (!powerInit()) {
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
    esp_task_wdt_init(WDT_TIMEOUT, true);
    esp_task_wdt_add(NULL); 
}

long lastBeep = 0;
long duration = 50;
bool beepOn = false;

void loop() {
    //Kick the dog every (WDT_TIMEOUT - 1) seconds
    wdt_handler.service();
    if (is_bouy){
        while (GPS.available()) {
            gps.encode(GPS.read());
        }
        if (gps.location.isUpdated()) {
            gps_fix.lat = gps.location.lat();
            gps_fix.lng = gps.location.lng();
            gps_fix.sats = gps.satellites.value();
            gps_fix.alt = gps.altitude.meters();
        }
        if (millis() - last_send > interval) {
            tx_handler.service();
            interval = random(500);
            Serial.println("Packet sent.");
            last_send = millis();
        }

        imu_handler.service();
        imu_cal_handler.service();
    }else{
        rx_handler.service();
    }
}