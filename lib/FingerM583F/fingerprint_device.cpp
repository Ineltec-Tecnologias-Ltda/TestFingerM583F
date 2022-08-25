#include "fingerprint_device.h"
#include <Arduino.h>
HardwareSerial fingerDevice(FINGER_PORT);

void commFingerInit(unsigned long baud)
{
	fingerDevice.begin(baud, SERIAL_8N1, 3, 1);
}

/* read one byte */
S32Bit FP_device_read_one_byte(U8Bit *data, U32Bit timeout)
{
	int bytesAvailable = 0;
	do
	{
		bytesAvailable = fingerDevice.available();
		if (bytesAvailable > 0)
		{
			*data = fingerDevice.read();
			return FP_OK;
		}
		delay(10);
	} while (timeout -= 10 > 0);

	return FP_DEVICE_TIMEOUT_ERROR;
}

/* read data */
S32Bit FP_device_read_data(U8Bit *data, size_t length, U32Bit timeout)
{
	for (U32Bit i = 0; i < length; i++)
	{
		if (FP_device_read_one_byte(data + i, timeout) != FP_OK)
			return FP_DEVICE_TIMEOUT_ERROR;
	}
	return FP_OK;
}

/* write data */
S32Bit FP_device_write_data(U8Bit *data, size_t length)
{
	fingerDevice.write(data,length);
}
