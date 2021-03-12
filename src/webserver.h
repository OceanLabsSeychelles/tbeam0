#ifndef TTBEAM0_WEBSERVER_H
#define TTBEAM0_WEBSERVER_H

#include "main.h"
#include "FS.h"
#include "LITTLEFS.h"

#define SPIFFS LITTLEFS

#include <HTTPClient.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include <LogFile.h>
#include <datatypes.h>

//const char *ssid = "LDN_EXT";
//const char *password = "blini010702041811";

const char* ssid = "sunsetvilla";
const char* password = "deptspecialboys";

void imu2json(JsonDocument &imuDoc, IMU_DATA data){

    String ms, sec, min, hour, day, month, year, datetime;
    ms = String(data.start.cs);
    sec = String(data.start.sec);
    min = String(data.start.min);
    hour = String(data.start.hour);
    day = String(data.start.day);
    month = String(data.start.month);
    year = String(data.start.year);
    datetime = year + "-" + month + "-" + day + "T" + hour + ":" + min + ":" + sec + "." + ms;


    imuDoc["accelx"] = data.accelx;
    imuDoc["accely"] = data.accely;
    imuDoc["accelz"] = data.accelz;
    imuDoc["pitch"] = data.pitch;
    imuDoc["roll"] = data.roll;
    imuDoc["yaw"] = data.yaw;
    imuDoc["start"] = datetime;
    imuDoc["dt"] = data.dt;

}

void gps2json(JsonDocument &gpsDoc, GPS_DATA data){
    
    String ms, sec, min, hour, day, month, year, datetime;

    ms = String(data.time.cs);
    sec = String(data.time.sec);
    min = String(data.time.min);
    hour = String(data.time.hour);
    day = String(data.time.day);
    month = String(data.time.month);
    year = String(data.time.year);

    datetime = year + "-" + month + "-" + day + "T" + hour + ":" + min + ":" + sec + "." + ms;
    
    gpsDoc["latitude"] = data.lat;
    gpsDoc["longitude"] = data.lng;
    gpsDoc["satellites"] = int(data.sats);
    gpsDoc["elevation"] = data.alt;
    gpsDoc["temperature"] = data.temp;
    gpsDoc["humidity"] = data.humid;
    gpsDoc["battery"] = data.batt;
    gpsDoc["imucalibration"] = "anastystringhere";
    gpsDoc["time"] = datetime;

}

LogFile gpsFile("/gpslog.txt");

int GpsPost(GPS_DATA data, bool last=false) {
    HTTPClient http;
    DynamicJsonDocument gpsDoc(1024);
    String firebaseUrl = "https://demobouy-8aabf-default-rtdb.europe-west1.firebasedatabase.app/gpscoordinates";
    String topGpsUrl = "https://demobouy-8aabf-default-rtdb.europe-west1.firebasedatabase.app/gpscoordinates/last.json";
    String gpsData;
    String ms, sec, min, hour, day, month, year, datetime;

    ms = String(data.time.cs);
    sec = String(data.time.sec);
    min = String(data.time.min);
    hour = String(data.time.hour);
    day = String(data.time.day);
    month = String(data.time.month);
    year = String(data.time.year);
    datetime = year + "-" + month + "-" + day + "T" + hour + ":" + min + ":" + sec;
    firebaseUrl+=("/"+year + "-" + month + "-" + day + "/" + hour + ":" + min + ":" + sec+".json");

    gpsDoc["latitude"] = data.lat;
    gpsDoc["longitude"] = data.lng;
    gpsDoc["satellites"] = int(data.sats);
    gpsDoc["elevation"] = data.alt;
    gpsDoc["temperature"] = data.temp;
    gpsDoc["humidity"] = data.humid;
    gpsDoc["battery"] = data.batt;
    gpsDoc["imucalibration"] = "anastystringhere";
    gpsDoc["time"] = datetime;
    serializeJson(gpsDoc, gpsData);

    Serial.print("Firebase GPS PUT...");
    if(!last){
        http.begin(firebaseUrl);
    }else{
        http.begin(topGpsUrl);
    }
    http.addHeader("content-type", "application/json");
    //http.addHeader( "x-apikey", "b525dd66d6a36e9394f23bd1a2d48ec702833");
    //http.addHeader("cache-control" , "no-cache");
    int httpResponseCode = http.PATCH(gpsData);
    http.end();
    Serial.println(httpResponseCode);
    return (httpResponseCode);
}

int ImuPost(IMU_DATA data) {
    HTTPClient http;
    DynamicJsonDocument imuDoc(1024);
    String firebaseUrl = "https://demobouy-8aabf-default-rtdb.europe-west1.firebasedatabase.app/imudata";    
    String imuData;

    String ms, sec, min, hour, day, month, year, datetime;
    ms = String(data.start.cs);
    sec = String(data.start.sec);
    min = String(data.start.min);
    hour = String(data.start.hour);
    day = String(data.start.day);
    month = String(data.start.month);
    year = String(data.start.year);
    datetime = year + "-" + month + "-" + day + "T" + hour + ":" + min + ":" + sec;
    firebaseUrl+=("/"+year + "-" + month + "-" + day + "/" + hour + ":" + min + ":" + sec+"/"+data.dt+".json");

    imuDoc["accelx"] = data.accelx;
    imuDoc["accely"] = data.accely;
    imuDoc["accelz"] = data.accelz;
    imuDoc["pitch"] = data.pitch;
    imuDoc["roll"] = data.roll;
    imuDoc["yaw"] = data.yaw;
    imuDoc["start"] = datetime;
    imuDoc["dt"] = data.dt;
    serializeJson(imuDoc, imuData);

    Serial.print("Firebase IMU PATCH...");
    http.begin(firebaseUrl);
    http.addHeader("content-type", "application/json");
    //http.addHeader("x-apikey", "b525dd66d6a36e9394f23bd1a2d48ec702833");
    //http.addHeader("cache-control", "no-cache");
    int httpResponseCode = http.PATCH(imuData);
    http.end();
    Serial.println(httpResponseCode);
    return (httpResponseCode);
}

void log(const String &line) {
    File logfile = SPIFFS.open("/logfile.txt", FILE_APPEND);
    Serial.println(line);
    logfile.println(line);
    logfile.close();
}

bool WiFiInit(bool host = false) {

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
    }
    Serial.println(WiFi.localIP());

    return true;
}

bool FlashInit() {

    if (!SPIFFS.begin()) {
        return false;
    }
    if (SPIFFS.exists("/logfile.txt")) {
        SPIFFS.remove("/logfile.txt");
    }

    File logfile = SPIFFS.open("/logfile.txt", FILE_WRITE);
    logfile.println("Log file init ok.");
    logfile.close();

    gpsFile.log("Init Ok.");

    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    log("__FILE SYSTEM__");


    while (file) {
        const String name = file.name();
        log(file.name());

        file = root.openNextFile();
    }

    if (file) {
        file.close();
    }

    log("_______________");

    String used = String(SPIFFS.usedBytes() / 1000);
    log(String("Used Memory (kB): " + used));

    String free = String((SPIFFS.totalBytes() - SPIFFS.usedBytes()) / 1000);
    log(String("Free Memory (kB): " + free));

    return true;
}

#endif