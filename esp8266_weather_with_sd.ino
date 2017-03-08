/**
   03/07/2017
   Arthur A Garcia
   This project will connect an ESP8266 to a display and DHT11 Temperature sensor
   The data will then be sent to Azure and an alert will be generated
   DEVICE : Node MCU ESP8266 (0.9)
*/


// include wifi library for nodemcu 8266
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`
#include <ESP8266WiFi.h>
// need this lib for Secure SSL for ESP 8266 chip
#include <WiFiClientSecure.h>  

// json library
#include <ArduinoJson.h>

// include SD library
#include <SD.h>

// Include the correct display library
// For a connection via I2C using Wire include
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//
// D2 -> SDA
// D1 -> SCL
#define OLED_RESET LED_BUILTIN //4
Adafruit_SSD1306 display(OLED_RESET);

// common include file with additional user functions ise 
// To use tabs with a .h extension, you need to #include it (using "double quotes" not <angle brackets>).     
#include "esp8266_common.h"                 

// for dht11 temp sensor on esp8266 chip
#include <DHT.h>
#define DHTPIN 2  //D4 on nodemcu
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

File dataFile;
 
String str,netid,pwd;
long duration, distance, lastDistance;


void setup() {

  Serial.begin(9600);
  Serial.println("");

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  // init done
  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  display.display();
  delay(2000);
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  sendToDisplay(0,0,"Init SD Card");
  
   // initialize sd card 
   // for nodemcu use begin() else use begin(4)
    Serial.print("Initializing SD card...");
    if (!SD.begin()) {
      Serial.println("initialization failed!");
      return;
    }
    Serial.println("initialization done.");
  
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  dataFile = SD.open("wifiFile.txt");
  int index=0;
  
  if(dataFile)
  {
  Serial.println("data from sd card");
    while(dataFile.available())
    {
      if (dataFile.find("SSID:"))
      {
        str = dataFile.readStringUntil('|');
        netid = str;
        Serial.println(netid);
        sendToDisplay(0,15,netid);
      }
      if (dataFile.find("PASSWORD:"))
      {
        str = dataFile.readStringUntil('|');
        pwd = str;
        Serial.println(pwd);
        sendToDisplay(0,30,pwd);
      }
    }
    // close file
    dataFile.close();
  }
  
  // initialize wifi
  WiFi.begin( (const char*)netid.c_str() , (const char*)pwd.c_str() );
  Serial.print("SSID:");
  Serial.println(WiFi.SSID());
  Serial.println(WiFi.macAddress() );
  
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    display.print("...");
    display.display();
  }

  // confirm connection to WiFi
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  display.clearDisplay();
  sendToDisplay(0,0,"Connected:");
  display.println( netid );
  display.display();
  
  // start temp sensor
  dht.begin();
  
  // start time client - used to get curren time.
  timeClient.begin();

}



void loop() {

  timeClient.update();
  Serial.println(timeClient.getEpochTime());

  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(1);
  display.println( "Temperature esp8266v2");

  float humidity = dht.readHumidity();
  delay(200);
  float t = dht.readTemperature();
  // fahrenheit = t * 1.8 + 32.0;
  float temp = t *1.8 + 32;
  delay(200);

 
  display.setCursor(0,18);
  display.print( "Temp:");
  display.println(String(temp));
  display.display();

  display.print( "Humidity:");
  display.print( String(humidity));
  display.println( "%");
  display.display();

  Serial.print("Temp:");
  Serial.println(temp);

  Serial.print("Humidity:");
  Serial.println(humidity);

  String key = (String)timeClient.getEpochTime();
  
  // format data into Json object to pass to Azure
  String tempJson = createJsonData("esp8266v2", temp, humidity,key);
 
  // send json to Azure
  httpRequest("POST", uri, "application/atom+xml;type=entry;charset=utf-8", tempJson);
  
  display.println();
  display.print("Json Sent:");
  display.println(tempJson);
  display.display();
  
  //delay(60000);   // delay 60000 miliseconds = 60 sec
   delay(20000);
}

void sendToDisplay( int col,int row, String data)
{
    display.setCursor(col,row);
    display.print(data);
    display.display();
}


