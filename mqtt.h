#pragma once

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>

#include <AzureIoTHub.h>
#include <AzureIoTProtocol_MQTT.h>
#include <AzureIoTUtility.h>


static bool messagePending = false;
static bool messageSending = true;

static AzureIoTHubClient iotHubClient;

void initIoThubClient()
{
	iotHubClient.begin(client);
}


static void sendCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *userContextCallback)
{
	if (IOTHUB_CLIENT_CONFIRMATION_OK == result)
	{
		Serial.println("Message Sent to Azure IOT Hub");
	}
	else
	{
		Serial.println("Error Sending Message Sent to Azure IOT Hub");
	}
	messagePending = false;
}


static void sendMessage(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const unsigned char* buffer, size_t size)
{
	Serial.println("in Sendmessage");

	static unsigned int messageTrackingId;
	IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromByteArray(buffer, size);
	if (messageHandle == NULL)
	{
		Serial.println("unable to create a new IoTHubMessage\r\n");
	}
	else
	{
		if (IoTHubClient_LL_SendEventAsync(iotHubClientHandle, messageHandle, sendCallback, (void*)(uintptr_t)messageTrackingId) != IOTHUB_CLIENT_OK)
		{
			Serial.println("failed to hand over the message to IoTHubClient");
		}
		else
		{
			Serial.println("IoTHubClient accepted the message for delivery\r\n");
		}
		IoTHubMessage_Destroy(messageHandle);
	}
	messageTrackingId++;
}


/*this function "links" IoTHub to the serialization library*/
static IOTHUBMESSAGE_DISPOSITION_RESULT IoTHubMessage(IOTHUB_MESSAGE_HANDLE message, void* userContextCallback)
{
	Serial.println("in IoTHubMessage ");

	IOTHUBMESSAGE_DISPOSITION_RESULT result;
	const unsigned char* buffer;
	size_t size;
	if (IoTHubMessage_GetByteArray(message, &buffer, &size) != IOTHUB_MESSAGE_OK)
	{
		printf(" message not ok in IoTHubMessage ");
		printf("unable to IoTHubMessage_GetByteArray\r\n");
		result = IOTHUBMESSAGE_ABANDONED;
	}
	else
	{
		printf(" message ok in IoTHubMessage ");

		printf(buffer);

		/*buffer is not zero terminated*/
		char* temp = malloc(size + 1);
		if (temp == NULL)
		{
			printf("failed to malloc\r\n");
			result = IOTHUBMESSAGE_ABANDONED;
		}
		else
		{
			printf(" message execute IoTHubMessage ");

			(void)memcpy(temp, buffer, size);
			temp[size] = '\0';
			EXECUTE_COMMAND_RESULT executeCommandResult = EXECUTE_COMMAND(userContextCallback, temp);
			result =
				(executeCommandResult == EXECUTE_COMMAND_ERROR) ? IOTHUBMESSAGE_ABANDONED :
				(executeCommandResult == EXECUTE_COMMAND_SUCCESS) ? IOTHUBMESSAGE_ACCEPTED :
				IOTHUBMESSAGE_REJECTED;
			free(temp);
		}
	}
	return result;
}



void initTime()
{
	time_t epochTime;
	configTime(0, 0, "pool.ntp.org", "time.nist.gov");

	while (true)
	{
		epochTime = time(NULL);

		if (epochTime == 0)
		{
			LogInfo("Fetching NTP epoch time failed! Waiting 2 seconds to retry.");
			Serial.println("Fetching NTP epoch time failed! Waiting 2 seconds to retry.");
			delay(2000);
		}
		else
		{
			LogInfo("Fetched NTP epoch time is: %lu", epochTime);
			Serial.printf("Fetched NTP epoch time is: %lu", epochTime);

			break;
		}
	}
}
