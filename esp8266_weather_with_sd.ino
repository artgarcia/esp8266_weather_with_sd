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
   4-22-17 : moved libraries to common.h
*/

// include wifi library for nodemcu 8266
#include <ESP8266WiFi.h>

// common include file with additional user functions ise 
// To use tabs with a .h extension, you need to #include it (using "double quotes" not <angle brackets>).     
#include "esp8266_common.h"                 

// for dht11 temp sensor on esp8266 chip
#include <DHT.h>
#define DHTPIN 2 // D4 on nodemcu = gpio 2 
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

String netid, pwd, deviceId, url, host, sas;
long duration, distance, lastDistance;
String passData[6];

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

  // verify variables from sd card got into globals
  Serial.print("NETID:");
  Serial.println(netid);
  Serial.print("PWD:");
  Serial.println(pwd);
  Serial.print("DEVICEID:");
  Serial.println(deviceId);
  Serial.print("URL:");
  Serial.println(url);

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
  dht.begin();
  sendToDisplay(0, 30, "Sensor Begin");

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

  float t = dht.readTemperature();
  // fahrenheit = t * 1.8 + 32.0;
  float temp = t *1.8 + 32;
  delay(200);
  sendToDisplay(0, 15, "Temp:" + String(temp));

  float humidity = dht.readHumidity();
  delay(200);
  sendToDisplay(60, 15, "Hum:" + String(humidity) + "%" );

  Serial.print("Temperature:");
  Serial.println(temp);

  Serial.print("Humidity:");
  Serial.println(humidity);
  
  // format data into Json object to pass to Azure
  //String myKeys[] = { "deviceId","Temp","Humidity","KeyValue" };
  String tempJson = createJsonData(deviceId, temp, humidity,key);
 
  // send json to Azure
  httpRequest("POST", url, host, sas, "application/atom+xml;type=entry;charset=utf-8", tempJson);

  sendToDisplay(0,30, 150,tempJson);
  Serial.print("Json Sent:");
  Serial.println(tempJson);
  
   delay(6000);   // delay 60000 miliseconds = 60 sec

}






