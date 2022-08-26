#include "fingerprint_protocol.h"
#include "fingerprint_action.h"

#define DEFAULT_TIME_OUT (100)

FP_send_t sendHeader = FP_send_t{};
FP_recv_t recvHeader = FP_recv_t{};
static FP_send_p s_send_p = &sendHeader;
static FP_recv_p s_recv_p = &recvHeader;
U8Bit rxData[128];
FP_data_area_t bufferRx = FP_data_area_t{rxData, 0};

void FP_action_set_send_buffer(U8Bit *buffer)
{
    s_send_p = (FP_send_p)buffer;
}

void FP_action_set_recv_buffer(U8Bit *buffer)
{
    s_recv_p = (FP_recv_p)buffer;
}

static S32Bit FP_action_get_errorCode(U8Bit *buffer)
{
    S32Bit errorCode;
    errorCode = (U32Bit)buffer[0] << 24;
    errorCode += (U32Bit)buffer[1] << 16;
    errorCode += (U32Bit)buffer[2] << 8;
    errorCode += (U32Bit)buffer[3];

    return errorCode;
}

FP_data_area_t FP_action_getID()
{
    bufferRx = FP_data_area_t{rxData, 0};

    U32Bit ret;

    /* get a frame to send */
    ret = FP_protocol_get_mtnce_read_id_frame(s_send_p);
    if (FP_OK == ret)
    {
        /* send */
        FP_protocol_send_mesg(s_send_p, DEFAULT_TIME_OUT);

        /* receive the responce frame */
        ret = FP_protocol_recv_complete_frame(s_recv_p, &bufferRx, DEFAULT_TIME_OUT);
        if (FP_OK == ret)
        {
            /* get the useful data */
            rxData[bufferRx.length] = 0;
            LOGF("Module Id: %s\r\n", rxData);
        }
    }
    return bufferRx;
}


void Heartbeat()
{
    bufferRx = FP_data_area_t{rxData, 0};

    U32Bit ret;

    /* get a frame to send */
    ret = FP_protocol_get_mtnce_read_id_frame(s_send_p);
    if (FP_OK == ret)
    {
        /* send */
        FP_protocol_send_mesg(s_send_p, DEFAULT_TIME_OUT);

        /* receive the responce frame */
        ret = FP_protocol_recv_complete_frame(s_recv_p, &bufferRx, DEFAULT_TIME_OUT);
        if (FP_OK == ret)
        {
            /* get the useful data */
            rxData[bufferRx.length] = 0;
            LOGF("Module Id: %s\r\n", rxData);
        }
    }
}

S32Bit FP_action_check_finger_is_touch(U8Bit *isTouch, U32Bit *errorCode)
{
  
    U32Bit ret;

    ret = FP_protocol_get_fp_is_touch_sensor_frame(s_send_p);
    if (FP_OK != ret)
        return ret;

    FP_protocol_send_mesg(s_send_p, DEFAULT_TIME_OUT);

    ret = FP_protocol_recv_complete_frame(s_recv_p, &bufferRx, DEFAULT_TIME_OUT);
    if (FP_OK != ret)
        return ret;

    *errorCode = FP_action_get_errorCode(s_recv_p->error_code);
    *isTouch = *bufferRx.data;

    return FP_OK;
}

S32Bit FP_action_enroll_start(U8Bit index, U32Bit *errorCode)
{

    U32Bit ret;

    ret = FP_protocol_get_fp_enroll_start_frame(s_send_p, index);
    if (FP_OK != ret)
        return ret;

    FP_protocol_send_mesg(s_send_p, DEFAULT_TIME_OUT);

    ret = FP_protocol_recv_complete_frame(s_recv_p, &bufferRx, DEFAULT_TIME_OUT);
    if (FP_OK != ret)
        return ret;

    *errorCode = FP_action_get_errorCode(s_recv_p->error_code);

    return FP_OK;
}

S32Bit FP_action_get_enroll_result(FP_enrollResult_p enrollResult, U32Bit *errorCode)
{
    U32Bit ret;

    ret = FP_protocol_get_fp_enroll_result_frame(s_send_p);
    if (FP_OK != ret)
        return ret;

    FP_protocol_send_mesg(s_send_p, DEFAULT_TIME_OUT);

    ret = FP_protocol_recv_complete_frame(s_recv_p, &bufferRx, DEFAULT_TIME_OUT);
    if (FP_OK != ret)
        return ret;

    enrollResult->slotID = (U16Bit) * (bufferRx.data + 0) * 0x100 + (U16Bit) * (bufferRx.data + 1);
    enrollResult->progress = (U8Bit) * (bufferRx.data + 2);

    *errorCode = FP_action_get_errorCode(s_recv_p->error_code);

    return FP_OK;
}

