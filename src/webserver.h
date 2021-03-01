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

DynamicJsonDocument imuDoc(1024);

const char* ssid = "LDN_EXT";
const char* password = "blini010702041811";

//const char* ssid = "sunsetvilla";
//const char* password = "deptspecialboys";

LogFile gpsFile("/gpslog.txt");

void PostTest(){
    HTTPClient http;
    DynamicJsonDocument gpsDoc(1024);

    Serial.print("RestDB GPS data POST test...");

    String gpsUrl = "https://demobuoy-9613.restdb.io/rest/gpscoordinates";
    String imuUrl = "https://demobuoy-9613.restdb.io/rest/imudata";

    gpsDoc["latitude"] = 1.0;
    gpsDoc["longitude"] = 2.0;
    gpsDoc["satellites"] = 3.0;
    gpsDoc["elevation"] = 4.0;
    gpsDoc["temperature"] = 5.0;
    gpsDoc["battery"] = 6.0;
    gpsDoc["imucalibration"] = "anastystringhere";
    gpsDoc["time"] = "how is datetime handled in c?";
    String gpsData;
    serializeJson(gpsDoc, gpsData);

    http.begin(gpsUrl);
    http.addHeader("content-type", "application/json");
    http.addHeader( "x-apikey", "b525dd66d6a36e9394f23bd1a2d48ec702833");
    http.addHeader("cache-control" , "no-cache");
    int httpResponseCode = http.POST(gpsData);
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

int GpsPost(GPS_DATA data){
    HTTPClient http;
    DynamicJsonDocument gpsDoc(1024);
    String gpsUrl = "https://demobuoy-9613.restdb.io/rest/gpscoordinates";
    String gpsData;

    String ms, sec, min, hour, day, month, year, datetime;
    ms = String(data.time.cs);
    sec = String(data.time.sec);        
    min = String(data.time.min);    
    hour = String(data.time.hour);    
    day = String(data.time.day);    
    month = String(data.time.month);    
    year= "20"+String(data.time.year);  
    datetime = year+"-"+month+"-"+day+"T"+hour+":"+min+":"+sec+"."+ms;

    gpsDoc["latitude"] = data.lat;
    gpsDoc["longitude"] =data.lng;
    gpsDoc["satellites"] = int(data.sats);
    gpsDoc["elevation"] = data.alt;
    gpsDoc["temperature"] = data.temp;
    gpsDoc["battery"] = data.batt;
    gpsDoc["imucalibration"] = "anastystringhere";
    gpsDoc["time"] = datetime;
    serializeJson(gpsDoc, gpsData);

    Serial.print("RestDB GPS POST...");
    http.begin(gpsUrl);
    http.addHeader("content-type", "application/json");
    http.addHeader( "x-apikey", "b525dd66d6a36e9394f23bd1a2d48ec702833");
    http.addHeader("cache-control" , "no-cache");
    int httpResponseCode = http.POST(gpsData);
    http.end();
    Serial.println(httpResponseCode);
    return(httpResponseCode);
}

int ImuPost(IMU_DATA data){
    HTTPClient http;
    DynamicJsonDocument imuDoc(1024);
    String imuUrl = "https://demobuoy-9613.restdb.io/rest/imudata";
    String imuData;

    String ms, sec, min, hour, day, month, year, datetime;
    ms = String(data.start.cs);
    sec = String(data.start.sec);        
    min = String(data.start.min);    
    hour = String(data.start.hour);    
    day = String(data.start.day);    
    month = String(data.start.month);    
    year= "20"+String(data.start.year);  
    datetime = year+"-"+month+"-"+day+"T"+hour+":"+min+":"+sec+"."+ms;

    imuDoc["accelx"] = data.accelx;
    imuDoc["accely"] = data.accely;
    imuDoc["accelz"] = data.accelz;
    imuDoc["pitch"] = data.pitch;
    imuDoc["roll"] = data.roll;
    imuDoc["yaw"] = data.yaw;
    imuDoc["start"] = datetime;
    imuDoc["dt"] = data.dt;
    serializeJson(imuDoc, imuData);

    Serial.print("RestDB IMU data POST test...");
    http.begin(imuUrl);
    http.addHeader("content-type", "application/json");
    http.addHeader( "x-apikey", "b525dd66d6a36e9394f23bd1a2d48ec702833");
    http.addHeader("cache-control" , "no-cache");
    int httpResponseCode = http.POST(imuData);
    http.end();
    Serial.println(httpResponseCode);
    return(httpResponseCode);
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