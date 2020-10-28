#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ESPFlash.h>

#define DEFAULT_DISTANCE 30 //meters
#define MAX_DISTANCE 100
#define MIN_DISTANCE 10
#define DEFAULT_DOWN     100 //seconds
#define MAX_DOWN 500
#define MIN_DOWN 30
#define DEFAULT_BUDDYLOCK 1

const char* ssid = "sunsetvilla";
const char* password = "deptspecialboys";

AsyncWebServer server(80);

const char* PARAM_INPUT_1 = "distmax";
const char* PARAM_INPUT_2 = "downmax";
const char* PARAM_INPUT_3 = "buddylock";

void checkFirstRun(){
    if(!SPIFFS.exists("/distmax")){
        ESPFlash<int> espFlashInteger("/distmax");
        espFlashInteger.set(DEFAULT_DISTANCE);
    }
    if(!SPIFFS.exists("/downmax")){
        ESPFlash<int> espFlashInteger("/downmax");
        espFlashInteger.set(DEFAULT_DOWN);
    }
    if(!SPIFFS.exists("/buddylock")){
        ESPFlash<int> espFlashInteger("/buddylock");
        espFlashInteger.set(DEFAULT_BUDDYLOCK);
    }
}

String processor(const String& var){
  //Serial.println(var);
  if(var == "distmax"){
    ESPFlash<int> espFlashInteger("/distmax");
    return String(espFlashInteger.get(), DEC);
  }
  if(var == "downmax"){
    ESPFlash<int> espFlashInteger("/downmax");
    return String(espFlashInteger.get(), DEC);
  }
  if(var == "buddylock"){
    ESPFlash<int> espFlashInteger("/buddylock");
    int buddylock = espFlashInteger.get();
    if (buddylock == 0){
        return(String("False"));
    }else{
        return(String("True"));
    }
  }
  return String();
}



bool serverInit(){
    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
    }

    if(!SPIFFS.begin()){
        return false;
    }

    checkFirstRun();

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/index.html", String(), false, processor);
    });

    server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/config.html", "text/html");
    });

    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/style.css", "text/css");
    });

    server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
        //List all parameters (Compatibility)
        int args = request->args();
        for(int i=0;i<args;i++){
            String argName = request -> argName(i).c_str();
            String arg = request -> arg(i).c_str();

            if(argName == "distmax"){
                int dist = arg.toInt();
                if ((dist < MAX_DISTANCE) && (dist > MIN_DISTANCE)){
                    ESPFlash<int> espFlashInteger("/distmax");
                    espFlashInteger.set(dist);
                }else{
                    //handle form error
                }
            }

            if(argName == "downmax"){
                int down = arg.toInt();
                if((down < MAX_DOWN ) && (down > MIN_DOWN)){
                    ESPFlash<int> espFlashInteger("/downmax");
                    espFlashInteger.set(down);
                }else{
                    //handle form error
                }
            }

            if(argName == "buddylock"){
                ESPFlash<int> espFlashInteger("/buddylock");
                espFlashInteger.set(1);
            }

            Serial.printf("ARG[%s]: %s\n", request->argName(i).c_str(), request->arg(i).c_str());
        }
        if(! request->hasParam("buddylock")){
            ESPFlash<int> espFlashInteger("/buddylock");
            espFlashInteger.set(0);
        } 
        request->send(SPIFFS, "/index.html", String(), false, processor);
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