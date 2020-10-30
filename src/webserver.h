#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ESPFlash.h>
#include <ESPFlashString.h>
#include <ESPmDNS.h>
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
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);
// DNS server
const byte DNS_PORT = 53;
DNSServer dnsServer;


class HtmlVar{
public:
    HtmlVar(const String &_varname, const String &_value, const String &_type);
    void flashWrite(const String &_value);
    void flashRead();
    void setLimits(const String &_min,const String &_max);
    void checkExists();
    String name;
    String value;
    String filename;
    String type;
    bool minmax = false;
    bool persistent = false;
    String min = "0";
    String max = "0";
};

HtmlVar::HtmlVar(const String &_varname, const String &_value, const String &_type){
    name=_varname;
    filename ="/"+_varname;
    type= _type;
    value= _value;
}

void HtmlVar::flashWrite(const String &_value) {
    if (persistent) {
        ESPFlashString flashString(filename.c_str());
        flashString.set(_value);
        value = _value;
    }
}

void HtmlVar::flashRead() {
    if (persistent) {
        ESPFlashString flashString(filename.c_str());
        value = flashString.get();
    }
}

void HtmlVar::setLimits(const String &_min,const String &_max){
    minmax = true;
    min = _min;
    max = _max;
}

void HtmlVar::checkExists() {
    if (persistent) {
        if (!SPIFFS.exists(filename)) {
            Serial.print(filename);
            Serial.println(" not found!");
            ESPFlashString flashManager(filename.c_str());
            flashManager.set(value);
        }else{
            Serial.print(filename);
            Serial.println(" found!");
            flashRead();
        }
    }
}

std::map<std::string, HtmlVar*> HtmlVarMap;
std::map<std::string, HtmlVar*>:: iterator it;


AsyncWebServer server(80);
void checkFirstRun();
void serverRoute();
String processor(const String&);


bool serverInit(){
    /* Values that are persistent MUST have a default value!! */
    HtmlVarMap.insert(std::make_pair("distmax", new HtmlVar("distmax", String(DEFAULT_DISTANCE), "int")));
    HtmlVarMap["distmax"]-> persistent = true;
    HtmlVarMap.insert(std::make_pair("downmax", new HtmlVar("downmax", String(DEFAULT_DOWN), "int")));
    HtmlVarMap["downmax"]-> persistent = true;
    HtmlVarMap.insert(std::make_pair("buddylock", new HtmlVar("buddylock", String(DEFAULT_BUDDYLOCK), "bool")));
    HtmlVarMap["buddylock"]-> persistent = true;
    HtmlVarMap.insert(std::make_pair("deviceid", new HtmlVar("deviceid", "uhuHunter", "string")));
    HtmlVarMap["deviceid"]-> persistent = true;

    HtmlVarMap.insert(std::make_pair("buddyid", new HtmlVar("buddyid", "", "string")));
    HtmlVarMap.insert(std::make_pair("bd-downmax", new HtmlVar("bd-downmax", "", "int")));
    HtmlVarMap.insert(std::make_pair("bd-distmax", new HtmlVar("bd-distmax", "", "int")));
    HtmlVarMap.insert(std::make_pair("bd-buddylock", new HtmlVar("bd-buddylock", "", "bool")));

    HtmlVarMap.insert(std::make_pair("divedist", new HtmlVar("divedist", "", "int")));
    HtmlVarMap.insert(std::make_pair("divedown", new HtmlVar("divedown", "", "int")));
    HtmlVarMap.insert(std::make_pair("divelock", new HtmlVar("divelock", "", "bool")));

    // Connect to Wi-Fi
    /* Setup the DNS server redirecting all the domains to the apIP */
    WiFi.softAP(host_ssid);
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(DNS_PORT, "*", local_ip);    Serial.println("WiFi ok.");
    display.setTextSize(2);      // Normal 1:1 pixel scale
    display.println(WiFi.softAPIP());
    display.display();
    delay(1000);
    /*
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
    }


    Serial.println("WiFi ok.");
    display.setTextSize(1);      // Normal 1:1 pixel scale
    display.println(WiFi.localIP());
    display.display();
    */
    for (it = HtmlVarMap.begin(); it != HtmlVarMap.end(); it ++){
        it -> second -> checkExists();
    }

    if(!SPIFFS.begin()){
        return false;
    }
    serverRoute();

    server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
        //List all parameters (Compatibility)
        int args = request->args();
        for(int i=0;i<args;i++){

            String argName = request -> argName(i).c_str();
            String arg = request -> arg(i).c_str();
            HtmlVarMap[argName.c_str()] -> flashWrite(arg.c_str());

            Serial.printf("ARG[%s]: %s\n", request->argName(i).c_str(), request->arg(i).c_str());
        }
        if(! request->hasParam("buddylock")){
            HtmlVarMap["buddylock"]->flashWrite("0");
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
    while(1){

    }
    return true;
}

String processor(const String& var){
    HtmlVarMap[var.c_str()]-> flashRead();
    return(HtmlVarMap[var.c_str()]-> value);
}

void serverRoute(){

    server.on("/wifi", handleWifi);
    server.on("/wifisave", handleWifiSave);
    server.on("/generate_204", handleRoot);  //Android captive portal. Maybe not needed. Might be handled by notFound handler.
    server.on("/fwlink", handleRoot);  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
    server.onNotFound ( handleNotFound );

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

    server.on("/foundbuddy", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/foundbuddy.html", String(), false, processor);
    });

    server.on("/diveplan", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/diveplan.html", String(), false, processor);
    });

    server.on("/ready", HTTP_GET, [](AsyncWebServerRequest *request){
        ready = true;
        request->send(SPIFFS, "/ready.html", String(), false, processor);
    });

    server.on("/app.js", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/app.js", "text/javascript");
    });
}
