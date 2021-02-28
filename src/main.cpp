#include "main.h"

long last_scan = millis();
long last_send = millis();
long last_screen = millis();
int interval = random(500);
bool imuConnected = false;

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

    if (!displayInit()) {
        Serial.println("SSD1306 allocation failed");
    } else {
        Serial.println("OLED Ok.");
    }

    GPS.begin(9600, SERIAL_8N1, 34, 12); //17-TX 18-RX
    LoRa.setPins(SS, RST, DI0);
    LoRa.enableCrc();
    LoRa.setTxPower(23);
    if (!LoRa.begin(BAND)) {
        Serial.println("Starting LoRa failed!");
        while (1);
    }else{
        Serial.println("LoRa Ok.");
    }
    imuConnected = bno.begin();
    if (!imuConnected) {
        Serial.println("BNO055 not detected!");
    } else {
        Serial.println("IMU Ok.");
        bno.setMode(bno.OPERATION_MODE_NDOF);
        bno.setExtCrystalUse(true);
    }


    fillHtmlMap();

}

long lastBeep = 0;
long duration = 50;
bool beepOn = false;

void loop() {
    while (GPS.available()) {
        gps.encode(GPS.read());
    }
    if (gps.location.isUpdated()) {
        gps_fix.lat = gps.location.lat();
        gps_fix.lng = gps.location.lng();
        gps_fix.sats = gps.satellites.value();
        HtmlVarMap["lng"] -> value = String(gps_fix.lng, GPS_SIG_FIGS);
        HtmlVarMap["lat"] -> value = String(gps_fix.lat, GPS_SIG_FIGS);
        HtmlVarMap["sats"] -> value = String(gps_fix.sats, GPS_SIG_FIGS);
        HtmlVarMap["alt"] -> value = String(gps.altitude.meters(), GPS_SIG_FIGS);
    }
    if (millis() - last_send > interval) {
        tx_handler.service();
        interval = random(500);
        last_send = millis();
    }

    rx_handler.service();
    screen_handler.service();
    if(imuConnected){
        imu_handler.service();
    }

    if(imuConnected){
        imu_cal_handler.service();
    }

}