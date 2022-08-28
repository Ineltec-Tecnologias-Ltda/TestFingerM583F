
#include "fingerprint_protocol.h"
#include "fingerprint_device.h"

#define DEFAULT_TIME_OUT (100)

#ifdef ENABLE_DEBUG_FINGER
HardwareSerial Log(DEBUG_PORT);
#endif


static const U8Bit rxHeader[8] = {0xF1, 0x1F, 0xE2, 0x2E, 0xB6, 0x6B, 0xA8, 0x8A};

///Used for tx and rx data to/from Finger Module
 U8Bit dataBuffer[140];

//Total received data lenght from finger module
 U8Bit answerDataLength;

//Answer received from finger module
 U8Bit rtxCommandHigh;
 U8Bit rtxCommandLow;

//Answer received from finger module: must be == zero
 S32Bit errorCode;

static S32Bit FP_action_get_errorCode(U8Bit *buffer)
{
    errorCode = (U32Bit)buffer[0] << 24;
    errorCode += (U32Bit)buffer[1] << 16;
    errorCode += (U32Bit)buffer[2] << 8;
    errorCode += (U32Bit)buffer[3];

    return errorCode;
}

bool FP_protocol_get_frame_head()
{
    U8Bit header = 0;
    U8Bit headerPos = 0;
    sum = 0;
    timeout = 10;
    while (timeout-- > 0)
    {
        if (FP_device_read_one_byte(&header) == FP_OK)
        {
            // Must receive complete rxHeader
            // To consider as a valid response
            if (header == rxHeader[headerPos])
            {
                timeout = 10;
                if (++headerPos == 8)
                {
                    if (FP_device_read_one_byte(&header) == FP_OK && header == 0) // Data lenght high is always zero
                        if (FP_device_read_one_byte(&header) == FP_OK && header >= 11 && header < 138)
                        { // Checks if valid data length
                            answerDataLength = header;
                            // Checks if calculated sum == received sum
                            if (FP_device_read_one_byte(&header) == FP_OK && sum == 0)
                                return true;
                        }
                    return false;
                }
            }
            else
            {
                sum = 0;
                headerPos = 0;
            }
        }
    }
    return false;
}

bool FP_protocol_recv_complete_frame()
{
    U8Bit command = 0;
    U8Bit timeout = 10;
    U8Bit pos = 0;
    if (!FP_protocol_get_frame_head())
        return false;

    // Must now receive at least Check password + Command + error code + Checksum
    sum = 0;
    timeout = 10;
    while (pos < 11)
    {
        if (FP_device_read_one_byte(dataBuffer + pos) == FP_OK)
        {
            pos++;
            answerDataLength--;
        }
        else
            return false;
    }
    if (answerDataLength == 0)
    { // response with no extra data bytes
        if (sum == 0)
        {
            rtxCommandHigh = dataBuffer[4];
            rtxCommandLow = dataBuffer[5];
            FP_action_get_errorCode(dataBuffer + 6);
            return true; // Valid response with no extra data bytes
        }
        return false;
    }

    // Has received first extra data byte
    dataBuffer[0] = dataBuffer[10];
    timeout = 10;

    // Has to receive all other extra data bytes
    pos = 1;
    U8Bit dataLength = answerDataLength;
    while (dataLength-- > 0)
    {
        if (FP_device_read_one_byte(dataBuffer + pos) == FP_OK)
            pos++;
        else
            return false;
    }
    if (sum == 0)
        return true;
    return false;
}
