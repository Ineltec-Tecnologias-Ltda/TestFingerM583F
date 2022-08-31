#include <Arduino.h>
#include "fingerprint_type.h"
#include "fingerprint_protocol.h"
#include "fingerprint_device.h"
#include "fingerprint_commands.h"

const char *ssid = "FingerTests";

const char *password = "123456789";

U8Bit slotID;
const char *errorMessage;

const char *EnrollOk = "Enroll OK";
const char *Enrolling = "Enrolling...";
const char *TimeoutError = "Timeout Error...";
const char *TryAgain = "Please Try Again";

/// Commands and command size
const U8Bit AutoEnroll[]{cmd_fingerprint, fp_auto_enroll, 11};
const U8Bit HeartBeat[]{cmd_maintenance, maintenance_heart_beat, 7};
const U8Bit LedControl[]{cmd_system, sys_set_led, 12};
const U8Bit ReadId[]{cmd_maintenance, maintenance_read_id, 7};
const U8Bit MatchTemplate[]{cmd_fingerprint, fp_match_start, 7};
const U8Bit MatchResult[]{cmd_fingerprint, fp_match_result, 7};
const U8Bit FingerIsTouch[]{cmd_fingerprint, fp_query_slot_status, 7};
const U8Bit Enroll[]{cmd_fingerprint, fp_enroll_start, 8};
const U8Bit EnrollResult[]{cmd_fingerprint, fp_enroll_result, 8};

/// Tests if Finger Module is responsive
// @see Users Manual page 49
bool heartbeat()
{
    sendCommandHeader(HeartBeat);
    writeBufferPlusCheckSum(dataBuffer, 6);

    return FP_protocol_recv_complete_frame();
}

// @see Users Manual page 45
bool ledControl(uint8_t *params)
{
    sendCommandHeader(LedControl);

    writeBuffer(dataBuffer, 6);
    writeBufferPlusCheckSum(params, 5);
    return FP_protocol_recv_complete_frame();
}

// @see Users Manual page 48
bool readId()
{

    sendCommandHeader(ReadId);
    writeBufferPlusCheckSum(dataBuffer, 6);
    if (FP_protocol_recv_complete_frame() == true && errorCode == 0 && answerDataLength > 0)
    {
        debugRxState = -1000;
        /* gets Ascii value module id */
        dataBuffer[answerDataLength] = 0;
        LOGF("Module Id...: %s\r\n", dataBuffer);
        return true;
    }
    LOGF("Module Id Error:  %04X\r\n", errorCode);
    return false;
}

bool fingerWaiting()
{
    LOG("Waiting for Finger...");
    int timeout = 600;
    fingerInterrupt = false;
    while (timeout-- > 0)
    {
        if (fingerInterrupt)
        {
            fingerInterrupt = false;
            delay(30);
            sendCommandHeader(FingerIsTouch);
            writeBufferPlusCheckSum(dataBuffer, 6);
            if (FP_protocol_recv_complete_frame() == true && errorCode == 0)
                if (dataBuffer[0] == 1)
                { // Finger is placed on module
                    LOG("Finger detected!!");
                    delay(30);
                    return true;
                }
            delay(30);
        }
        delay(10);
    }

    LOG("No Finger detected!!");
    return false;
}

/// Returns true if match ok, and sets slotId with template position inside finger module
// otherwise sets errorCode and errorMessage
// @see Users Manual pages 22 and 23
bool autoEnroll()
{
    if (!fingerWaiting())
        return false;

    sendCommandHeader(AutoEnroll);

    // enrollPara.enroll_mode = 0x01 will indicate that user must lift finger and press again during enrollment
    dataBuffer[6] = 1;

    // enrollPara.times is the number of presses (can be set to 1~6 times)
    dataBuffer[7] = 6;

    // enrollPara.slotID = 0xFFFF ,will be automatically assigned by the system
    dataBuffer[8] = 0xff;
    dataBuffer[9] = 0xff;
    writeBufferPlusCheckSum(dataBuffer, 10);

    int8_t retry = 7;

    while (retry-- > 0)
    {
        if (FP_protocol_recv_complete_frame())
        {
            if (errorCode == 0)
            {
                LOGF("State: %d    Enroll Progress: %d %\r\n", dataBuffer[0], dataBuffer[3]);
                if ((dataBuffer[3] == 100) && (dataBuffer[0] == 0xff))
                {
                    slotID = dataBuffer[2];
                    LOGF("Template slot: %d\r\n", slotID);
                    errorMessage = EnrollOk;
                    return true;
                }
                else
                {
                    // TODO implement callback!!!
                    errorMessage = Enrolling;
                }
            }
            else if (errorCode = FP_DEVICE_TIMEOUT_ERROR)
            {
                LOG("Timeout...");
                delay(100);
                continue;
            }
            else
            {
                errorMessage = TryAgain;
                LOGF("Enroll Error:  %04X\r\n", errorCode);
                if (errorCode == COMP_CODE_NO_FINGER_DETECT)
                    delay(100);
                else
                    return false;
            }
        }
        else
        {
            errorMessage = TryAgain;
            LOGF("TryAgain?  Error:  %04X\r\n", errorCode);
            delay(100);
            writeBufferPlusCheckSum(dataBuffer, 10);
            delay(100);
            return false;
        }
    }
    LOG("Timeout Error");
    errorMessage = TimeoutError;

    return false;
}

