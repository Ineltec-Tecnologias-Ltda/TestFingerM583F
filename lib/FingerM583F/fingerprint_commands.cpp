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

/// Command codes and extra data size
/// @see users manual Command set summary pages 9-12
Command AutoEnroll{cmd_fingerprint, fp_auto_enroll, 4};
Command HeartBeat{cmd_maintenance, maintenance_heart_beat, 0};
Command LedControl{cmd_system, sys_set_led, 5};
Command ReadId{cmd_maintenance, maintenance_read_id, 0};
Command MatchTemplate{cmd_fingerprint, fp_match_start, 0};
Command MatchResult{cmd_fingerprint, fp_match_result, 0};
Command FingerIsTouch{cmd_fingerprint, fp_query_slot_status, 0};
Command Enroll{cmd_fingerprint, fp_enroll_start, 1};
Command EnrollResult{cmd_fingerprint, fp_enroll_result, 1};
Command ModuleReset{cmd_system, sys_reset, 0};
Command SendTemplateStart{cmd_fingerprint, fp_start_send_template, 4};   // @see users manual page 36
Command SendTemplateData{cmd_fingerprint, fp_send_template_data, 0x89};  // 0x89 is the maximum to be sent at each packet
Command ReceiveTemplateStart{cmd_fingerprint, fp_start_get_template, 2}; // @see users manual page 38
Command ReceiveTemplateData{cmd_fingerprint, fp_get_template_data, 2};
Command DeleteTemplates{cmd_fingerprint, fp_delete_templates, 3}; // @see users manual page 33

/// @brief Sends Commands with no extra data, and receives response from module
// @see page Command set summary on pages 9 to 12 on users manual
/// @param command Only commands with fixed extra data bytes after header
/// @return if true, sets "dataBuffer" and "answerDataLength" according to received data
/// if false errorCode and  errorMessage are set
bool sendCommandReceiveResponse(Command command)
{
    sendCommandHeader(command, command[2]);
    writeBufferPlusCheckSum(command[2]);
    delay(100);
    return receiveCompleteResponse();
}

/// @brief Sends Commands with extra data, and receives response from module
// @see page Command set summary on pages 9 to 12 on users manual
/// @param command  Fix commands with variable extra data bytes after header(like 5.21 Fingerprint feature data download)
/// @param length number of extra bytes to send after
/// "dataBuffer" has to be filled with data( starting at index 6) to be sent
///  first 6 bytes are added by protocol methods with check password (4)+ command(2)
/// @return if true, sets "dataBuffer" and "answerDataLength" according to received data
/// if false "errorCode" and  "errorMessage" are set
bool sendCommandReceiveResponse(Command command, size_t length)
{
    sendCommandHeader(command, length);
    writeBufferPlusCheckSum(length);
    delay(100);
    return receiveCompleteResponse();
}

/// @brief Tests if Finger Module is responsive
/// @see Users Manual page 49
/// This is an example of how to send a command without aditional data
/// @return  true if command was accepted from module
/// if false "errorCode" and  "errorMessage" are set
bool heartbeat()
{
    return sendCommandReceiveResponse(HeartBeat);
}

/// @brief This is an example of how to send a command with aditional data
/// @param params == 5 bytes as described on  Users Manual page 45
/// "dataBuffer" (starting at index 6) is filled with "params"
///  first 6 bytes are added by protocol methods with check password (4)+ command(2)
/// @return  true if command was accepted from module
/// if false "errorCode" and  "errorMessage" are set
bool ledControl(uint8_t *params)
{
    memcpy(dataBuffer + 6, params, LedControl[2]);
    return sendCommandReceiveResponse(LedControl);
}

/// @brief This is an example of how to send a command without aditional data
/// @return  true if command was accepted from module
/// if false "errorCode" and  "errorMessage" are set
bool moduleReset()
{
    return sendCommandReceiveResponse(ModuleReset);
}

/// @brief see Users Manual page 48
/// This is an example of how to send a command without aditional data and receive data from module
/// @return  true if command was accepted from module,  "errorCode" has to be == 0 and "answerDataLength" > 0
/// if false "errorCode" and  "errorMessage" are set
bool readId()
{
    if (sendCommandReceiveResponse(ReadId) == true && errorCode == FP_OK && answerDataLength > 0)
    {
        debugRxState = -1000;
        /* gets Ascii value module id */
        dataBuffer[answerDataLength] = 0;
        LOGF("Module Id...: %s\r\n", dataBuffer);
        return true;
    }
    LOGF("Module Id Error:  0x%04X\r\n", errorCode);
    return false;
}

bool fingerDetection()
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
            // @see Users Manual page 32
            if (sendCommandReceiveResponse(FingerIsTouch) == true && errorCode == FP_OK)
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

/// @brief Returns true if match ok
// @see Users Manual pages 22 and 23
/// @return if true "slotID" is where the saved finger template was saved inside module
/// if false errorCode and  errorMessage are set
bool autoEnroll()
{
    if (!fingerDetection())
        return false;

    sendCommandHeader(AutoEnroll, AutoEnroll[2]);

    // enrollPara.enroll_mode = 0x01 will indicate that user must lift finger and press again during enrollment
    dataBuffer[6] = 0;

    // enrollPara.times is the number of presses (can be set to 1~6 times)
    dataBuffer[7] = 3;

    // enrollPara.slotID = 0xFFFF ,will be automatically assigned by the system
    dataBuffer[8] = 0xff;
    dataBuffer[9] = 0xff;
    writeBufferPlusCheckSum(AutoEnroll[2]);

    int8_t retry = 7;

    while (retry-- > 0)
    {
        if (receiveCompleteResponse())
        {
            if (errorCode == 0)
            {
                if (answerDataLength == 4)
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
                        delay(100);
                    }
                }
                else
                {
                    retry = 7;
                    LOGF("State: %d %\r\n", dataBuffer[0]);
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
                LOGF("Enroll Error:  0x%04X\r\n", errorCode);
                if (errorCode == COMP_CODE_NO_FINGER_DETECT)
                    delay(100);
                else
                    return false;
            }
        }
        else
        {
            errorMessage = TryAgain;
            LOGF("TryAgain?  Error:  0x%04X\r\n", errorCode);
            return false;
        }
    }
    LOG("Timeout Error");
    errorMessage = TimeoutError;

    return false;
}

/// Returns true if match ok, and sets "slotId" with template position inside finger module
// otherwise sets errorCode and errorMessage
// @see Users Manual pages 23 and 24
bool matchTemplate()
{
    if (!fingerDetection())
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
            if (sendCommandReceiveResponse(MatchTemplate) && errorCode == 0)
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
            if (!sendCommandReceiveResponse(MatchResult))
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
                    delay(100);
                else
                {
                    errorMessage = TryAgain;
                    LOGF(" Error: 0x%04X\r\n", errorCode);
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
