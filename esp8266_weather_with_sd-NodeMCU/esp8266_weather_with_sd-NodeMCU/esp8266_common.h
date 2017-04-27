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
WiFiClientSecure client;

// azure iot client
AzureIoTHubClient iotHubClient;

// You can specify the time server pool and the offset (in seconds, can be
// changed later with setTimeOffset() ). Additionaly you can specify the
// update interval (in milliseconds, can be changed using setUpdateInterval() ).
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

static bool messagePending = false;
static bool messageSending = false;

const char *onSuccess = "\"Successfully invoke device method\"";
const char *notFound = "\"No method found\"";

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

//
// MQTT 
// https://github.com/Azure-Samples/iot-hub-feather-huzzah-client-app
//
static void sendCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *userContextCallback)
{
	if (IOTHUB_CLIENT_CONFIRMATION_OK == result)
	{
		LogInfo("Message sent to Azure IoT Hub");
	}
	else
	{
		LogInfo("Failed to send message to Azure IoT Hub");
	}
	messagePending = false;
}


static void sendMessage(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, char *buffer, bool temperatureAlert)
{
	IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromByteArray((const unsigned char *)buffer, strlen(buffer));
	if (messageHandle == NULL)
	{
		LogInfo("unable to create a new IoTHubMessage");
	}
	else
	{
		MAP_HANDLE properties = IoTHubMessage_Properties(messageHandle);
		Map_Add(properties, "temperatureAlert", temperatureAlert ? "true" : "false");
		LogInfo("Sending message: %s", buffer);
		if (IoTHubClient_LL_SendEventAsync(iotHubClientHandle, messageHandle, sendCallback, NULL) != IOTHUB_CLIENT_OK)
		{
			LogInfo("Failed to hand over the message to IoTHubClient");
		}
		else
		{
			messagePending = true;
			LogInfo("IoTHubClient accepted the message for delivery");
		}

		IoTHubMessage_Destroy(messageHandle);
	}
}


IOTHUBMESSAGE_DISPOSITION_RESULT receiveMessageCallback(IOTHUB_MESSAGE_HANDLE message, void *userContextCallback)
{
	IOTHUBMESSAGE_DISPOSITION_RESULT result;
	const unsigned char *buffer;
	size_t size;
	if (IoTHubMessage_GetByteArray(message, &buffer, &size) != IOTHUB_MESSAGE_OK)
	{
		LogInfo("unable to IoTHubMessage_GetByteArray");
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
		LogInfo("Receive C2D message: %s", temp);
		free(temp);
	}
	return IOTHUBMESSAGE_ACCEPTED;
}


void start()
{
	LogInfo("Start sending temperature and humidity data");
	messageSending = true;
}

void stop()
{
	LogInfo("Stop sending temperature and humidity data");
	messageSending = false;
}

int deviceMethodCallback(const char *methodName,const unsigned char *payload,size_t size,unsigned char **response,size_t *response_size,void *userContextCallback)
{
	LogInfo("Try to invoke method %s", methodName);
	const char *responseMessage = onSuccess;
	int result = 200;

	if (strcmp(methodName, "start") == 0)
	{
		start();
	}
	else if (strcmp(methodName, "stop") == 0)
	{
		stop();
	}
	else
	{
		LogInfo("No method %s found", methodName);
		responseMessage = notFound;
		result = 404;
	}

	*response_size = strlen(responseMessage);
	*response = (unsigned char *)malloc(*response_size);
	strncpy((char *)(*response), responseMessage, *response_size);

	return result;
}

void twinCallback(DEVICE_TWIN_UPDATE_STATE updateState,const unsigned char *payLoad,size_t size,void *userContextCallback)
{
	char *temp = (char *)malloc(size + 1);
	for (int i = 0; i < size; i++)
	{
		temp[i] = (char)(payLoad[i]);
	}
	temp[size] = '\0';
	//parseTwinMessage(temp);
	free(temp);
}

void parseTwinMessage(char *message)
{
	StaticJsonBuffer<256> jsonBuffer;
	JsonObject &root = jsonBuffer.parseObject(message);
	if (!root.success())
	{
		LogError("parse %s failed", message);
		return;
	}

	if (root["desired"]["interval"].success())
	{
		//interval = root["desired"]["interval"];
	}
	else if (root.containsKey("interval"))
	{
		//interval = root["interval"];
	}
}
