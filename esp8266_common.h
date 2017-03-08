#include <NTPClient.h>
#include <WiFiUdp.h>

WiFiUDP ntpUDP;

// setup https client for 8266 SSL client
WiFiClientSecure client;

// endpoint to use to send message /devices/{device name}/messages/events?api-version=2016-02-03
// host name address for your Azure IoT Hub
// on device monitor generate a sas token on config page.
String uri = "/devices/esp8266v1/messages/events?api-version=2016-02-03";
char hostnme[] = "ArtTempIOT.azure-devices.net";
char authSAS[] = "SharedAccessSignature sr=ArtTempIOT.azure-devices.net&sig=vmUF6p3IANfHmNWrvk4Zf%2BlpngD365hUX9f%2FB2zNaUM%3D&se=1515799811&skn=iothubowner";

// You can specify the time server pool and the offset (in seconds, can be
// changed later with setTimeOffset() ). Additionaly you can specify the
// update interval (in milliseconds, can be changed using setUpdateInterval() ).
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);
  
// declare json functions
String createJsonData(String devId, float temp ,float humidity,String keyid);


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


void httpRequest(String verb, String uri, String contentType, String content)
{
	Serial.println("--- Start Process --- ");
	if (verb.equals("")) return;
	if (uri.equals("")) return;

	// close any connection before send a new request.
	// This will free the socket on the WiFi shield
	client.stop();

	// if there's a successful connection:
	if (client.connect(hostnme, 443)) {
		Serial.println("--- We are Connected --- ");
		Serial.print("*** Sending Data To:  ");
		Serial.println(hostnme + uri);

		Serial.print("*** Data To Send:  ");
		Serial.println(content);

		client.print(verb); //send POST, GET or DELETE
		client.print(" ");
		client.print(uri);  // any of the URI
		client.println(" HTTP/1.1");
		client.print("Host: ");
		client.println(hostnme);  //with hostname header
		client.print("Authorization: ");
		client.println(authSAS);  //Authorization SAS token obtained from Azure IoT device explorer
								  //client.println("Connection: close");

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




