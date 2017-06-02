/*
	Header file for Node MCU
*/

#include <AzureIoTHub.h>
#include <AzureIoTProtocol_MQTT.h>
#include <AzureIoTUtility.h>

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
WiFiClientSecure sslClient;

// azure iot client
static AzureIoTHubClient iotHubClient;


// Interval time(ms) for sending message to IoT Hub
static int interval = 2000;

#define MESSAGE_MAX_LEN 256

// You can specify the time server pool and the offset (in seconds, can be
// changed later with setTimeOffset() ). Additionaly you can specify the
// update interval (in milliseconds, can be changed using setUpdateInterval() ).
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

const char *onSuccess = "\"Successfully invoke device method\"";
const char *notFound = "\"No method found\"";

// declare functions
String createJsonData(String devId, float temp ,float humidity,String keyid);
void sendToDisplay(int col, int row, String data);
void sendToDisplay(int col, int row, int len, String data);


void initIoThubClient()
{
	iotHubClient.begin(sslClient);
}

String createJsonData(String devId, float temp, float humidity,String keyid)
{

  // create json object
  StaticJsonBuffer<200> jsonBuffer;
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

void createJsonBuffer(String devId, float temp, float humidity, String keyid , char *payload)
{

	// create json object
	StaticJsonBuffer<MESSAGE_MAX_LEN> jsonBuffer;
	char* buff;
	String outdata;

	JsonObject& root = jsonBuffer.createObject();
	root["DeviceId"] = devId;
	root["KeyId"] = keyid;
	root["temperature"] = temp;
	root["humidity"] = humidity;

	// convert to string
	root.printTo(payload, MESSAGE_MAX_LEN);

}

void WifiConnect(String netid, String pwd)
{

	// initialize wifi
	WiFi.disconnect();
	WiFi.begin((const char*)netid.c_str(), (const char*)pwd.c_str());

	display.clear();
	Serial.print("Connecting to SSID:");
	Serial.println(WiFi.SSID());
	Serial.println(WiFi.macAddress());

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

		sendToDisplay(0, 15, "...");

	}

	// confirm connection to WiFi
	Serial.println("WiFi connected");
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());

	display.clear();
	sendToDisplay(0, 0, "Connected:" + netid);
	sendToDisplay(0, 15, "IP:" + WiFi.localIP().toString());
	delay(1000);
}

void httpRequest(String verb, String uri, String host, String sas, String contentType, String content)
{
	Serial.println("--- Start Process --- ");
	if (verb.equals("")) return;
	if (uri.equals("")) return;

	// close any connection before send a new request.
	// This will free the socket on the WiFi shield
	sslClient.stop();

	// if there's a successful connection:
	if (sslClient.connect((const char*)host.c_str(), 443)) {
		Serial.println("--- We are Connected --- ");
		Serial.print("*** Sending Data To:  ");
		Serial.println(host + uri);

		Serial.print("*** Data To Send:  ");
		Serial.println(content);

		sslClient.print(verb); //send POST, GET or DELETE
		sslClient.print(" ");
		sslClient.print(uri);  // any of the URI
		sslClient.println(" HTTP/1.1");

		sslClient.print("Host: ");
		sslClient.println((const char*)host.c_str());  //with hostname header

		sslClient.print("Authorization: ");
		sslClient.println((const char*)sas.c_str());  //Authorization SAS token obtained from Azure IoT device explorer

		if (verb.equals("POST"))
		{
			Serial.println("--- Sending POST ----");
			sslClient.print("Content-Type: ");
			sslClient.println(contentType);
			sslClient.print("Content-Length: ");
			sslClient.println(content.length());
			sslClient.println();
			sslClient.println(content);
		}
		else
		{
			Serial.println("--- sslClient status --- ");
			Serial.println(sslClient.status());

			sslClient.println();

		}
	}

	while (sslClient.available()) {
		char c = sslClient.read();
		Serial.write(c);
	}

	Serial.println("--- Send complete ----");
}

