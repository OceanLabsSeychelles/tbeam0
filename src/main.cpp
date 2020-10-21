#include "main.h"
#include "geometry.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

typedef struct{
  double lat;
  double lng;
  int sats;
} GPS_DATA;

GPS_DATA gps_fix;
uint8_t* gps_fix_ptr = (uint8_t*)&gps_fix;

typedef struct{
  GPS_DATA info;
  double last_time;
} PARTNER_DATA;

PARTNER_DATA partner_fix;

long last_scan = millis();
long last_send = millis();
long last_screen = millis();
int interval = random(500);
double distance = 0.0;
double bearing = 0.0;

float getBatteryVoltage() {
  // we've set 10-bit ADC resolution 2^10=1024 and voltage divider makes it half of maximum readable value (which is 3.3V)
  float sum = 0;
  for (int j = 0; j< 10; j++){
    sum += analogRead(BATTERY_PIN);
  }
  sum /= 10;
  sum /= 1024;
  sum *= 3.3;
  return (sum); 
}

void OTAInit(){
  display.setCursor(0, 0);     // Start at top-left corner
  display.clearDisplay();
  display.println("Starting WiFi");
  display.display();
  delay(1000);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
   ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  display.setCursor(0, 0);     // Start at top-left corner
  display.clearDisplay();
  display.println("OTA Update");
  display.print("IP address: ");
  display.println(WiFi.localIP());
  display.display();
  delay(1000);

  //while(true){
  //  ArduinoOTA.handle();
  //}
}

void IRAM_ATTR ISR() {
  display.setCursor(0, 0);     // Start at top-left corner
  display.clearDisplay();
  display.println("BUTTON!");
  display.display();
}

void setup()
{
  // set battery measurement pin
  Serial.begin(115200);
  Wire.begin(21, 22);
  if (!axp.begin(Wire, AXP192_SLAVE_ADDRESS)) {
    Serial.println("AXP192 Begin PASS");
  } else {
    Serial.println("AXP192 Begin FAIL");
  }
  axp.setPowerOutPut(AXP192_LDO2, AXP202_ON); // LORA radio
  axp.setPowerOutPut(AXP192_LDO3, AXP202_ON); // GPS main power
  axp.setPowerOutPut(AXP192_DCDC2, AXP202_ON);
  axp.setPowerOutPut(AXP192_EXTEN, AXP202_ON);
  axp.setPowerOutPut(AXP192_DCDC1, AXP202_ON);
  axp.setDCDC1Voltage(3300); // for the OLED power

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    //for(;;); // Don't proceed, loop forever
  }

  pinMode(USER_BUTTON, INPUT);
  attachInterrupt(USER_BUTTON, ISR, FALLING);

  adcAttachPin(BATTERY_PIN);
  adcStart(BATTERY_PIN);
  analogReadResolution(10); // Default of 12 is not very linear. Recommended to use 10 or 11 depending on needed resolution.

  display.clearDisplay();
  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
  display.setCursor(0, 0);     // Start at top-left corner
  display.println("Buddy");
  display.println("Tracker");
  display.display();
  delay(2000);
  
  OTAInit();


  GPS.begin(9600, SERIAL_8N1, 34, 12);   //17-TX 18-RX
  SPI.begin(SCK,MISO,MOSI,SS);
  LoRa.setPins(SS,RST,DI0);
  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  
  display.setCursor(0, 0);     // Start at top-left corner
  display.clearDisplay();
  display.println("Init Ok.");
  display.print("VBat: ");
  display.println(getBatteryVoltage());
  display.display();
  delay(1000);
  display.clearDisplay();
  display.setCursor(0, 0);     // Start at top-left corner
  display.println("GPS");
  display.println("Acquiring");
  display.display();  

  while(gps.satellites.value() < 3){
    gps.encode(GPS.read());
  }
  gps_fix.lat = gps.location.lat();
  gps_fix.lng = gps.location.lng();
  gps_fix.sats = gps.satellites.value();

  display.clearDisplay();
  display.setCursor(0, 0);     // Start at top-left corner
  display.println("Fix");
  display.println("Acquired");
  display.display();
  delay(1000);

  display.clearDisplay();
  display.setCursor(0, 0);     // Start at top-left corner
  display.println("Waiting");
  display.println("For");
  display.println("Partner");
  display.display();

  int packetSize = 0;
  while(packetSize == 0){
    packetSize = LoRa.parsePacket();
    if (packetSize) { 

      uint8_t packet[packetSize];
      for(int j =0; j< packetSize; j++){
        packet[j] = LoRa.read();
      }
    }
    if (millis() - last_send>(1000 + interval)){
      LoRa.beginPacket();
      LoRa.write(gps_fix_ptr, sizeof(GPS_DATA));
      LoRa.endPacket();
      last_send = millis();
      interval = random(500);
    }
  }
  
  display.clearDisplay();
  display.setCursor(0, 0);     // Start at top-left corner
  display.println("Partner");
  display.println("Acquired");
  display.display();
  delay(1000);

}

void loop(){
  while(GPS.available()){
     gps.encode(GPS.read());
  }
  if(gps.location.isUpdated()){
    gps_fix.lat = gps.location.lat();
    gps_fix.lng = gps.location.lng();
    gps_fix.sats = gps.satellites.value();
  }
  if (millis() - last_send>(1000 + interval)){
    LoRa.beginPacket();
    LoRa.write(gps_fix_ptr, sizeof(GPS_DATA));
    LoRa.endPacket();
    last_send = millis();
  }
  
  if(millis() - last_scan > 20){
    int packetSize = LoRa.parsePacket();
      if (packetSize) { 

        uint8_t packet[packetSize];
        for(int j =0; j< packetSize; j++){
          packet[j] = LoRa.read();
        }

        memcpy(&partner_fix.info, packet, sizeof(GPS_DATA));
        partner_fix.last_time = millis()/1000.0;
        distance = distanceEarth(gps_fix.lat, gps_fix.lng, partner_fix.info.lat, partner_fix.info.lng);
        bearing = getBearing(gps_fix.lat, gps_fix.lng, partner_fix.info.lat, partner_fix.info.lng);
        last_scan = millis();
        //rssi = "RSSI " + String(LoRa.packetRssi(), DEC) ;
        //Serial.println(rssi);
      }
  }

  if (millis() - last_screen > 500){
    display.setCursor(0, 0);     // Start at top-left corner
    display.clearDisplay();

    display.print("Sats:");
    display.print(gps_fix.sats);
    display.print(",");
    display.println( partner_fix.info.sats);

    display.print("dT(s):");
    display.println(millis()/1000.0 - partner_fix.last_time,1);

    if(partner_fix.info.sats>2){
      display.print("D(m):");
      display.println(distance, 1);
    }

    display.display();
    interval = random(500);
    last_screen = millis();
  }
}

/*
Serial.print("Latitude  : ");
Serial.println(gps.location.lat(), 5);
Serial.print("Longitude : ");
Serial.println(gps.location.lng(), 4);
Serial.print("Satellites: ");
Serial.println(gps.satellites.value());
Serial.print("Altitude  : ");
Serial.print(gps.altitude.feet() / 3.2808);
Serial.println("M");
Serial.print("Time      : ");
Serial.print(gps.time.hour());
Serial.print(":");
Serial.print(gps.time.minute());
Serial.print(":");
Serial.println(gps.time.second());// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Serial.print("Speed     : ");
Serial.println(gps.speed.kmph());
Serial.println("**********************");
*/