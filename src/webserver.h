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
LogFile gpsFile("/gpslog.txt");

void log(const String &line){
    File logfile = SPIFFS.open("/logfile.txt", FILE_APPEND);
    Serial.println(line);
    logfile.println(line);
    logfile.close();
}

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