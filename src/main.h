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
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include "webserver.h"
#include "geometry.h"
#include "math.h"
#include <FunctionTimer.h>

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
#define BUZZER_RESOLUTION 10
#define BUZZER_BITS 1024
#define BUZZER_FREQ 2700
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define BAND  433E6
#define GPS_SIG_FIGS 7

TinyGPSPlus gps;
HardwareSerial GPS(1);
AXP20X_Class axp;
Adafruit_BNO055 bno = Adafruit_BNO055(-1, 0x28);
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
        HtmlVarMap["bd-bearing"]-> value = String(getBearing(gps_fix.lat, gps_fix.lng, partner_fix.info.lat, partner_fix.info.lng),4);

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

void GPSScreenUpdate(){
    display.setCursor(0, 0); // Start at top-left corner
    display.clearDisplay();

    display.print("Sats:");
    display.print(gps_fix.sats);
    display.print(",");
    display.println(partner_fix.info.sats);

    display.print("dT(s):");
    float dt = millis() / 1000.0 - partner_fix.last_time;
    HtmlVarMap["bdTxTime"]->value = dt;
    display.println(dt, 1);

    if (partner_fix.info.sats > 2) {
        display.print("D(m):");
        display.println(distance, 1);
    }

    display.display();
}
void IMUScreenUpdate(){
    display.setCursor(0, 0); // Start at top-left corner
    display.clearDisplay();

    display.print("MAG:");
    display.print(HtmlVarMap["mag-cal"]->value);
    display.print(",");
    display.print(HtmlVarMap["accel-cal"]->value);
    display.print(",");
    display.println(HtmlVarMap["gyro-cal"]->value);

    display.print("Yaw:");
    float yaw = HtmlVarMap["imu-heading"]-> value.toFloat();
    display.println(yaw);

    display.print("Bdy:");
    float bearing = HtmlVarMap["bd-bearing"]-> value.toFloat();
    display.println(bearing);

    display.print("Dif: ");
    float difference = yaw-bearing;
    display.println(difference);
}
void ScreenUpdate(){
    static int last;
    static bool gps = false;
    if(gps){
        GPSScreenUpdate();
    }else{
        IMUScreenUpdate();
    }
    if((millis() - last) > 5000){
        gps = !gps;
        last = millis();
    }

}
FunctionTimer screen_handler(&ScreenUpdate, 100);

void IMUCal(){
    uint8_t system, gyro, accel, mag;
    system = gyro = accel = mag = 0;
    bno.getCalibration(&system, &gyro, &accel, &mag);

    HtmlVarMap["mag-cal"]->value = String(mag);
    HtmlVarMap["accel-cal"]->value = String(accel);
    HtmlVarMap["gyro-cal"]->value = String(gyro);
}
FunctionTimer imu_cal_handler(&IMUCal, 500);


void IMUUpdate(){

    sensors_event_t event;
    bno.getEvent(&event);

    HtmlVarMap["imu-heading"]-> value = String(event.orientation.x, 2);
    HtmlVarMap["imu-pitch"]-> value = String(event.orientation.z, 2);
    HtmlVarMap["imu-roll"]-> value = String(event.orientation.y, 2);

}
FunctionTimer imu_handler(&IMUUpdate, 100);

void BuzzerTone(){
    int cal = HtmlVarMap["mag-cal"] -> value.toInt();
    if (cal > 0){

        float heading = HtmlVarMap["imu-heading"]->value.toFloat();
        float headingRad = deg2rad(heading);
        int freq = int(1024*(sin(headingRad)));
        ledcWriteTone(BUZZER_CHANNEL,  2700+freq);
        ledcWrite(BUZZER_CHANNEL, 35);

        /*
        int dutyCycle = int(256*(sin(headingRad)+1));
        ledcWrite(BUZZER_CHANNEL, dutyCycle);
        Serial.print(headingRad);
        Serial.print(",");
        Serial.println(dutyCycle);
         */
        /*
        float error = abs(heading-180);
        int dutyCycle =  1024 - int(error*2.844);
        Serial.println(dutyCycle);
        ledcWrite(BUZZER_CHANNEL, dutyCycle);
        */
    }
}
FunctionTimer buzzer_handler(&BuzzerTone, 30);


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