/*
	Header file for Node MCU
*/

#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

// include SD library
#include <SD.h>

// need this lib for Secure SSL for ESP 8266 chip
#include <WiFiClientSecure.h>  

// Include the correct display library
// For a connection via I2C using Wire include
#include <SPI.h>

// http://easycoding.tn/tuniot/demos/code/
// D2 -> SDA
// D1 -> SCL      display( address of display, SDA,SCL)
#include "SSD1306.h"
SSD1306  display(0x3C, 4, 5);

WiFiUDP ntpUDP;

// setup https client for 8266 SSL client
WiFiClientSecure client;

// You can specify the time server pool and the offset (in seconds, can be
// changed later with setTimeOffset() ). Additionaly you can specify the
// update interval (in milliseconds, can be changed using setUpdateInterval() ).
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);
  
// declare functions
String createJsonData(String devId, float temp ,float humidity,String keyid);
void sendToDisplay(int col, int row, String data);
void sendToDisplay(int col, int row, int len, String data);

String createJsonData(String devId, float temp, float humidity,String keyid)
{

  // create json object
  StaticJsonBuffer<200> jsonBuffer;
  char* buff;
  String outdata;

  JsonObject& root = jsonBuffer.createObject();
  root["DeviceId"] = devId;
  root["KeyId"] = keyid;
  root["temperature"] = temp;
  root["humidity"] = humidity;

  // convert to string
  root.printTo(outdata);
  return outdata;

}

void httpRequest(String verb, String uri, String host, String sas, String contentType, String content)
{
	Serial.println("--- Start Process --- ");
	if (verb.equals("")) return;
	if (uri.equals("")) return;

	// close any connection before send a new request.
	// This will free the socket on the WiFi shield
	client.stop();

	// if there's a successful connection:
	if (client.connect((const char*)host.c_str(), 443)) {
		Serial.println("--- We are Connected --- ");
		Serial.print("*** Sending Data To:  ");
		Serial.println(host + uri);

		Serial.print("*** Data To Send:  ");
		Serial.println(content);

		client.print(verb); //send POST, GET or DELETE
		client.print(" ");
		client.print(uri);  // any of the URI
		client.println(" HTTP/1.1");

		client.print("Host: ");
		client.println((const char*)host.c_str());  //with hostname header

		client.print("Authorization: ");
		client.println((const char*)sas.c_str());  //Authorization SAS token obtained from Azure IoT device explorer

		if (verb.equals("POST"))
		{
			Serial.println("--- Sending POST ----");
			client.print("Content-Type: ");
			client.println(contentType);
			client.print("Content-Length: ");
			client.println(content.length());
			client.println();
			client.println(content);
		}
		else
		{
			Serial.println("--- client status --- ");
			Serial.println(client.status());

			client.println();

		}
	}

	while (client.available()) {
		char c = client.read();
		Serial.write(c);
	}

	Serial.println("--- Send complete ----");
}

void getSDData(String *passData)
{
	String str, netid, pwd, deviceId, url, hostname, sas;

	File dataFile;
	Serial.println("In getSDData");

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
	int index = 0;

	if (dataFile)
	{
		Serial.println("data from sd card");
		display.clear();
		sendToDisplay(0, 0, "Data From Card");

		while (dataFile.available())
		{
			if (dataFile.find("SSID:"))
			{
				str = dataFile.readStringUntil('|');
				netid = str;
				Serial.println(netid);
				sendToDisplay(0, 15, netid);
			}
			if (dataFile.find("PASSWORD:"))
			{
				str = dataFile.readStringUntil('|');
				pwd = str;
				Serial.println(pwd);
				//sendToDisplay(0,30, pwd);
			}
			if (dataFile.find("DEVICEID:"))
			{
				str = dataFile.readStringUntil('|');
				deviceId = str;
				Serial.println(deviceId);
				sendToDisplay(0,30, deviceId);
			}
			if (dataFile.find("URL:"))
			{
				str = dataFile.readStringUntil('|');
				url = str;
				Serial.println(url);
				//sendToDisplay(50, 30, url);
			}
			if (dataFile.find("HOSTNAME:"))
			{
				str = dataFile.readStringUntil('|');
				hostname = str;
				Serial.println(hostname);
				sendToDisplay(0, 45, hostname);
			}
			if (dataFile.find("SAS:"))
			{
				str = dataFile.readStringUntil('|');
				sas = str;
				Serial.println(sas);
			}
		}
		// close the file
		dataFile.close();
	}

	passData[0] = netid;
	passData[1] = pwd;
	passData[2] = deviceId;
	passData[3] = url;
	passData[4] = hostname;
	passData[5] = sas;
	 
}

void sendToDisplay(int col, int row, String data)
{
	display.drawString(col, row, data);
	display.display();
}

void sendToDisplay(int col, int row,int len, String data)
{
	display.drawStringMaxWidth(col, row,len, data);
	display.display();
}

