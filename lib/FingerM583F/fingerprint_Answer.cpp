
#include "fingerprint_protocol.h"
#include "fingerprint_device.h"

S16Bit debugRxState = 0;

static const U8Bit rxHeader[8] = {0xF1, 0x1F, 0xE2, 0x2E, 0xB6, 0x6B, 0xA8, 0x8A};

/// Used for tx and rx data to/from Finger Module
U8Bit dataBuffer[140];

// Total received data lenght from finger module
U8Bit answerDataLength;

// Answer command received from finger module
U8Bit rtxCommandHigh;
// Answer command received from finger module
U8Bit rtxCommandLow;

// Answer received from finger module: must be == zero
S32Bit errorCode;

/// Saves error code and command from received data
static S32Bit FP_action_get_errorCode(U8Bit *buffer)
{
    rtxCommandHigh = dataBuffer[4];
    rtxCommandLow = dataBuffer[5];

    errorCode = (U32Bit)buffer[6] << 24;
    errorCode += (U32Bit)buffer[7] << 16;
    errorCode += (U32Bit)buffer[8] << 8;
    errorCode += (U32Bit)buffer[9];

    return errorCode;
}

///Gets complete header
///Verifies header checksum
/// If ok, sets "answerDataLength" == total data bytes to receive after header
/// If not ok, sets "errorCode" value
bool FP_protocol_get_frame_head()
{
    debugRxState = 0;
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
            if (debugRxState == 0)
                debugRxState = 1;
            if (header == rxHeader[headerPos])
            {

                debugRxState++;

                timeout = 10;
                if (++headerPos == 8)
                {
                    debugRxState = 30;
                    if ((errorCode = FP_device_read_one_byte(&header)) == FP_OK && header == 0) // Data lenght high is always zero
                        if ((errorCode = FP_device_read_one_byte(&header)) == FP_OK && header >= 11 && header < 138)
                        { // Checks if valid data length
                            debugRxState = 40;
                            answerDataLength = header;
                            // Checks if calculated sum == received sum
                            if (FP_device_read_one_byte(&header) == FP_OK && ((U8Bit)((~sum) + 1)) == 0)
                            {
                                debugRxState = 50;
                                return true;
                            }
                            else
                                errorCode = FP_PROTOCOL_UART_HEAD_CHECKSUM_ERROR;
                        }
                        else
                            errorCode = FP_PROTOCOL_RECV_DATA_LENGTH_ERROR;
                    else
                        errorCode = FP_DEVICE_TIMEOUT_ERROR;
                    return false;
                }
            }
            else
            {
                sum = 0;
                headerPos = 0;
            }
        }
        else
        {
            errorCode = FP_DEVICE_TIMEOUT_ERROR;
            delay(10);
            debugRxState += 100;
        }
    }
    return false;
}

/// Receives complete response
/// if response ok:
///   - Sets "errorCode" with received code ( codes list at "fingerprint_protocol.h")
///   - Places all received data to "dataBuffer"
///   - Sets "answerDataLength" with received data size
bool FP_protocol_recv_complete_frame()
{
    U8Bit command = 0;
    U8Bit timeout = 10;
    U8Bit pos = 0;
    if (!FP_protocol_get_frame_head())
        return false;
    debugRxState = -1;

    // Must now receive at least Check password + Command + error code + Checksum
    sum = 0;
    timeout = 10;
    errorCode = 0;
    while (pos < 11)
    {
        if (FP_device_read_one_byte(dataBuffer + pos) == FP_OK)
        {
            debugRxState--;
            pos++;
            answerDataLength--;
        }
        else
        {
            errorCode = FP_DEVICE_TIMEOUT_ERROR;
            return false;
        }
    }
    if (answerDataLength == 0)
    { // response with no extra data bytes
        if (sum == 0)
        {
            FP_action_get_errorCode(dataBuffer);
            LOG(" Valid response, no extras\r\n");
            return true; // Valid response with no extra data bytes
        }
        else
            errorCode = FP_PROTOCOL_DATA_CHECKSUM_ERROR;
        return false;
    }

    // Has received first extra data byte after error code
    //Place this bytes on first data buffer position
    dataBuffer[0] = dataBuffer[10];

    pos = 1; //index where to place other data bytes
    timeout = 10;
    debugRxState = -100;
    FP_action_get_errorCode(dataBuffer);

    // Has to receive all other extra data bytes
    U8Bit dataLength = answerDataLength;
    while (dataLength-- > 0)
    {
        debugRxState--;
        if (FP_device_read_one_byte(dataBuffer + pos) == FP_OK)
            pos++;
        else
        {
            errorCode = FP_DEVICE_TIMEOUT_ERROR;
            debugRxState = -255;
        }
    }
    if (((U8Bit)((~sum) + 1)) == 0)
    {
        debugRxState = -200;
        LOGF(" Valid response, errorCode: %d\r\n", errorCode);
        return true;
    }
    else
        errorCode = FP_PROTOCOL_DATA_CHECKSUM_ERROR;

    debugRxState = -256;
    return false;
}
