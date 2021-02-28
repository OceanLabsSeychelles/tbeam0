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

DynamicJsonDocument doc(1024);
DynamicJsonDocument imuDoc(1024);

//const char* ssid = "LDN_EXT";
//const char* password = "blini010702041811";

const char* ssid = "sunsetvilla";
const char* password = "deptspecialboys";

LogFile gpsFile("/gpslog.txt");

void PostTest(){
    HTTPClient http;

    Serial.print("RestDB GPS data POST test...");

    String gpsUrl = "https://demobuoy-9613.restdb.io/rest/gpscoordinates";
    String imuUrl = "https://demobuoy-9613.restdb.io/rest/imudata";

    doc["latitude"] = 1.0;
    doc["longitude"] = 2.0;
    doc["satellites"] = 3.0;
    doc["elevation"] = 4.0;
    doc["temperature"] = 5.0;
    doc["battery"] = 6.0;
    doc["imucalibration"] = "anastystringhere";
    doc["time"] = "how is datetime handled in c?";
    String jsonData;
    serializeJson(doc, jsonData);

    http.begin(gpsUrl);
    http.addHeader("content-type", "application/json");
    http.addHeader( "x-apikey", "b525dd66d6a36e9394f23bd1a2d48ec702833");
    http.addHeader("cache-control" , "no-cache");
    int httpResponseCode = http.POST(jsonData);
    http.end();
    Serial.println(httpResponseCode);

    Serial.print("RestDB IMU data POST test...");
    imuDoc["accelx"] = 1.0;
    imuDoc["accely"] = 2.0;
    imuDoc["accelz"] = 3.0;
    imuDoc["pitch"] = 4.0;
    imuDoc["roll"] = 5.0;
    imuDoc["yaw"] = 6.0;
    imuDoc["start"] = "how is datetime handled in c?";
    imuDoc["dt"] = 7;
    String imuData;
    serializeJson(imuDoc, imuData);

    http.begin(imuUrl);
    http.addHeader("content-type", "application/json");
    http.addHeader( "x-apikey", "b525dd66d6a36e9394f23bd1a2d48ec702833");
    http.addHeader("cache-control" , "no-cache");
    httpResponseCode = http.POST(imuData);
    http.end();
    Serial.println(httpResponseCode);

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

    gpsFile.log("Init Ok.");

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