#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include <Arduino.h>

typedef struct{
  uint16_t ms;
  uint8_t sec;
  uint8_t min;
  uint8_t hour;
  uint8_t day;
  uint8_t month;
  uint8_t year;
} DATE_TIME;

typedef struct{
    float lat;
    float lng;
    float sats;
    float alt;
    float temp;
    float batt;
    DATE_TIME time;
} GPS_DATA;

typedef struct{
    float accelx;
    float accely;
    float accelz;
    float pitch;
    float roll;
    float yaw;
} IMU_DATA;

#endif