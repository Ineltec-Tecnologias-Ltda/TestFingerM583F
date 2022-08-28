#include "fingerprint_device.h"
#include "fingerprint_protocol.h"
#include <Arduino.h>
HardwareSerial fingerDevice(FINGER_PORT);
static S8Bit sum = 0;
static U8Bit timeout = 10;
/// Header + header lenght
static U8Bit txHeader[] = {0xF1, 0x1F, 0xE2, 0x2E, 0xB6, 0x6B, 0xA8, 0x8A, 0, 0};

void commFingerInit(unsigned long baud)
{
	fingerDevice.begin(baud, SERIAL_8N1, 3, 1);
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
			*data = fingerDevice.read();
			sum += *data;
			return FP_OK;
		}
		delay(10);
	} while (timeout -= 10 > 0);

	return FP_DEVICE_TIMEOUT_ERROR;
}

// Sends Command header and prepares command buffer
void sendCommandHeader(U8Bit commandHigh, U8Bit commandLow)
{
	sum = 0;
	writeBufferPlusCheckSum(txHeader, 10);
	memset(dataBuffer, 0, 4); //Sets Check password to zeroes

	// Command data
	dataBuffer[4] = commandHigh;
	dataBuffer[5] = commandLow;
}

/* write data */
void FP_device_write_data(U8Bit *data, size_t length)
{
	fingerDevice.write(data, length);
}

// writes data + calculate and send checksum
void writeBufferPlusCheckSum(U8Bit *data, size_t length)
{
	U8Bit x = 0;
	for (int i = 0; i < length; i++)
	{
		x = *data++;
		sum += x;
		fingerDevice.print(x);
	};
	fingerDevice.print((U8Bit)((~sum) + 1));
}

// writes data + calculate checksum
void writeBuffer(U8Bit *data, size_t length)
{
	U8Bit x = 0;
	for (int i = 0; i < length; i++)
	{
		x = *data++;
		sum += x;
		fingerDevice.print(x);
	};
}