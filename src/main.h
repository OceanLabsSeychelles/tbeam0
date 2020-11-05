#ifndef TTBEAM0_MAIN_H
#define TTBEAM0_MAIN_H

#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <TinyGPS++.h>
#include <axp20x.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Tone32.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include "webserver.h"
#include "geometry.h"
#include "math.h"
#include <../lib/FunctionTimer/FunctionTimer.h>

#define OLED_RESET     16 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCK     5    // GPIO5  -- SX1278's SCK
#define MISO    19   // GPIO19 -- SX1278's MISnO
#define MOSI    27   // GPIO27 -- SX1278's MOSI
#define SS      18   // GPIO18 -- SX1278's CS
#define RST     14   // GPIO14 -- SX1278's RESET
#define DI0     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)
#define BATTERY_PIN 35 // battery level measurement pin, here is the voltage divider connected
#define BUZZER_PIN  13
#define BUZZER_CHANNEL 0
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define BAND  433E6
#define GPS_SIG_FIGS 7

TinyGPSPlus gps;
HardwareSerial GPS(1);
AXP20X_Class axp;
Adafruit_BNO055 bno = Adafruit_BNO055(-1, 0x29);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

typedef struct{
    double lat;
    double lng;
    int sats;
} GPS_DATA;

typedef struct{
    GPS_DATA info;
    double last_time;
} PARTNER_DATA;

typedef struct{
    char id[40];
    int distmax;
    int downmax;
    int buddylock;
    bool ready;
} PAIRING_DATA;

GPS_DATA gps_fix;
uint8_t* gps_fix_ptr = (uint8_t*)&gps_fix;
PARTNER_DATA partner_fix;
PAIRING_DATA local_config;
uint8_t* local_config_ptr = (uint8_t*)&local_config;
PAIRING_DATA partner_config;

volatile bool server_on = true;
volatile bool lora_scan = false;
volatile bool buddy_found = false;
volatile bool ready = false;
volatile bool buddy_ready = false;
volatile int distmax;
volatile int downmax;
volatile int buddylock;
volatile int divedist;
volatile int divedown;
volatile int divelock;

bool displayInit(){
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
    return false;

  }else{
    display.clearDisplay();
    display.setTextSize(2);      // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.cp437(true);         // Use full 256 char 'Code Page 437' font
    display.setCursor(0, 0);     // Start at top-left corner
    display.println("Dive-Alive");
    display.display();
    return true;
  }
};

bool powerInit(){
  if (!axp.begin(Wire, AXP192_SLAVE_ADDRESS)) {
    axp.setPowerOutPut(AXP192_LDO2, AXP202_ON); // LORA radio
    axp.setPowerOutPut(AXP192_LDO3, AXP202_ON); // GPS main power
    axp.setPowerOutPut(AXP192_DCDC2, AXP202_ON);
    axp.setPowerOutPut(AXP192_EXTEN, AXP202_ON);
    axp.setPowerOutPut(AXP192_DCDC1, AXP202_ON); //OLED
    axp.setDCDC1Voltage(3300); // for the OLED power
    return true;
  } else {   
    return false;
  }
}


void LoRaScan(){
    int packetSize = LoRa.parsePacket();
    if (packetSize) {

        uint8_t packet[packetSize];
        for (int j = 0; j < packetSize; j++) {
            packet[j] = LoRa.read();
        }

        memcpy( & partner_fix.info, packet, sizeof(GPS_DATA));
        HtmlVarMap["bd-lng"] -> value = String(partner_fix.info.lng, GPS_SIG_FIGS);
        HtmlVarMap["bd-lat"] -> value = String(partner_fix.info.lat, GPS_SIG_FIGS);
        HtmlVarMap["bd-sats"] -> value = String(partner_fix.info.sats, GPS_SIG_FIGS);
        partner_fix.last_time = millis() / 1000.0;
        distance = distanceEarth(gps_fix.lat, gps_fix.lng, partner_fix.info.lat, partner_fix.info.lng);
        bearing = getBearing(gps_fix.lat, gps_fix.lng, partner_fix.info.lat, partner_fix.info.lng);
        //rssi = "RSSI " + String(LoRa.packetRssi(), DEC) ;
        //Serial.println(rssi);
    }
}
FunctionTimer rx_handler(& LoRaScan, 20);

void LoRaSend(){
    LoRa.beginPacket();
    LoRa.write(gps_fix_ptr, sizeof(GPS_DATA));
    LoRa.endPacket();
}
FunctionTimer tx_handler(& LoRaSend, 1000);

void ScreenUpdate(){
    display.setCursor(0, 0); // Start at top-left corner
    display.clearDisplay();

    display.print("Sats:");
    display.print(gps_fix.sats);
    display.print(",");
    display.println(partner_fix.info.sats);

    display.print("dT(s):");
    display.println(millis() / 1000.0 - partner_fix.last_time, 1);

    if (partner_fix.info.sats > 2) {
        display.print("D(m):");
        display.println(distance, 1);
    }else{
        display.print("Accel:");
        display.println(HtmlVarMap["lin-accel"]->value);
    }

    display.display();
}
FunctionTimer screen_handler(& ScreenUpdate, 500);

void IMUUpdate(){

    imu::Vector<3> event = bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL );
    float x,y,z,mag;
    x = event.x();
    y = event.y();
    z = event.z();

    mag = sqrt(x*x + y*y + z*z);
    HtmlVarMap["lin-accel"]-> value = mag;
}
FunctionTimer imu_handler(&IMUUpdate, 10);

void BuddyTX(){
    local_config.distmax = HtmlVarMap["distmax"]->value.toInt();
    local_config.downmax = HtmlVarMap["downmax"]->value.toInt();
    local_config.buddylock = HtmlVarMap["buddylock"]->value.toInt();
    local_config.ready = ready;
    strcpy(local_config.id, "zero2spearo");

    LoRa.beginPacket();
    LoRa.write(local_config_ptr, sizeof(PAIRING_DATA));
    LoRa.endPacket();
}
FunctionTimer buddy_tx_handler(&BuddyTX, 1000);

void BuddyRX() {
    int packetSize = LoRa.parsePacket();
    if (packetSize) {

        uint8_t packet[packetSize];
        for (int j = 0; j < packetSize; j++) {
            packet[j] = LoRa.read();
        }
        memcpy(&partner_config, packet, sizeof(PAIRING_DATA));
        //buddy_found = true;
        display.clearDisplay();
        display.setCursor(0, 0);     // Start at top-left corner
        display.println(partner_config.ready);
        display.println(partner_config.distmax);
        display.println(partner_config.downmax);
        //divedist = getmin(distmax, partner_config.distmax);
        divedown = partner_config.downmax;
        divelock = buddylock || partner_config.buddylock;
        display.display();
    }
}
FunctionTimer buddy_rx_handler(&BuddyRX, 20);

#endif