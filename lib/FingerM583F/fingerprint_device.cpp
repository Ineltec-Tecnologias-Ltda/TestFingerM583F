#include "fingerprint_device.h"
#include "fingerprint_protocol.h"
#include <Arduino.h>

#define fingerVin GPIO_NUM_13
#define fingerInt GPIO_NUM_5

HardwareSerial fingerDevice(FINGER_PORT);

extern U8Bit dataBuffer[];

bool fingerInterrupt = false;

U8Bit sumTxDebug = 0;
int sum = 0;
S16Bit timeout = 10;
/// Header + header lenght
U8Bit txHeader[] = {0xF1, 0x1F, 0xE2, 0x2E, 0xB6, 0x6B, 0xA8, 0x8A, 0, 0};

void fingerModuleInterrupt()
{
	// LOG("Finger sensor...");
	fingerInterrupt = true;
}

void commFingerInit(unsigned long baud)
{
	pinMode(fingerVin, OUTPUT);
	// Turns on MosFet to powerup Finger Module
	digitalWrite(fingerVin, HIGH);

	pinMode(fingerInt, INPUT);
	attachInterrupt(fingerInt, fingerModuleInterrupt, RISING);

	fingerDevice.begin(baud, SERIAL_8N1, 3, 1);
	delay(100);
	LOGINIT();
	delay(100);
}

/* read one byte and adds to checksum*/
S32Bit FP_device_read_one_byte(U8Bit *data)
{
	int bytesAvailable = 0;
	do
	{
		bytesAvailable = fingerDevice.available();
		if (bytesAvailable > 0)
		{
			*data = (U8Bit)fingerDevice.read();
			sum += *data;
			return FP_OK;
		}
		delay(10);
	} while (timeout-- > 0);

	return FP_DEVICE_TIMEOUT_ERROR;
}

// Sends Command header and prepares command buffer
void sendCommandHeader(const U8Bit *command)
{
	// Total Command lenght
	txHeader[8] = 0;
	txHeader[9] = command[2];

	sum = 0;
	writeBufferPlusCheckSum(txHeader, 10);
	memset(dataBuffer, 0, 8); // Sets Check password to zeroes

	// Command data
	dataBuffer[4] = command[0];
	dataBuffer[5] = command[1];

	sum = 0;
}

// writes data + calculate and send checksum
void writeBufferPlusCheckSum(U8Bit *data, size_t length)
{
	U8Bit x = 0;
	for (uint i = 0; i < length; i++)
	{
		x = *data++;
		sum += x;
		fingerDevice.write(x);
	};
	sum = (U8Bit)((~sum) + 1);
	sumTxDebug = (U8Bit)sum;
	fingerDevice.write((U8Bit)sum);
}

// Sends Commands with no extra data
void sendSimpleCommand(const U8Bit *command)
{
	sendCommandHeader(command);
	writeBufferPlusCheckSum(dataBuffer, 6);
}

// writes data + calculate checksum
void writeBuffer(U8Bit *data, size_t length)
{
	U8Bit x = 0;
	for (uint i = 0; i < length; i++)
	{
		x = *data++;
		sum += x;
		fingerDevice.write(x);
	};
}