void getSDData(String *passData)
{
	String str, netid, pwd, deviceId, url, hostname, sas, timedelay;

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
			if (dataFile.find("DELAY:"))
			{
				str = dataFile.readStringUntil('|');
				timedelay = str;
				Serial.println(timedelay);
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
	passData[6] = timedelay;
	 
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

#pragma region mqtt

//
// MQTT 
// https://github.com/Azure-Samples/iot-hub-feather-huzzah-client-app
//
static void sendCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *userContextCallback)
{
	Serial.println("in sendCallback");

	if (IOTHUB_CLIENT_CONFIRMATION_OK == result)
	{
		Serial.println("Message sent to Azure IoT Hub");
	}
	else
	{
		Serial.println("Failed to send message to Azure IoT Hub");
	}

}

static void sendMessage(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, char *buffer, bool temperatureAlert)
{
	Serial.println("in sendmessage");

	IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromByteArray((const unsigned char *)buffer, strlen(buffer));
	if (messageHandle == NULL)
	{
		Serial.println("unable to create a new IoTHubMessage");
	}
	else
	{
		MAP_HANDLE properties = IoTHubMessage_Properties(messageHandle);
		Map_Add(properties, "temperatureAlert", temperatureAlert ? "true" : "false");

		Serial.printf("Sending message: %s \n\r", buffer);
		if (IoTHubClient_LL_SendEventAsync(iotHubClientHandle, messageHandle, sendCallback, NULL) != IOTHUB_CLIENT_OK)
		{
			Serial.println("Failed to hand over the message to IoTHubClient");
		}
		else
		{
			Serial.println("IoTHubClient accepted the message for delivery");
		}
		IoTHubMessage_Destroy(messageHandle);
	}
}

IOTHUBMESSAGE_DISPOSITION_RESULT receiveMessageCallback(IOTHUB_MESSAGE_HANDLE message, void *userContextCallback)
{
	Serial.println("in recieve callback");

	IOTHUBMESSAGE_DISPOSITION_RESULT result;
	const unsigned char *buffer;
	size_t size;

	if (IoTHubMessage_GetByteArray(message, &buffer, &size) != IOTHUB_MESSAGE_OK)
	{
		Serial.println("unable to IoTHubMessage_GetByteArray");
		result = IOTHUBMESSAGE_REJECTED;
	}
	else
	{
		/*buffer is not zero terminated*/
		char *temp = (char *)malloc(size + 1);
		if (temp == NULL)
		{
			return IOTHUBMESSAGE_ABANDONED;
		}

		strncpy(temp, (const char *)buffer, size);
		temp[size] = '\0';
		Serial.printf("Receive C2D message: %s", temp);
		free(temp);
	}
	return IOTHUBMESSAGE_ACCEPTED;
}

int deviceMethodCallback(const char *methodName,const unsigned char *payload,size_t size,unsigned char **response,size_t *response_size,void *userContextCallback)
{
	Serial.printf("Try to invoke method %s", methodName);
	const char *responseMessage = onSuccess;
	int result = 200;
		
	if (strcmp(methodName, "start") == 0)
	{
	//	start();
	}
	else if (strcmp(methodName, "stop") == 0)
	{
	//	stop();
	}
	else
	{
		Serial.printf("No method %s found", methodName);
		responseMessage = notFound;
		result = 404;
	}

	*response_size = strlen(responseMessage);
	*response = (unsigned char *)malloc(*response_size);
	strncpy((char *)(*response), responseMessage, *response_size);
	return result;
}

void parseTwinMessage(char *message)
{
	Serial.println("in parseTwinMessage");

	StaticJsonBuffer<MESSAGE_MAX_LEN> jsonBuffer;
	JsonObject &root = jsonBuffer.parseObject(message);
	if (!root.success())
	{
		LogError("parse %s failed", message);
		return;
	}

	if (root["desired"]["interval"].success())
	{
		interval = root["desired"]["interval"];
	}
	else if (root.containsKey("interval"))
	{
		interval = root["interval"];
	}
}

void twinCallback(DEVICE_TWIN_UPDATE_STATE updateState,const unsigned char *payLoad,size_t size,void *userContextCallback)
{
	char *temp = (char *)malloc(size + 1);
	for (int i = 0; i < size; i++)
	{
		temp[i] = (char)(payLoad[i]);
	}
	temp[size] = '\0';
	parseTwinMessage(temp);
	free(temp);
}

#pragma endregion
