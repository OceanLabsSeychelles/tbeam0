#include "main.h"
#include "html.h"
#include "geometry.h"

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


void setup()
{
  pinMode(BUZZER_PIN, OUTPUT);
  tone(BUZZER_PIN, NOTE_A6, 125, BUZZER_CHANNEL);
  
  Serial.begin(115200);
  Wire.begin(21, 22);
  
  if(!powerOn()){
    Serial.println("AXP192 Begin FAIL");
    for(;;); // Don't proceed, loop forever
  }else{
    Serial.println("Power Ok.");
  }

  if(!displayInit()){
    Serial.println(F("SSD1306 allocation failed"));
  }else{
    Serial.println("OLED Ok.");
    delay(1000);
  }

  GPS.begin(9600, SERIAL_8N1, 34, 12);   //17-TX 18-RX
  SPI.begin(SCK,MISO,MOSI,SS);
  LoRa.setPins(SS,RST,DI0);
  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  display.setCursor(0, 0);     // Start at top-left corner
  display.clearDisplay();
  display.println("Hardware");
  display.print("Init OK.");
  display.display();
  delay(1000);

  display.setCursor(0, 0);     // Start at top-left corner
  display.clearDisplay();
  display.println("Starting");
  display.print("WiFi");
  display.display();

  if(!serverInit()){
    Serial.println("Error starting server, likely SPIFFS");
  }else{
    Serial.println("Webserver Ok");
    tone(BUZZER_PIN, NOTE_C7, 250, BUZZER_CHANNEL);
  }

  display.setCursor(0, 0);     // Start at top-left corner
  display.clearDisplay();
  display.println("WiFi OK");
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.println(WiFi.localIP());
  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.display();
  delay(2000);




  /*
  if(!bno.begin())
  {
    Serial.print("Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
    while(1);
  }
  while(1){
	  sensors_event_t event; 
	  bno.getEvent(&event);
	  
	  display.setCursor(0, 0);     // Start at top-left corner
	  display.clearDisplay();
	  display.print("\tX: ");
	  display.println(event.orientation.x, 2);
	  display.print("\tY: ");
	  display.println(event.orientation.y, 2);
	  display.print("\tZ: ");
	  display.println(event.orientation.z, 2);
	  display.display();
  }
  */
  
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

      for(int j =0; j< packetSize; j++){
        LoRa.read();
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