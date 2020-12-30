#ifndef TTBEAM0_WEBSERVER_H
#define TTBEAM0_WEBSERVER_H

#include "FS.h"
//#include "SPIFFS.h"
#include "LITTLEFS.h"
#define SPIFFS LITTLEFS

#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <ESPFlash.h>
#include <ESPFlashString.h>
#include <ESPmDNS.h>
#include <HtmlVar.h>
#include <main.h>
#include <map>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <LogFile.h>

StaticJsonDocument<1024> doc;

volatile const int DEFAULT_DISTANCE = 30; //meters
#define MAX_DISTANCE 100
#define MIN_DISTANCE 10
volatile const int DEFAULT_DOWN = 100; //seconds
#define MAX_DOWN 500
#define MIN_DOWN 30
volatile const int DEFAULT_BUDDYLOCK = 1;
String DEFAULT_ID = "uhuHunter";

//const char* ssid = "LDN_EXT";
//const char* password = "blini010702041811";

//const char* ssid = "brettsphone";
//const char* password = "whiskeytango";

const char* ssid = "sunsetvilla";
const char* password = "deptspecialboys";

//For device as AP
const char* host_ssid = "zero2spearo";
const char* host_password = "testpassword";
const char* dns = "buddytracker";

IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);
// DNS server
const byte DNS_PORT = 53;
DNSServer dnsServer;

AsyncWebServer server(80);
String processor(const String&);

std::map<std::string, HtmlVar*> HtmlVarMap;
std::map<std::string, HtmlVar*>:: iterator html_it;

void checkFirstRun();
void serverRoute();
void fillHtmlMap();

void log(const String &line){
    File logfile = SPIFFS.open("/logfile.txt", FILE_APPEND);
    Serial.println(line);
    logfile.println(line);
    logfile.close();
}

LogFile gpsFile("/gpslog.txt");

bool WiFiInit(bool host=false){
    if(!host){
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) {
            delay(1000);
        }
        Serial.println(WiFi.localIP());
    }else{
        WiFi.softAP(host_ssid);
        dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
        dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
        Serial.println("WiFi AP ok.");
    }
    //delay(1000);
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

    for (html_it = HtmlVarMap.begin(); html_it != HtmlVarMap.end(); html_it ++){
        html_it -> second -> checkExists();
    }

    String used = String(SPIFFS.usedBytes()/1000);
    log(String("Used Memory (kB): "+used));

    String free = String((SPIFFS.totalBytes()-SPIFFS.usedBytes())/1000);
    log(String("Free Memory (kB): "+free));

    return true;
}


