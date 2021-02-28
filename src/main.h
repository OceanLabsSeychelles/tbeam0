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
    float lat;
    float lng;
    float sats;
    float alt;
    float temp;
    float batt;

} GPS_DATA;

GPS_DATA gps_fix;
uint8_t* gps_fix_ptr = (uint8_t*)&gps_fix;


typedef struct{
    float accelx;
    float accely;
    float accelz;
    float pitch;
    float roll;
    float yaw;
} IMU_DATA;

IMU_DATA imu_frame;
uint8_t* imu_frame_ptr = (uint8_t*)&imu_frame;

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

void LoRaSend(){

    LoRa.beginPacket();
    LoRa.write(gps_fix_ptr, sizeof(GPS_DATA));
    LoRa.endPacket();
}
FunctionTimer tx_handler(& LoRaSend, 1000);

void IMUCal(){
    uint8_t system, gyro, accel, mag;
    system = gyro = accel = mag = 0;
    bno.getCalibration(&system, &gyro, &accel, &mag);

    //HtmlVarMap["mag-cal"]->value = String(mag);
    //HtmlVarMap["accel-cal"]->value = String(accel);
    //HtmlVarMap["gyro-cal"]->value = String(gyro);
}
FunctionTimer imu_cal_handler(&IMUCal, 500);


void IMUUpdate(){

    sensors_event_t event;
    bno.getEvent(&event);
    imu_frame.pitch=event.orientation.z;
    imu_frame.roll=event.orientation.y;
    imu_frame.yaw=event.orientation.x;

}
FunctionTimer imu_handler(&IMUUpdate, 100);

#endif