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
#include <NTPClient.h>
#include "time.h"

const char* ssid = "LDN_EXT";
const char* password = "blini010702041811";

//const char* ssid = "sunsetvilla";
//const char* password = "deptspecialboys";

int imuPatch(String data){
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
        Serial.println("Failed to obtain time. Data PATCH cancelled...");
        return -999;
    }
    String sec= String(timeinfo.tm_sec);
    String min= String(timeinfo.tm_min);
    String hour= String(timeinfo.tm_hour);
    String day= String(timeinfo.tm_yday);
    String year = String(timeinfo.tm_year);

    HTTPClient http;
    int httpResponseCode;
    String imuUrl = "https://demobouy-8aabf-default-rtdb.europe-west1.firebasedatabase.app/imudata";
    imuUrl += ("/"+year+"-"+day+"/"+hour+":"+min+":"+sec+".json");
    Serial.print("Firebase IMU PATCH...");
    http.begin(imuUrl);        
    http.addHeader("content-type", "application/json");
    //http.addHeader( "x-apikey", "b525dd66d6a36e9394f23bd1a2d48ec702833");
    httpResponseCode = http.PATCH(data);
    http.end();
    Serial.println(httpResponseCode);
    return(httpResponseCode);
}

int gpsPatch(String data){
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
        Serial.println("Failed to obtain time. Data PATCH cancelled...");
        return -999;
    }
    String sec= String(timeinfo.tm_sec);
    String min= String(timeinfo.tm_min);
    String hour= String(timeinfo.tm_hour);
    String day= String(timeinfo.tm_yday);
    String year = String(timeinfo.tm_year);


    HTTPClient http;
    int httpResponseCode;
    String gpsUrl = "https://demobouy-8aabf-default-rtdb.europe-west1.firebasedatabase.app/gpscoordinates";
    gpsUrl += ("/"+year+"-"+day+"/"+hour+":"+min+":"+sec+".json");
    Serial.print("Firebase GPS PATCH...");
    http.begin(gpsUrl);        
    http.addHeader("content-type", "application/json");
    //http.addHeader( "x-apikey", "b525dd66d6a36e9394f23bd1a2d48ec702833");
    httpResponseCode = http.PATCH(data);
    http.end();
    Serial.println(httpResponseCode);
    return(httpResponseCode);
}


int imuPutLast(String data){
    HTTPClient http;
    int httpResponseCode;
    String imuUrl = "https://demobouy-8aabf-default-rtdb.europe-west1.firebasedatabase.app/imudata/last.json";
    Serial.print("Firebase IMU PUT...");
    http.begin(imuUrl);        
    http.addHeader("content-type", "application/json");
    //http.addHeader( "x-apikey", "b525dd66d6a36e9394f23bd1a2d48ec702833");
    httpResponseCode = http.PUT(data);
    http.end();
    Serial.println(httpResponseCode);
    return(httpResponseCode);
}

int gpsPutLast(String data){
    HTTPClient http;
    int httpResponseCode;
    String gpsUrl = "https://demobouy-8aabf-default-rtdb.europe-west1.firebasedatabase.app/gpscoordinates/last.json";
    Serial.print("Firebase GPS PUT...");
    http.begin(gpsUrl);        
    http.addHeader("content-type", "application/json");
    //http.addHeader( "x-apikey", "b525dd66d6a36e9394f23bd1a2d48ec702833");
    httpResponseCode = http.PUT(data);
    http.end();
    Serial.println(httpResponseCode);
    return(httpResponseCode);
}

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
    //gpsDoc["humidity"] = data.humid;
    gpsDoc["battery"] = data.batt;
    //gpsDoc["imucalibration"] = "anastystringhere";
    gpsDoc["time"] = datetime;

}

void log(const String &line){
    File logfile = SPIFFS.open("/logfile.txt", FILE_APPEND);
    Serial.println(line);
    logfile.println(line);
    logfile.close();
}

bool WiFiInit(bool host=false){

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
    }
    Serial.println(WiFi.localIP());

    return true;
}

bool FlashInit(){

    if(!SPIFFS.begin()){
        return false;
    }
    if(SPIFFS.exists("/logfile.txt")){
        SPIFFS.remove("/logfile.txt");
    }

    File logfile = SPIFFS.open("/logfile.txt", FILE_WRITE);
    logfile.println("Log file init ok.");
    logfile.close();

    //gpsFile.log("Init Ok.");

    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    log("__FILE SYSTEM__");


    while(file){
        const String name = file.name();
        log(file.name());

        file = root.openNextFile();
    }

    if(file) {
        file.close();
    }

    log("_______________");

    String used = String(SPIFFS.usedBytes()/1000);
    log(String("Used Memory (kB): "+used));

    String free = String((SPIFFS.totalBytes()-SPIFFS.usedBytes())/1000);
    log(String("Free Memory (kB): "+free));

    return true;
}
#endif