#ifndef TTBEAM0_MAIN_H
#define TTBEAM0_MAIN_H

#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <TinyGPS++.h>
#include <axp20x.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h> //There's no reason I should have to include this...
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include <FunctionTimer.h>
#include <esp_task_wdt.h>
#include <datatypes.h>
#include <webserver.h>
#include <RingBuf.h>
#include <DHTesp.h>


#define OLED_RESET     16 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCK     5     // GPIO5  -- SX1278's SCK
#define MISO    19    // GPIO19 -- SX1278's MISnO
#define MOSI    27    // GPIO27 -- SX1278's MOSI
#define SS      18    // GPIO18 -- SX1278's CS
#define RST     14    // GPIO14 -- SX1278's RESET
#define DI0     26      // GPIO26 -- SX1278's IRQ(Interrupt Request)
#define BATTERY_PIN 2 // battery level measurement pin, here is the voltage divider connected
#define BUZZER_PIN  13
#define BUZZER_CHANNEL 0
#define BAND  433E6
#define WDT_TIMEOUT 10
#define DHT_PIN 23

#define IMU_BUFFER_LEN 120 //10hz = 300 sec = 5 minutes
RingBuf<IMU_DATA, IMU_BUFFER_LEN> imu_buffer;

#define GPS_BUFFER_LEN 12 //1hz = 300 sec = 5 minutes
RingBuf<GPS_DATA, GPS_BUFFER_LEN> gps_buffer;

GPS_DATA gps_fix;
uint8_t *gps_fix_ptr = (uint8_t *) &gps_fix;
IMU_DATA imu_frame;
uint8_t *imu_frame_ptr = (uint8_t *) &imu_frame;
long start_millis;
DATE_TIME start_time;
long last_rx = 0;

TinyGPSPlus gps;
HardwareSerial GPS(1);
AXP20X_Class axp;
Adafruit_BNO055 bno = Adafruit_BNO055(-1, 0x28);
DHTesp dhtSensor;
TempAndHumidity dht_frame;


float getBatteryVoltage() {
    // we've set 10-bit ADC resolution 2^10=1024 and voltage divider makes it half of maximum readable value (which is 3.3V)
    float sum = 0;
    for (int j = 0; j < 10; j++) {
        sum += (float) analogRead(BATTERY_PIN);
    }
    sum /= 10;
    sum /= 4096;
    sum *= 6.6;
    return (sum);
}

bool axpPowerOn() {
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

bool axpPowerOff() {
    axp.setPowerOutPut(AXP192_LDO2, AXP202_OFF); // LORA radio
    axp.setPowerOutPut(AXP192_LDO3, AXP202_OFF); // GPS main power
    return true;
}

void LoRaScan() {
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
        Serial.println("Got packet");
        uint8_t packet[packetSize];
        for (int j = 0; j < packetSize; j++) {
            packet[j] = LoRa.read();
        }
        if (packetSize == sizeof(GPS_DATA)) {
            memcpy(&gps_fix, packet, sizeof(GPS_DATA));
            gps_buffer.push(gps_fix);
        } else if (packetSize == sizeof(IMU_DATA)) {
            memcpy(&imu_frame, packet, sizeof(IMU_DATA));
            imu_buffer.push(imu_frame);
        }
        last_rx = millis();
    }
}

FunctionTimer rx_handler(&LoRaScan, 20);


void WdtKick() {
    esp_task_wdt_reset();
}

FunctionTimer wdt_handler(&WdtKick, WDT_TIMEOUT - 1);

void IMUCal() {
    uint8_t system, gyro, accel, mag;
    system = gyro = accel = mag = 0;
    bno.getCalibration(&system, &gyro, &accel, &mag);
}

FunctionTimer imu_cal_handler(&IMUCal, 500);

DATE_TIME getTime() {
    DATE_TIME time_object;
    time_object.hour = uint8_t(gps.time.hour());
    time_object.min = uint8_t(gps.time.minute());
    time_object.sec = uint8_t(gps.time.second());
    time_object.cs = gps.time.centisecond();
    time_object.day = gps.date.day();
    time_object.month = gps.date.month();
    time_object.year = gps.date.year();
    return time_object;
}

IMU_DATA IMUUpdate() {
    IMU_DATA frame;
    sensors_event_t event;
    bno.getEvent(&event);
    frame.start = start_time;
    frame.pitch = event.orientation.z;
    frame.roll = event.orientation.y;
    frame.yaw = event.orientation.x;

    //So this reading actually takes some time, not shown in the database
    imu::Vector<3> lineacc = bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);
    frame.accelx = lineacc.x();
    frame.accely = lineacc.y();
    frame.accelz = lineacc.z();
    frame.dt = millis() - start_millis;

    return frame;

}
//FunctionTimer imu_handler(&IMUUpdate, 100);

GPS_DATA GPSUpdate() {
    Serial.print("Buffering GPS data...");
    GPS_DATA frame;
    frame.time = getTime();
    frame.lat = gps.location.lat();
    frame.lng = gps.location.lng();
    frame.sats = gps.satellites.value();
    frame.alt = gps.altitude.meters();
    Serial.println("done.");

    Serial.print("Buffering temp and humidity data...");
    dht_frame = dhtSensor.getTempAndHumidity();
    frame.temp = dht_frame.temperature;
    frame.humid = dht_frame.humidity;
    Serial.println("done.");

    Serial.print("Buffering battery data...");
    frame.batt = getBatteryVoltage();
    Serial.println("done.");

    Serial.println(dht_frame.temperature);
    return (frame);
}

#endif