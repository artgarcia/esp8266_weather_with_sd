/**
   03/07/2017
   Arthur A Garcia
   This project will connect an ESP8266 to a display and DHT11 Temperature sensor
   The data will then be sent to Azure and an alert will be generated
   DEVICE : Node MCU ESP8266 (0.9)
          : SD card reader Micro SD adaptor

   3-10-17 : added url and device id to sd card reader
   4-16-07 : cleaned up code and moved sendtodisplay function th common
           : removed host and sas token from code. made data inputs from sd card
*/



// include wifi library for nodemcu 8266
#include <ESP8266WiFi.h>

// need this lib for Secure SSL for ESP 8266 chip
#include <WiFiClientSecure.h>  

// include SD library
#include <SD.h>

// library for BME 280 pressure, temperature and humidity sensor
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// Include the correct display library
// For a connection via I2C using Wire include
#include <SPI.h>

// display adapter
#include "SSD1306.h"

// common include file with additional user functions ise 
// To use tabs with a .h extension, you need to #include it (using "double quotes" not <angle brackets>).     
#include "esp8266_common.h"                 

String netid, pwd, deviceId, url, host, sas;
long duration, distance, lastDistance;
int waittime;
String passData[10];

// http://patriot-geek.blogspot.com/2017/03/esp8266-bme280-and-oled-displays.html
// The ESP8266 has exactly one I2C bus, so the two devices (the BME280 and the OLED display) must be on that one I2C bus!
// see fritz diagram. bme280 and lcd are wired to same pins. d1 & d2
#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme; // I2C


void setup() {

  Serial.begin(9600);
  Serial.println("");

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  // init done
  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  // stup for display
  display.init();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

  sendToDisplay(0, 0, "Screen Init");
  delay(2000);
  
  sendToDisplay(0,15,"Init SD Card");
  
  // get data from sd card
  // passing an array to house sd card information
  getSDData(passData);
 
  // move sd card data to global variables
  netid = passData[0];
  pwd = passData[1];
  deviceId = passData[2];
  url = passData[3];
  waittime = (int)passData[6].toInt();

  display.clear();
  sendToDisplay(0, 15, "SSID:" + netid);
  sendToDisplay(0, 30, "pwd :" + pwd);
  sendToDisplay(0, 45, "Device Id:" + deviceId);
  delay(2000);

  // endpoint to use to send message /devices/{device name}/messages/events?api-version=2016-02-03
  // host =  address for your Azure IoT Hub
  // sas  =  sas authorization token from below
  //
  // on device monitor generate a sas token on config page.
  //String uri = "/devices/esp8266v2/messages/events?api-version=2016-02-03";
  host = passData[4];
  sas = passData[5];

  // replace device id in url 
  url.replace("{0}", deviceId);
  
  // initialize wifi
  WiFi.disconnect();
  WiFi.begin( (const char*)netid.c_str() , (const char*)pwd.c_str() );

  display.clear();
  Serial.print("Connecting to SSID:");
  Serial.println(WiFi.SSID());
  Serial.println(WiFi.macAddress() );
  
  sendToDisplay(0, 0, "Connecting:" + netid);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
	
	switch (WiFi.status())	
	{
	case WL_CONNECTION_LOST:
		Serial.println("Connection Lost");
		break;
	case WL_CONNECT_FAILED:
		Serial.println("Connection Failed");
		break;
	case WL_DISCONNECTED:
		Serial.println(" Not Connected");
		break;
	default:
		Serial.print("Status:");
		Serial.println(WiFi.status());
		break;
	}

    sendToDisplay(0,15,"...");

  }

  // confirm connection to WiFi
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  display.clear();
  sendToDisplay(0,0,"Connected:" + netid);
  sendToDisplay(0, 15, "IP:" + WiFi.localIP().toString());
  delay(1000);

  // start temp sensor
  bool status;
  status = bme.begin();
  if (!status) {
	  Serial.println("Could not find a valid BME280 sensor, check wiring!");
	  sendToDisplay(0, 30, "BME 280 Error");
	  while (1);
  }

  sendToDisplay(0, 30, "bme280  Begin");

  // start time client - used to get curren time.
  timeClient.begin();
  sendToDisplay(0, 45, "TimeClient Begin");
  delay(1000);

}


void loop() {

  Serial.println("");
  Serial.println("Loop");

  timeClient.update();
  String key = (String)timeClient.getEpochTime();
  Serial.println(key);

  display.clear();
  sendToDisplay(0, 0, "Temp from:" + deviceId);

  float t = bme.readTemperature();
  // fahrenheit = t * 1.8 + 32.0;
  float temp = t *1.8 + 32;
  delay(200);
  sendToDisplay(0, 15, "Temperature : " + String(temp));

  float pressure = bme.readAltitude(SEALEVELPRESSURE_HPA);
  sendToDisplay(0, 30, "Pressure : " + String(pressure) );

  Serial.print("Approx. Altitude = ");
  Serial.print(pressure);

  float humidity = bme.readHumidity();
  delay(200);
  sendToDisplay(0, 45, "Humidity : " + String(humidity) + "%" );

  Serial.print("Temperature:");
  Serial.println(temp);

  Serial.print("Humidity:");
  Serial.println(humidity);
  delay(1000);

  // format data into Json object to pass to Azure
  //String myKeys[] = { "deviceId","Temp","Humidity","KeyValue" };
  String tempJson = createJsonData(deviceId, temp, humidity,pressure,key);
 
  // send json to Azure
  httpRequest("POST", url, host, sas, "application/atom+xml;type=entry;charset=utf-8", tempJson);
  delay(1000);

  sendToDisplay(0,30, 150,tempJson);
  Serial.print("Json Sent:");
  Serial.println(tempJson);
  
  display.clear();
  sendToDisplay(0, 0, "Temp from:" + deviceId);
  sendToDisplay(0, 30, "Waiting MS :" + passData[6]);
  delay(waittime);   // delay 60000 miliseconds = 60 sec

}






