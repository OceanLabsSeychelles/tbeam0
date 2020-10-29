#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <TinyGPS++.h>
#include <axp20x.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Tone32.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

#define OLED_RESET     16 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCK     5    // GPIO5  -- SX1278's SCK
#define MISO    19   // GPIO19 -- SX1278's MISnO
#define MOSI    27   // GPIO27 -- SX1278's MOSI
#define SS      18   // GPIO18 -- SX1278's CS
#define RST     14   // GPIO14 -- SX1278's RESET
#define DI0     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)
#define BATTERY_PIN 35 // battery level measurement pin, here is the voltage divider connected
#define USER_BUTTON 38
#define BUZZER_PIN  13
#define BUZZER_CHANNEL 0

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define BAND  868E6

TinyGPSPlus gps;
HardwareSerial GPS(1);
AXP20X_Class axp;
Adafruit_BNO055 bno = Adafruit_BNO055(55);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

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

typedef struct{
    char id[40];
    int distmax;
    int downmax;
    int buddylock;
} PAIRING_DATA;

PAIRING_DATA local_config;
uint8_t* local_config_ptr = (uint8_t*)&local_config;

PAIRING_DATA partner_config;

volatile bool server_on = true;
volatile bool lora_scan = false;
volatile bool buddy_found = false;
volatile int distmax;
volatile int downmax;
volatile int buddylock;

bool displayInit(){
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
    return false;

  }else{
    display.clearDisplay();
    display.setTextSize(2);      // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.cp437(true);         // Use full 256 char 'Code Page 437' font
    display.setCursor(0, 0);     // Start at top-left corner
    display.println("Dive-Alive");
    display.display();
    return true;
  }
};

bool powerOn(){
  if (!axp.begin(Wire, AXP192_SLAVE_ADDRESS)) {
    axp.setPowerOutPut(AXP192_LDO2, AXP202_ON); // LORA radio
    axp.setPowerOutPut(AXP192_LDO3, AXP202_ON); // GPS main power
    axp.setPowerOutPut(AXP192_DCDC2, AXP202_ON);
    axp.setPowerOutPut(AXP192_EXTEN, AXP202_ON);
    axp.setPowerOutPut(AXP192_DCDC1, AXP202_ON); //OLED
    axp.setDCDC1Voltage(3300); // for the OLED power
    return true;
  } else {   
    return false;
  }
}

float getBatteryVoltage() {
  // we've set 10-bit ADC resolution 2^10=1024 and voltage divider makes it half of maximum readable value (which is 3.3V)
  float sum = 0;
  for (int j = 0; j< 10; j++){
    sum += (float) analogRead(BATTERY_PIN);
  }
  sum /= 10;
  return (sum); 
}