S32Bit FP_action_save_start(S16Bit id, U32Bit *errorCode)
{
    U32Bit ret;
    ret = FP_protocol_get_fp_enroll_save_start_frame(s_send_p, id);
    if (FP_OK != ret)
        return ret;

    FP_protocol_send_mesg(s_send_p, DEFAULT_TIME_OUT);

    ret = FP_protocol_recv_complete_frame(s_recv_p, &bufferRx, DEFAULT_TIME_OUT);
    if (FP_OK != ret)
        return ret;

    *errorCode = FP_action_get_errorCode(s_recv_p->error_code);

    return FP_OK;
}

S32Bit FP_action_get_save_result(U32Bit *errorCode, U16Bit *SaveID)
{
    S32Bit ret;

    ret = FP_protocol_get_fp_enroll_save_result_frame(s_send_p);
    if (FP_OK != ret)
        return ret;

    FP_protocol_send_mesg(s_send_p, DEFAULT_TIME_OUT);

    ret = FP_protocol_recv_complete_frame(s_recv_p, &bufferRx, DEFAULT_TIME_OUT);
    if (FP_OK != ret)
        return ret;

    *SaveID = (U16Bit) * (bufferRx.data + 0) * 0x100 + (U16Bit) * (bufferRx.data + 1);

    *errorCode = FP_action_get_errorCode(s_recv_p->error_code);

    return FP_OK;
}

S32Bit FP_action_match_start()
{
    U32Bit ret;

    ret = FP_protocol_get_fp_match_start_frame(s_send_p);
    if (FP_OK == ret)
    {
        FP_protocol_send_mesg(s_send_p, DEFAULT_TIME_OUT);
        ret = FP_protocol_recv_complete_frame(s_recv_p, &bufferRx, DEFAULT_TIME_OUT);
        if (FP_OK == ret)
            ret = FP_action_get_errorCode(s_recv_p->error_code);
    }
    return ret;
}

S32Bit FP_action_get_match_result(FP_matchResult_p matchResult)
{
    U32Bit ret;

    ret = FP_protocol_get_fp_match_result_frame(s_send_p);
    if (FP_OK == ret)
    {
        FP_protocol_send_mesg(s_send_p, DEFAULT_TIME_OUT);

        ret = FP_protocol_recv_complete_frame(s_recv_p, &bufferRx, DEFAULT_TIME_OUT);
        if (FP_OK != ret)
            return ret;

        matchResult->isPass = bufferRx.data[1];
        matchResult->matchScore = (U16Bit) * (bufferRx.data + 2) * 0x100 + (U16Bit) * (bufferRx.data + 3);
        matchResult->slotID = (U16Bit) * (bufferRx.data + 4) * 0x100 + (U16Bit) * (bufferRx.data + 5);
        ret = FP_action_get_errorCode(s_recv_p->error_code);
    }
    return FP_OK;
}

S32Bit FP_action_delete_start(S16Bit id, U32Bit *errorCode)
{
    U32Bit ret;

    ret = FP_protocol_get_fp_delete_start_frame(s_send_p, id);
    if (FP_OK != ret)
        return ret;

    FP_protocol_send_mesg(s_send_p, DEFAULT_TIME_OUT);

    ret = FP_protocol_recv_complete_frame(s_recv_p, &bufferRx, DEFAULT_TIME_OUT);
    if (FP_OK != ret)
        return ret;

    *errorCode = FP_action_get_errorCode(s_recv_p->error_code);

    return FP_OK;
}

S32Bit FP_action_get_delete_result(U32Bit *errorCode)
{
    U32Bit ret;

    ret = FP_protocol_get_fp_delete_result_frame(s_send_p);
    if (FP_OK != ret)
        return ret;

    FP_protocol_send_mesg(s_send_p, DEFAULT_TIME_OUT);

    ret = FP_protocol_recv_complete_frame(s_recv_p, &bufferRx, DEFAULT_TIME_OUT);
    if (FP_OK != ret)
        return ret;

    *errorCode = FP_action_get_errorCode(s_recv_p->error_code);

    return FP_OK;
}

S32Bit FP_action_sleep(FP_sleep_type type)
{
    U32Bit ret;

    ret = FP_protocol_get_sys_sleep_frame(s_send_p, type);
    if (FP_OK == ret)
    {
        FP_protocol_send_mesg(s_send_p, DEFAULT_TIME_OUT);
        ret = FP_protocol_recv_complete_frame(s_recv_p, &bufferRx, DEFAULT_TIME_OUT);
        if (FP_OK == ret)
            ret = FP_action_get_errorCode(s_recv_p->error_code);
    }
    return ret;
}

