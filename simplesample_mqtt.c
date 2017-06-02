// 
// 
// 

#include "simplesample_mqtt.h"
/*

File Name  : mqtt_common.h
Create Date: 5/17/2017
Author     : Arthur A Garcia
Purpose    : This file will hold all the code necessary to connect to Azure IOT hub via the MQTT
protocol.
this code came from : https://github.com/Azure-Samples/iot-hub-feather-huzzah-client-app


*/

// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include <stdio.h>
#include <stdint.h>


/* This sample uses the _LL APIs of iothub_client for example purposes.
That does not mean that MQTT only works with the _LL APIs.
Simply changing the using the convenience layer (functions not having _LL)
and removing calls to _DoWork will yield the same results. */

//#ifdef ARDUINO
#include "AzureIoTHub.h"
//#else
//#include "azure_c_shared_utility/threadapi.h"
//#include "azure_c_shared_utility/platform.h"
//#include "serializer.h"
//#include "iothub_client_ll.h"
//#include "iothubtransportmqtt.h"
//#endif
//
//#ifdef MBED_BUILD_TIMESTAMP
//#include "certs.h"
//#endif // MBED_BUILD_TIMESTAMP


/*String containing Hostname, Device Id & Device Key in the format:             */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessKey=<device_key>"    */
//static const char* connectionString = "[device connection string]";
static const char* connectionString = "HostName=ArtTempIOT.azure-devices.net;DeviceId=esp8266v1;SharedAccessKey=8JkUvREdIVJH9RQ+7ew7eWCdKQPNL3w1lkxBCHNFAVg=";

// Define the Model
BEGIN_NAMESPACE(WeatherStation);

DECLARE_MODEL(ContosoAnemometer,
WITH_DATA(ascii_char_ptr, DeviceId),
WITH_DATA(int, WindSpeed),
WITH_ACTION(TurnFanOn),
WITH_ACTION(TurnFanOff),
WITH_ACTION(SetAirResistance, int, Position)
);

END_NAMESPACE(WeatherStation);


EXECUTE_COMMAND_RESULT TurnFanOn(ContosoAnemometer* device)
{
	printf("in TurnFanOn");
	(void)device;
	(void)printf("Turning fan on.\r\n");
	return EXECUTE_COMMAND_SUCCESS;
}

EXECUTE_COMMAND_RESULT TurnFanOff(ContosoAnemometer* device)
{
	printf("in TurnFanOff");
	(void)device;
	(void)printf("Turning fan off.\r\n");
	return EXECUTE_COMMAND_SUCCESS;
}

EXECUTE_COMMAND_RESULT SetAirResistance(ContosoAnemometer* device, int Position)
{
	(void)device;
	(void)printf("Setting Air Resistance Position to %d.\r\n", Position);
	return EXECUTE_COMMAND_SUCCESS;
}

void sendCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
	printf("-in sendCallback-");

	unsigned int messageTrackingId = (unsigned int)(uintptr_t)userContextCallback;

	(void)printf("Message Id: %u Received.\r\n", messageTrackingId);

	(void)printf("Result Call Back Called! Result is: %s \r\n", ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
}

static void sendMessage(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const unsigned char* buffer, size_t size)
{
	printf("in Sendmessage");


	static unsigned int messageTrackingId;
	IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromByteArray(buffer, size);

	printf("in Sendmessage create done ");

	if (messageHandle == NULL)
	{
		printf("unable to create a new IoTHubMessage\r\n");
	}
	else
	{
		printf(" handle exists ");

		if (IoTHubClient_LL_SendEventAsync(iotHubClientHandle, messageHandle, sendCallback, (void*)(uintptr_t)messageTrackingId) != IOTHUB_CLIENT_OK)
		{
			printf("failed to hand over the message to IoTHubClient");
		}
		else
		{
			printf("IoTHubClient accepted the message for delivery\r\n");
		}
		IoTHubMessage_Destroy(messageHandle);
	}
	messageTrackingId++;
}

/*this function "links" IoTHub to the serialization library*/
static IOTHUBMESSAGE_DISPOSITION_RESULT IoTHubMessage(IOTHUB_MESSAGE_HANDLE message, void* userContextCallback)
{
	printf("in IoTHubMessage ");


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
		//printf(buffer);


		///*buffer is not zero terminated*/
		//char* temp = malloc(size + 1);
		//if (temp == NULL)
		//{
		//	printf("failed to malloc\r\n");
		//	result = IOTHUBMESSAGE_ABANDONED;
		//}
		//else
		//{
		//	printf(" message execute IoTHubMessage ");

		//	(void)memcpy(temp, buffer, size);
		//	temp[size] = '\0';
		//	EXECUTE_COMMAND_RESULT executeCommandResult = EXECUTE_COMMAND(userContextCallback, temp);
		//	result =
		//		(executeCommandResult == EXECUTE_COMMAND_ERROR) ? IOTHUBMESSAGE_ABANDONED :
		//		(executeCommandResult == EXECUTE_COMMAND_SUCCESS) ? IOTHUBMESSAGE_ACCEPTED :
		//		IOTHUBMESSAGE_REJECTED;
		//	free(temp);
		//}
	}
	return result;
}

void simplesample_mqtt_run(void)
{
	printf("in mqtt run ");

	if (platform_init() != 0)
	{
		(void)printf("Failed to initialize platform.\r\n");
	}
	else
	{
		if (serializer_init(NULL) != SERIALIZER_OK)
		{
			(void)printf("Failed on serializer_init\r\n");
		}
		else
		{
			IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(connectionString, MQTT_Protocol);
			srand((unsigned int)time(NULL));
			int avgWindSpeed = 10;

			if (iotHubClientHandle == NULL)
			{
				(void)printf("Failed on IoTHubClient_LL_Create\r\n");
			}
			else
			{

				ContosoAnemometer* myWeather = CREATE_MODEL_INSTANCE(WeatherStation, ContosoAnemometer);
				if (myWeather == NULL)
				{
					(void)printf("Failed on CREATE_MODEL_INSTANCE\r\n");
				}
				else
				{
					if (IoTHubClient_LL_SetMessageCallback(iotHubClientHandle, IoTHubMessage, myWeather) != IOTHUB_CLIENT_OK)
					{
						printf("unable to IoTHubClient_SetMessageCallback\r\n");
					}
					else
					{
						myWeather->DeviceId = "myFirstDevice";
						myWeather->WindSpeed = avgWindSpeed + (rand() % 4 + 2);
						{
							unsigned char* destination;
							size_t destinationSize;
							if (SERIALIZE(&destination, &destinationSize, myWeather->DeviceId, myWeather->WindSpeed) != CODEFIRST_OK)
							{
								(void)printf("Failed to serialize\r\n");
							}
							else
							{
								sendMessage(iotHubClientHandle, destination, destinationSize);
								free(destination);
							}
						}

						/* wait for commands */
						while (1)
						{
							IoTHubClient_LL_DoWork(iotHubClientHandle);
							ThreadAPI_Sleep(100);
						}
					}

					DESTROY_MODEL_INSTANCE(myWeather);
				}
				IoTHubClient_LL_Destroy(iotHubClientHandle);
			}
			serializer_deinit();
		}
		platform_deinit();
	}
}


