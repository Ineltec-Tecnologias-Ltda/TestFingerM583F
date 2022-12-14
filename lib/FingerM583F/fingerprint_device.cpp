#include "fingerprint_device.h"
#include "fingerprint_protocol.h"
#include <Arduino.h>

HardwareSerial fingerDevice(1);

bool fingerInterrupt = false;

U8Bit sumTxDebug = 0;
int sum = 0;
S16Bit timeoutFinger = 10;
/// Header + header lenght
U8Bit txHeader[] = {0xF1, 0x1F, 0xE2, 0x2E, 0xB6, 0x6B, 0xA8, 0x8A, 0, 0};

void fingerModuleInterrupt()
{
	// Log("Finger sensor...");
	fingerInterrupt = true;
}

/// @brief Symbols ENABLE_DEBUG_FINGER,FINGER_PORT, FINGER_VIN_GPIO, FINGER_INT_GPIO are defined on file platformio.ini
/// This method has to be called before any other
/// @param baud
void commFingerInit(unsigned long baud)
{
	pinMode(FINGER_VIN_GPIO, OUTPUT);
	pinMode(FINGER_INT_GPIO, INPUT);

	// Turns on MosFet to powerup Finger Module
	digitalWrite(FINGER_VIN_GPIO, HIGH); 

	/// module sends interrupt signal when finger is touched
	attachInterrupt(6, fingerModuleInterrupt, RISING);

	fingerDevice.begin(baud, SERIAL_8N1, FINGER_UART_RX, FINGER_UART_TX);
	delay(100);
	LOGINIT();
	delay(100);
}

/// @brief Wait 100ms for char
void waitCharAvailable(){
	int tout = 100;
	while (tout-- > 0){
		if (fingerDevice.available()) return;
		delay(1);
	}
}
/// @brief /* read one byte and adds to checksum*/
/// @param data  pointer to received data byte
/// @return FP_OK or FP_DEVICE_timeoutFinger_ERROR
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
		else timeoutFinger--;
		delay(10);
	} while (timeoutFinger > 0);

	return FP_DEVICE_TIMEOUT_ERROR;
}

/// @brief Sends Command header to serial port and prepares command buffer
/// @param command pointer to command data buffer
///  @param length total of data bytes to be sent after header
void sendCommandHeader(Command command, const unsigned char length)
{
	// Total Command length including final checksum
	txHeader[8] = 0;
	txHeader[9] = length+7;

	sum = 0;

	U8Bit *data = txHeader;
	U8Bit x = 0;
	for (uint i = 0; i < 10; i++)
	{
		x = *data++;
		sum += x;
		fingerDevice.write(x);
	};
	sum = (U8Bit)((~sum) + 1);
	sumTxDebug = (U8Bit)sum;
	fingerDevice.write((U8Bit)sum);
	memset(dataBuffer, 0, 4); // Sets Check password to zeroes

	// Command data
	dataBuffer[4] = command[0];
	dataBuffer[5] =  command[1];

	sum = 0;
}

/// @brief  writes data to serial port + calculate and send checksum
/// @param length  bytes of "dataBuffer" to be output to serial port
void writeBufferPlusCheckSum(unsigned char length)
{
	length+=6;  //adds check password (4)+ command(2) 
	U8Bit x = 0;
	U8Bit *data = dataBuffer;
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