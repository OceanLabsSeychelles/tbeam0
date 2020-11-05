#include "main.h"

long last_scan = millis();
long last_send = millis();
long last_screen = millis();
int interval = random(500);

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
        log("SSD1306 allocation failed");
    } else {
        log("OLED Ok.");
    }

    GPS.begin(9600, SERIAL_8N1, 34, 12); //17-TX 18-RX
    LoRa.setPins(SS, RST, DI0);
    if (!LoRa.begin(BAND)) {
        log("Starting LoRa failed!");
        while (1);
    }else{
        log("LoRa Ok.");
    }

    if (!bno.begin()) {
       log("BNO055 not detected!");
    } else {
        log("IMU Ok.");
        bno.setMode(bno.OPERATION_MODE_NDOF);
        bno.setExtCrystalUse(false);
    }


    fillHtmlMap();

    WiFiInit();
    display.setTextSize(1);      // Normal 1:1 pixel scale
    display.println(WiFi.localIP());
    display.display();
    display.setTextSize(2);

    if (!serverInit()) {
        log("Error starting server, likely SPIFFS");
    } else {
        log("Webserver Ok.");
    }
    ledcSetup(BUZZER_CHANNEL, BUZZER_FREQ, BUZZER_RESOLUTION);
    ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
    /*
    display.clearDisplay();
    display.setTextSize(2);      // Normal 1:1 pixel scale
    display.setCursor(0, 0);     // Start at top-left corner
    display.println("Pairing");
    display.display();
    while ((!ready) && (!buddy_ready)) {
        if (millis() - last_send > (interval)) {
            buddy_tx_handler.service();
            interval = random(500);
            last_send = millis();
        }
        budd_rx_handler.service();
        }
    }
    display.clearDisplay();
    display.setCursor(0, 0);     // Start at top-left corner
    display.println("START!");
    display.display();
    delay(2000);
    server.end();
    btStop();
     */

}


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
    imu_handler.service();
    imu_cal_handler.service();
    //buzzer_handler.service();
}