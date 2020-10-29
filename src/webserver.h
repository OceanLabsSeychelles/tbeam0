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
void checkFirstRun();
String processor(const String&);

bool serverInit(){

    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
    }
    Serial.println("WiFi ok.");
    display.setTextSize(1);      // Normal 1:1 pixel scale
    display.println(WiFi.localIP());
    display.display();


    if(!SPIFFS.begin()){
        return false;
    }

    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/style.css", "text/css");
    });

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/index.html", String(), false, processor);
    });

    server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/config.html", String(), false, processor);
    });

    server.on("/scanning", HTTP_GET, [](AsyncWebServerRequest *request){
        if(buddy_found) {
            request->send(SPIFFS, "/foundbuddy.html", String(), false, processor);
        }else{
            request->send(SPIFFS, "/index.html", String(), false, processor);
        }
        });

    /*
    server.on("/foundbuddy", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/foundbuddy.html", String(), false, processor);
    });
    */

    server.on("/diveplan", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/diveplan.html", String(), false, processor);
    });

    server.on("/poweroff", HTTP_GET, [](AsyncWebServerRequest *request){
        server_on = false;
        request->send(SPIFFS, "/poweroff.html", String(), false, processor);
    });

    server.on("/app.js", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/app.js", "text/javascript");
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
                    distmax = dist;
                }else{
                    //handle form error
                }
            }

            if(argName == "downmax"){
                int down = arg.toInt();
                if((down < MAX_DOWN ) && (down > MIN_DOWN)){
                    ESPFlash<int> espFlashInteger("/downmax");
                    espFlashInteger.set(down);
                    downmax = down;
                }else{
                    //handle form error
                }
            }

            if(argName == "buddylock"){
                ESPFlash<int> espFlashInteger("/buddylock");
                espFlashInteger.set(1);
                buddylock = 1;
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

void checkFirstRun(){
    if(!SPIFFS.exists("/distmax")){
        ESPFlash<int> espFlashInteger("/distmax");
        espFlashInteger.set(DEFAULT_DISTANCE);
    }
    ESPFlash<int> espFlashInteger("/distmax");
    distmax = espFlashInteger.get();

    if(!SPIFFS.exists("/downmax")){
        ESPFlash<int> espFlashInteger("/downmax");
        espFlashInteger.set(DEFAULT_DOWN);
    }
    ESPFlash<int> espFlashInteger2("/downmax");
    downmax = espFlashInteger2.get();

    if(!SPIFFS.exists("/buddylock")){
        ESPFlash<int> espFlashInteger("/buddylock");
        espFlashInteger.set(DEFAULT_BUDDYLOCK);
    }
    ESPFlash<int> espFlashInteger3("/buddylock");
    buddylock = espFlashInteger3.get();
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
/*
template <typename HTML_VAR>
struct html_var{
    HTML_VAR val;
    HTML_VAR max;
    HTML_VAR min;
    HTML_VAR default_val;

    String name;
    String filename;
    bool maxmin;

    html_var(String _name, HTML_VAR _default_value){
        name = _name;
        filename = "/"+_name;
        default_val = _default_value;
        maxmin = false;
    }
    void set_limits(HTML_VAR _min, HTML_VAR _max){
        maxmin = true;
        min = _min;
        max = _max;
    }

    bool check_limits(HTML_VAR new_val){
        if((new_val < max) &&(new_val > min)){
            return true;
        }else{
            return false;
        }
    }

    bool write_flash(HTML_VAR new_val){
        ESPFlash<HTML_VAR> flashManager(filename.c_str());
        flashManager.set(new_val);
        return(true);
    }

    HTML_VAR get_flash(){
        ESPFlash<HTML_VAR> flashManager(filename.c_str());
        return(flashManager.get());
    }

    void check_exists(){
        if(!SPIFFS.exists(filename.c_str())){
            write_flash(default_val);
        }
    }
};

html_var<int> distmax("distmax", DEFAULT_DISTANCE);
html_var<int> downmax("downmax", DEFAULT_DOWN);
html_var<int> buddylock("buddylock", DEFAULT_BUDDYLOCK);
 */