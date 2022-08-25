#include <Arduino.h>
#include "fingerprint_action.h"
#include "fingerprint_type.h"
#include "fingerprint_protocol.h"

const char *ssid = "FingerTests";
const char *password = "123456789";

U16Bit slotID;
S32Bit errorCode = 0;
const char *errorMessage;

const char *EnrollOk = "Enroll OK";
const char *Enrolling = "Enrolling...";
const char *TimeoutError = "Timeout Error...";
const char *TryAgain = "Please Try Again";

/// Returns true if match ok, and sets slotId with template position inside finger module
// otherwise sets errorCode and errorMessage
bool autoEnroll()
{

    S32Bit ret = 0;
    U8Bit enroll_mode;
    U8Bit times;

    // enrollPara.enroll_mode = 0x01 will indicate that user must lift finger and press again during enrollment
    // enrollPara.times is the number of presses (can be set to 1~6 times)
    // enrollPara.slotID = 0xFFFF ,will be automatically assigned by the system
    const FP_auto_enroll_t enrollPara = FP_auto_enroll_t{1, 6, 0xFFFF};
    FP_enrollResult_p pEnrollResult;

    int8_t retry = 3;
    bool start = true;

    while (retry-- > 0)
    {
        if (start)
        {
            ret = FP_action_auto_enroll_send(enrollPara);
            if (FP_OK == ret)
            {
                retry = 6; // max 6 enroll updates
                start = false;
            }
            else
                LOGF("Error: %d  retry: %d", ret, retry);
        }
        else
        {
            ret = FP_action_auto_enroll_recv(pEnrollResult);
            if (ret == FP_OK)
            {
                LOGF("State: %d    Enroll Progress: %d %\r\n", pEnrollResult->state, pEnrollResult->progress);
                if ((100 == pEnrollResult->progress) && (pEnrollResult->state == 0xff))
                {
                    LOGF("Template slot: %d\r\n", pEnrollResult->slotID);
                    slotID = pEnrollResult->slotID;
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
                errorCode = ret;
                errorMessage = TryAgain;
                LOGF("Enroll Error: %d\r\n", errorCode);
            }
        }
    }
    LOG("Timeout Error");
    errorMessage = TimeoutError;

    return false;
}

/// Returns true if match ok, and sets slotId with template position inside finger module
// otherwise sets errorCode and errorMessage
bool matchTemplate()
{
    S32Bit ret;
    int8_t retry = 100;
    FP_matchResult_t matchResult;
    bool start = true;

    while (retry-- > 0)
    {
        if (start)
        {
            ret = FP_action_match_start();
            if (ret == FP_OK)
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
            ret = FP_action_get_match_result(&matchResult);
            if (FP_DEVICE_TIMEOUT_ERROR == ret)
                vTaskDelay(100);
            else
            {
                if (ret != FP_OK)
                {
                    Log.printf("Match Error: %d\r\n", ret);
                    errorCode = ret;
                    return false;
                }

                if (matchResult.isPass == 1)
                {
                    LOGF("Match ok: %d\r\n", matchResult.slotID);
                    slotID = matchResult.slotID;
                    return true;
                }
            }
        }
        LOG("Timeout Error");
        errorMessage = TimeoutError;
        errorCode = FP_DEVICE_TIMEOUT_ERROR;
        return false;
    }
