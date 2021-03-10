#ifndef TBEAM0_DATATYPES_H
#define TBEAM0_DATATYPES_H

#include <Arduino.h>

typedef struct{
  uint16_t cs; //centisecond
  uint8_t sec;
  uint8_t min;
  uint8_t hour;
  uint8_t day;
  uint8_t month;
  uint16_t year;
} DATE_TIME;

typedef struct{
    float lat;
    float lng;
    float sats;
    float alt;
    float temp;
    float batt;
    float humid;
    DATE_TIME time; 
} GPS_DATA;

typedef struct{
    float accelx;
    float accely;
    float accelz;
    float pitch;
    float roll;
    float yaw;
    DATE_TIME start;
    uint32_t dt;
} IMU_DATA;

#endif