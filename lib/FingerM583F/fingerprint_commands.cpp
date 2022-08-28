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

/// Tests if Finger Module is responsive
// @see Users Manual page 49
bool heartbeat()
{

    // Total Command lenght
    txHeader[8] = 0;
    txHeader[9] = 7;
    sendCommandHeader(cmd_maintenance, maintenance_heart_beat);
    writeBufferPlusCheckSum(dataBuffer, 6);
    return FP_protocol_recv_complete_frame();
}

// @see Users Manual page 45
bool ledControl(uint8_t *params)
{
    // Total Command length
    txHeader[8] = 0;
    txHeader[9] = 12;
    sendCommandHeader(cmd_system, sys_set_led);

    writeBuffer(dataBuffer, 6);
    writeBufferPlusCheckSum(params, 5);
    return FP_protocol_recv_complete_frame();
}

// @see Users Manual page 48
bool readId()
{
    // Total Command lenght
    txHeader[8] = 0;
    txHeader[9] = 7;
    sendCommandHeader(cmd_maintenance, maintenance_read_id);
    writeBufferPlusCheckSum(dataBuffer, 6);
    if (FP_protocol_recv_complete_frame())
    {
        /* gets Ascii value module id */
        dataBuffer[answerDataLength] = 0;
        LOGF("Module Id: %s\r\n", dataBuffer);
        return true;
    }

    return false;
}

/// Returns true if match ok, and sets slotId with template position inside finger module
// otherwise sets errorCode and errorMessage
// @see Users Manual pages 22 and 23
bool autoEnroll()
{

    // Total Command lenght
    txHeader[8] = 0;
    txHeader[9] = 11;
    sendCommandHeader(cmd_fingerprint, fp_auto_enroll);

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
                    errorMessage = Enrolling;
                }
            }
            else
            {
                errorMessage = TryAgain;
                LOGF("Enroll Error: %d\r\n", errorCode);
                return false;
            }
        }
        else
        {
            errorMessage = TryAgain;
            LOGF(" Error: %d\r\n", errorCode);
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
// TODO usar metodo 5.11 Fingerprint matching (synchronization) ?? page 26 ??
bool matchTemplate()
{
    // Command length
    txHeader[8] = 0;
    txHeader[9] = 7;
    sendCommandHeader(cmd_fingerprint, fp_match_start);

    int8_t retry = 3;
    bool start = true;

    while (retry-- > 0)
    {
        if (start)
        {
            sum = 0;
            writeBufferPlusCheckSum(dataBuffer, 6);
            if (FP_protocol_recv_complete_frame())
            {
                retry = 10;
                start = false;
                vTaskDelay(200);
            }
            else
                vTaskDelay(50);
        }
        else
        {
            sum = 0;
            dataBuffer[5] = fp_match_result;
            writeBufferPlusCheckSum(dataBuffer, 6);
            if (!FP_protocol_recv_complete_frame())
                vTaskDelay(100);
            else
            {
                if (errorCode == 0 && dataBuffer[11] == 1) // pass ok == 1
                {
                    uint16_t score = dataBuffer[12] << 8 + dataBuffer[13];
                    slotID = dataBuffer[15]; // slotID;
                    LOGF("Score: %d   Match ok: %d\r\n", score, slotID);
                    return true;
                }
                else if (errorCode == COMP_CODE_CMD_NOT_FINISHED || errorCode == FP_DEVICE_TIMEOUT_ERROR)
                    vTaskDelay(100);
                else
                {
                    errorMessage = TryAgain;
                    LOGF(" Error: %d\r\n", errorCode);
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
