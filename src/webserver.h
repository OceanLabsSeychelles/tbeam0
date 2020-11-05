#ifndef TTBEAM0_WEBSERVER_H
#define TTBEAM0_WEBSERVER_H

#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ESPFlash.h>
#include <ESPFlashString.h>
#include <ESPmDNS.h>
#include <HtmlVar.h>
#include <main.h>
#include <map>

volatile const int DEFAULT_DISTANCE = 30; //meters
#define MAX_DISTANCE 100
#define MIN_DISTANCE 10
volatile const int DEFAULT_DOWN = 100; //seconds
#define MAX_DOWN 500
#define MIN_DOWN 30
volatile const int DEFAULT_BUDDYLOCK = 1;
String DEFAULT_ID = "uhuHunter";

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

bool WiFiInit(bool host=false){
    if(!host){
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) {
            delay(1000);
        }
        Serial.println("WiFi ok.");

    }else{
        WiFi.softAP(host_ssid);
        dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
        dnsServer.start(DNS_PORT, "*", local_ip);    Serial.println("WiFi ok.");
    }
    delay(1000);
    return true;
}


bool FlashInit(){

    if(!SPIFFS.begin()){
        return false;
    }
    //logFileInit();

    File root = SPIFFS.open("/");
    File file = root.openNextFile();

    while(file){
        Serial.print("FILE: ");
        Serial.println(file.name());
        file = root.openNextFile();
    }

    if(file){
        file.close();
    }

    for (html_it = HtmlVarMap.begin(); html_it != HtmlVarMap.end(); html_it ++){
        html_it -> second -> checkExists();
    }

    Serial.print("Used Memory: ");
    Serial.print(SPIFFS.usedBytes()/1000);
    Serial.println(" kbytes");

    Serial.print("Free Memory : ");
    Serial.print((SPIFFS.totalBytes()-SPIFFS.usedBytes())/1000);
    Serial.println(" kbytes");

    return true;
}


bool serverInit(){
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
    HtmlVarMap.insert(std::make_pair("bd-lng", new HtmlVar("bd-lng", "", "double")));
    HtmlVarMap.insert(std::make_pair("bd-lat", new HtmlVar("bd-lat", "", "double")));
    HtmlVarMap.insert(std::make_pair("bd-sats", new HtmlVar("bd-sats", "", "int")));

    HtmlVarMap.insert(std::make_pair("buddyid", new HtmlVar("buddyid", "", "string")));
    HtmlVarMap.insert(std::make_pair("bd-downmax", new HtmlVar("bd-downmax", "", "int")));
    HtmlVarMap.insert(std::make_pair("bd-distmax", new HtmlVar("bd-distmax", "", "int")));
    HtmlVarMap.insert(std::make_pair("bd-buddylock", new HtmlVar("bd-buddylock", "", "bool")));

    HtmlVarMap.insert(std::make_pair("divedist", new HtmlVar("divedist", "", "int")));
    HtmlVarMap.insert(std::make_pair("divedown", new HtmlVar("divedown", "", "int")));
    HtmlVarMap.insert(std::make_pair("divelock", new HtmlVar("divelock", "", "bool")));

}

String processor(const String& var){
    HtmlVarMap[var.c_str()]-> flashRead();
    return(HtmlVarMap[var.c_str()]-> value);
}

void serverRoute(){

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
}

#endif