bool serverInit(){
    //https://github.com/me-no-dev/ESPAsyncWebServer/issues/726
    server.on("/var/gps", HTTP_GET, [](AsyncWebServerRequest *request) {
        doc.clear();
        doc["lng"] = HtmlVarMap["lng"] -> value;
        doc["lat"] = HtmlVarMap["lat"] -> value;
        doc["bdLng"] = HtmlVarMap["bd-lng"] -> value;
        doc["bdLat"] = HtmlVarMap["bd-lat"] -> value;
        doc["bdBearing"] = HtmlVarMap["bd-bearing"] -> value;
        doc["bdTxTime"] = HtmlVarMap["bdTxTime"] -> value;


        //JsonArray data = doc.createNestedArray("data");
        //data.add(48.756080);
        //data.add(2.302038);
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    server.on("/var/imu", HTTP_GET, [](AsyncWebServerRequest *request) {
        doc.clear();
        doc["imuHeading"] = HtmlVarMap["imu-heading"]-> value;
        doc["imuPitch"] = HtmlVarMap["imu-pitch"]-> value;
        doc["imuRoll"] = HtmlVarMap["imu-roll"]-> value;
        doc["magCal"] = HtmlVarMap["mag-cal"]->value;
        doc["accelCal"] = HtmlVarMap["accel-cal"]->value;
        doc["gyroCal"] = HtmlVarMap["gyro-cal"]->value;

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    server.on("/var/config", HTTP_GET, [](AsyncWebServerRequest *request) {
        doc.clear();
        doc["lng"] = HtmlVarMap["lng"] -> value;
        doc["lat"] = HtmlVarMap["lat"] -> value;
        doc["bdLng"] = HtmlVarMap["bd-lng"] -> value;
        doc["bdLat"] = HtmlVarMap["bd-lat"] -> value;
        doc["bdBearing"] = HtmlVarMap["bd-bearing"] -> value;
        doc["bdTxTime"] = HtmlVarMap["bdTxTime"] -> value;

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    server.on("/var/buddy", HTTP_GET, [](AsyncWebServerRequest *request) {
        doc.clear();

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    serverRoute();

    server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
        //List all parameters (Compatibility)
        int args = request->args();
        for(int i=0;i<args;i++){

            String argName = request -> argName(i).c_str();
            String arg = request -> arg(i).c_str();
            if(HtmlVarMap[argName.c_str()] -> persistent){
                HtmlVarMap[argName.c_str()] -> flashWrite(arg.c_str());
            }else{
                HtmlVarMap[argName.c_str()] -> value = arg.c_str();
            }


            Serial.printf("ARG[%s]: %s\n", request->argName(i).c_str(), request->arg(i).c_str());
        }
        if(! request->hasParam("buddylock")){
            HtmlVarMap["buddylock"]->flashWrite("0");
        }
        request->send(SPIFFS, "/index.html", String(), false, processor);
    });

    server.on("/var/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
        //List all parameters (Compatibility)
        String response_text;
        int args = request->args();
        for(int i=0;i<args;i++) {

            String argName = request->argName(i).c_str();
            //String arg = request->arg(i).c_str();
            response_text += argName.c_str();
            response_text += "=";
            response_text += HtmlVarMap[argName.c_str()]-> value.c_str();
            if (i<args-1) {
                response_text += "&";
            }
        }
        request->send(200, "text/plain", response_text);
    });

    server.on("/power-off", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/index.html", String(), false, processor);
        server.end();
        WiFi.mode(WIFI_OFF);
        btStop();
    });
    server.begin();
    return true;
}

void fillHtmlMap(){
    /* Values that are persistent MUST have a default value!! */
    HtmlVarMap.insert(std::make_pair("distmax", new HtmlVar("distmax", String(DEFAULT_DISTANCE), "int")));
    HtmlVarMap["distmax"]-> persistent = true;
    HtmlVarMap.insert(std::make_pair("downmax", new HtmlVar("downmax", String(DEFAULT_DOWN), "int")));
    HtmlVarMap["downmax"]-> persistent = true;
    HtmlVarMap.insert(std::make_pair("buddylock", new HtmlVar("buddylock", String(DEFAULT_BUDDYLOCK), "bool")));
    HtmlVarMap["buddylock"]-> persistent = true;
    HtmlVarMap.insert(std::make_pair("deviceid", new HtmlVar("deviceid", "uhuHunter", "string")));
    HtmlVarMap["deviceid"]-> persistent = true;
    HtmlVarMap.insert(std::make_pair("logcount", new HtmlVar("logcount", "0", "int")));
    HtmlVarMap["logcount"]-> persistent = true;

    HtmlVarMap.insert(std::make_pair("lin-accel", new HtmlVar("lin-accel", "", "float")));
    
    HtmlVarMap.insert(std::make_pair("lng", new HtmlVar("lng", "", "double")));
    HtmlVarMap.insert(std::make_pair("lat", new HtmlVar("lat", "", "double")));
    HtmlVarMap.insert(std::make_pair("sats", new HtmlVar("sats", "", "int")));
    HtmlVarMap.insert(std::make_pair("alt", new HtmlVar("alt", "", "float")));
    HtmlVarMap.insert(std::make_pair("bd-lng", new HtmlVar("bd-lng", "55.431476", "double")));
    HtmlVarMap.insert(std::make_pair("bd-lat", new HtmlVar("bd-lat", "-4.561342", "double")));
    HtmlVarMap.insert(std::make_pair("bd-sats", new HtmlVar("bd-sats", "", "int")));
    HtmlVarMap.insert(std::make_pair("bd-bearing", new HtmlVar("bd-bearing", "", "float")));

    HtmlVarMap.insert(std::make_pair("buddyid", new HtmlVar("buddyid", "", "string")));
    HtmlVarMap.insert(std::make_pair("bd-downmax", new HtmlVar("bd-downmax", "", "int")));
    HtmlVarMap.insert(std::make_pair("bd-distmax", new HtmlVar("bd-distmax", "", "int")));
    HtmlVarMap.insert(std::make_pair("bd-buddylock", new HtmlVar("bd-buddylock", "", "bool")));
    HtmlVarMap.insert(std::make_pair("bdTxTime", new HtmlVar("bdTxTime", "", "float")));

    HtmlVarMap.insert(std::make_pair("divedist", new HtmlVar("divedist", "", "int")));
    HtmlVarMap.insert(std::make_pair("divedown", new HtmlVar("divedown", "", "int")));
    HtmlVarMap.insert(std::make_pair("divelock", new HtmlVar("divelock", "", "bool")));
    
    HtmlVarMap.insert(std::make_pair("mag-cal", new HtmlVar("mag-cal", "", "int")));
    HtmlVarMap.insert(std::make_pair("accel-cal", new HtmlVar("accel-cal", "", "int")));
    HtmlVarMap.insert(std::make_pair("gyro-cal", new HtmlVar("gyro-cal", "", "int")));
    HtmlVarMap.insert(std::make_pair("imu-heading", new HtmlVar("imu-heading", "", "float")));
    HtmlVarMap.insert(std::make_pair("imu-roll", new HtmlVar("imu-roll", "", "float")));
    HtmlVarMap.insert(std::make_pair("imu-pitch", new HtmlVar("imu-pitch", "", "float")));

}

String processor(const String& var){
    HtmlVarMap[var.c_str()]-> flashRead();
    return(HtmlVarMap[var.c_str()]-> value);
}

void serverRoute(){

    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/index.html", String(), false, processor);
    });

    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/style.css", "text/css");
    });

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/index.html", String(), false, processor);
    });

    server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/config.html", String(), false, processor);
    });

    /*
    server.on("/scanning", HTTP_GET, [](AsyncWebServerRequest *request){
        if(buddy_found) {
            request->send(SPIFFS, "/foundbuddy.html", String(), false, processor);
        }else{
            request->send(SPIFFS, "/index.html", String(), false, processor);
        }
    });
     */

    server.on("/foundbuddy", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/foundbuddy.html", String(), false, processor);
    });

    server.on("/diveplan", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/diveplan.html", String(), false, processor);
    });

    server.on("/ready", HTTP_GET, [](AsyncWebServerRequest *request){
        //ready = true;
        request->send(SPIFFS, "/ready.html", String(), false, processor);
    });

    server.on("/app.js", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/app.js", "text/javascript");
    });

    server.on("/log", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/logfile.txt", "text/plain");
    });

    server.on("/navigation.js", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/navigation.js", "text/javascript");
    });

    server.on("/app", HTTP_GET, [](AsyncWebServerRequest *request){
        //ready = true;
        request->send(SPIFFS, "/navigation.html", String(), false, processor);
    });

    server.on("/marker-red", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/mapMarkers/marker-red.png", "image/png");
    });

    server.on("/marker-green", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/mapMarkers/marker-green.png", "image/png");
    });

    server.on("/arrow-blue", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/mapMarkers/arrow-blue.png", "image/png");
    });
}

#endif