/// Returns true if match ok, and sets slotId with template position inside finger module
// otherwise sets errorCode and errorMessage
// @see Users Manual pages 13 and 14
bool enroll()
{
    U8Bit regIdx = 1;
    if (!fingerWaiting())
        return false;

    sendCommandHeader(Enroll);

    dataBuffer[6] = regIdx;
    writeBufferPlusCheckSum(dataBuffer, 7);

    bool start = true;
    int8_t retry = 7;

    while (retry-- > 0)
    {
        if (FP_protocol_recv_complete_frame())
        {
            if (start)
            {
                start = false;
                delay(100);
                sendCommandHeader(Enroll);
                writeBufferPlusCheckSum(dataBuffer, 7);
            }
            else if (errorCode == 0)
            {
                LOGF("State: %d    Enroll Progress: %d %\r\n", dataBuffer[10], dataBuffer[13]);
                if ((100 == dataBuffer[13]) && (dataBuffer[10] == 0xff))
                {
                    slotID = dataBuffer[12];
                    LOGF("Template slot: %d\r\n", slotID);
                    errorMessage = EnrollOk;
                    return true;
                }
                else
                {
                    // TODO implement callback!!!
                    regIdx++;
                    delay(100);
                    sendCommandHeader(EnrollResult);
                    writeBufferPlusCheckSum(dataBuffer, 7);
                    errorMessage = Enrolling;
                }
            }
            else
            {
                errorMessage = TryAgain;
                LOGF("Enroll Error: %04X\r\n", errorCode);
                if (errorCode == 8)
                    delay(100);
                else
                    return false;
            }
        }
        else
        {
            errorMessage = TryAgain;
            LOGF("TryAgain?  Error: %04X\r\n", errorCode);
            return false;
        }
    }
    LOG("Timeout Error");
    errorMessage = TimeoutError;

    return false;
}

/// Returns true if match ok, and sets slotId with template position inside finger module
// otherwise sets errorCode and errorMessage
// @see Users Manual pages 23 and 24
bool matchTemplate()
{
    if (!fingerWaiting())
        return false;

    int retry = 10;
    bool start = true;
    errorCode = 0;
    delay(200);

    while (retry-- > 0)
    {
        if (start)
        {
            sum = 0;
            sendCommandHeader(MatchTemplate);
            writeBufferPlusCheckSum(dataBuffer, 6);
            if (FP_protocol_recv_complete_frame() && errorCode == 0)
            {
                retry = 10;
                start = false;
            }
            else
                errorCode = 0;

            delay(200);
        }
        else
        {
            sum = 0;
            sendCommandHeader(MatchResult);
            writeBufferPlusCheckSum(dataBuffer, 6);
            if (!FP_protocol_recv_complete_frame())
                delay(100);
            else
            {
                if (errorCode == 0)
                {
                    if (dataBuffer[1] == 1)
                    {
                        // pass ok == 1)
                        uint16_t score = (uint16_t)dataBuffer[2] << 8 + dataBuffer[3];
                        slotID = dataBuffer[5]; // slotID;
                        LOGF(" Match ok   Score: %d   SlotId: %d\r\n", score, slotID);
                        return true;
                    }
                    else
                    {
                        slotID = 0xff;
                        LOG(" No Match");
                        return true;
                    }
                }
                else if (errorCode == COMP_CODE_CMD_NOT_FINISHED || errorCode == FP_DEVICE_TIMEOUT_ERROR)
                    vTaskDelay(100);
                else
                {
                    errorMessage = TryAgain;
                    LOGF(" Error: %04X\r\n", errorCode);
                    return false;
                }
            }
        }
    }
    LOG("Timeout Error");
    errorMessage = TimeoutError;
    errorCode = FP_DEVICE_TIMEOUT_ERROR;
    return false;
}