S32Bit FP_action_reset(U32Bit *errorCode)
{
    U32Bit ret;
    ret = FP_protocol_get_sys_reset_frame(s_send_p);
    if (FP_OK != ret)
        return ret;

    FP_protocol_send_mesg(s_send_p, DEFAULT_TIME_OUT);

    ret = FP_protocol_recv_complete_frame(s_recv_p, &bufferRx, DEFAULT_TIME_OUT);
    if (FP_OK != ret)
        return ret;

    *errorCode = FP_action_get_errorCode(s_recv_p->error_code);

    return FP_OK;
}

S32Bit FP_action_update_start(S16Bit updateID)
{
    U32Bit ret;

    ret = FP_protocol_get_fp_update_start_frame(s_send_p, updateID);
    if (FP_OK == ret)
    {
        FP_protocol_send_mesg(s_send_p, DEFAULT_TIME_OUT);
        if (FP_OK == ret)
        {
            ret = FP_protocol_recv_complete_frame(s_recv_p, &bufferRx, DEFAULT_TIME_OUT);
            if (FP_OK == ret)
                ret = FP_action_get_errorCode(s_recv_p->error_code);
        }
    }
    return ret;
}

S32Bit FP_action_get_update_result()
{
    U32Bit ret;

    ret = FP_protocol_get_fp_update_result_frame(s_send_p);
    if (FP_OK == ret)
    {
        FP_protocol_send_mesg(s_send_p, DEFAULT_TIME_OUT);
        if (FP_OK == ret)
        {
            ret = FP_protocol_recv_complete_frame(s_recv_p, &bufferRx, DEFAULT_TIME_OUT);
            if (FP_OK == ret)
                ret = FP_action_get_errorCode(s_recv_p->error_code);
        }
    }
    return ret;
}

S32Bit FP_action_auto_enroll_send(FP_auto_enroll_t enrollPara)
{
    U32Bit ret;

    ret = FP_protocol_fp_auto_enroll_frame(s_send_p, enrollPara);
    if (FP_OK == ret)
        FP_protocol_send_mesg(s_send_p, DEFAULT_TIME_OUT);
    return ret;
}

S32Bit FP_action_auto_enroll_recv(FP_enrollResult_p enrollResult)
{
    U32Bit ret;

    ret = FP_protocol_recv_complete_frame(s_recv_p, &bufferRx, DEFAULT_TIME_OUT);
    if (FP_OK == ret)
    {
        enrollResult->state = (U8Bit) * (bufferRx.data + 0);
        enrollResult->slotID = (U16Bit) * (bufferRx.data + 1) * 0x100 + (U16Bit) * (bufferRx.data + 2);
        enrollResult->progress = (U8Bit) * (bufferRx.data + 3);
        ret = FP_action_get_errorCode(s_recv_p->error_code);
    }
    return ret;
}

S32Bit FP_action_match_syn_send(void)
{
    U32Bit ret;

    ret = FP_protocol_fp_match_syn_frame(s_send_p);
    if (FP_OK == ret)
        FP_protocol_send_mesg(s_send_p, DEFAULT_TIME_OUT);
    return ret;
}

S32Bit FP_action_match_syn_recv(FP_matchResult_p matchResult)
{
    U32Bit ret;

    ret = FP_protocol_recv_complete_frame(s_recv_p, &bufferRx, DEFAULT_TIME_OUT);
    if (FP_OK == ret)
    {
        matchResult->isPass = bufferRx.data[1];
        matchResult->matchScore = (U16Bit) * (bufferRx.data + 2) * 0x100 + (U16Bit) * (bufferRx.data + 3);
        matchResult->slotID = (U16Bit) * (bufferRx.data + 4) * 0x100 + (U16Bit) * (bufferRx.data + 5);
        ret = FP_action_get_errorCode(s_recv_p->error_code);
    }
    return ret;
}

S32Bit FP_action_delete_syn_send(S16Bit id)
{
    U32Bit ret;

    ret = FP_protocol_fp_delete_syn_frame(s_send_p, id);
    if (FP_OK != ret)
        return ret;

    FP_protocol_send_mesg(s_send_p, DEFAULT_TIME_OUT);

    return FP_OK;
}

S32Bit FP_action_delete_syn_recv(U32Bit *errorCode)
{
    U32Bit ret;

    ret = FP_protocol_recv_complete_frame(s_recv_p, &bufferRx, DEFAULT_TIME_OUT);
    if (FP_OK != ret)
        return ret;

    *errorCode = FP_action_get_errorCode(s_recv_p->error_code);

    return FP_OK;
}
