
#include "fingerprint_protocol.h"
#include "fingerprint_device.h"
#include "fingerprint_action.h"

#ifdef ENABLE_DEBUG_FINGER
HardwareSerial Log(DEBUG_PORT);
#endif

typedef enum command_type
{
    cmd_automatic = 0x00,
    cmd_fingerprint = 0x01,
    cmd_system = 0x02,
    cmd_maintenance = 0x03
} FP_cmd_type;

typedef enum automatic_type_word
{
    fp_automatic_enroll = 0x03, // users manual page 22
} FP_automatic_cmd_word;

typedef enum fingerprint_type_word
{
    fp_capture_start = 0x01,  // Not in users manual
    fp_capture_result = 0x02, // Not in users manual

    fp_enroll_start = 0x11,
    fp_enroll_result = 0x12,
    fp_enroll_save_start = 0x13,
    fp_enroll_save_result = 0x14,
    fp_enroll_cancel = 0x15,

    fp_update_start = 0x16,
    fp_update_result = 0x17,

    fp_auto_enroll = 0x18,

    fp_match_start = 0x21,
    fp_match_result = 0x22,
    fp_match_sync = 0x23,

    fp_delete_start = 0x31,
    fp_delete_result = 0x32,
    fp_is_fp_id_exist = 0x33,
    fp_get_all_slots_status = 0x34,
    fp_query_slot_status = 0x35,

    fp_delete_templates = 0x36,

    fp_enroll_verify_start = 0x41,
    fp_enroll_verify_result = 0x42,
    fp_start_send_template = 0x51,
    fp_send_template_data = 0x52,
    fp_start_get_template = 0x53,
    fp_get_template_data = 0x54,

} FP_fp_cmd_word;

typedef enum system_command_word
{
    sys_set_passwd = 0x01,
    sys_reset = 0x02,
    sys_get_nb_templates = 0x03, // Obtain the number of fingerprint templates
    sys_set_gain = 0x08,
    sys_get_gain = 0x09,
    sys_set_thresholds = 0x0A, // set matching thresholds
    sys_get_thresholds = 0x0B, // Obtain matching thresholds
    sys_sleep = 0x0C,
    sys_set_enroll_max_num = 0x0D, // Setting range (1~6)
    sys_set_led = 0x0F,
    sys_get_policys = 0xFB, // Get System Policy Send Format Table
    sys_set_policys = 0xFC  // Set System Policy Send Format Table
} FP_sys_cmd_word;

typedef enum maintenance_command_word
{
    maintenance_read_id = 0x01,
    maintenance_set_id = 0x02,
    maintenance_heart_beat = 0x03,
    maintenance_set_baudrate = 0x04,
    maintenance_set_com_passwd = 0x05
} FP_mtnce_cmd_word;

static const U8Bit s_forhead[8] = {0xF1, 0x1F, 0xE2, 0x2E, 0xB6, 0x6B, 0xA8, 0x8A};


static U32Bit FP_protocol_get_data_area_length(U8Bit type, U8Bit command)
{
    U32Bit len = 0;
    if (cmd_fingerprint == type)
    {
        switch  (command)
        {
        case fp_capture_start:
            len = 2;
            break;
        case fp_capture_result:
            len = 2;
            break;
        case fp_enroll_start:
            len = 1;
            break;
        case fp_enroll_result:
            len = 0;
            break;
        case fp_enroll_save_start:
            len = 2;
            break;
        case fp_enroll_save_result:
            len = 0;
            break;
        case fp_enroll_cancel:
            len = 0;
            break;
        case fp_update_start:
            len = 2;
            break;
        case fp_update_result:
            len = 0;
            break;
        case fp_match_start:
            len = 0;
            break;
        case fp_match_result:
            len = 0;
            break;
        case fp_delete_start:
            len = 3;
            break;
        case fp_delete_result:
            len = 0;
            break;
        case fp_is_fp_id_exist:
            len = 2;
            break;
        case fp_get_all_slots_status:
            len = 0;
            break;
        case fp_query_slot_status:
            len = 0;
            break;
        case fp_enroll_verify_start:
            len = 0;
            break;
        case fp_enroll_verify_result:
            len = 0;
            break;
        }
    }
    else if (cmd_system == type)
    {
        switch (command)
        {
        case sys_set_passwd:
            len = 4;
            break;
        case sys_reset:
            len = 0;
            break;
        case sys_get_nb_templates:
            len = 0;
            break;
        case sys_set_gain:
            len = 3;
            break;
        case sys_get_gain:
            len = 0;
            break;
        case sys_set_thresholds:
            len = 2;
            break;
        case sys_get_thresholds:
            len = 0;
            break;
        case sys_sleep:
            len = 1;
            break;
        case sys_set_enroll_max_num:
            len = 1;
            break;
        }
    }
    else if (cmd_maintenance == type)
    {
        switch  switch (command)
        {
        case maintenance_read_id:
            len = 0;
            break;
        case maintenance_set_id:
            len = 16;
            break;
        case maintenance_heart_beat:
            len = 0;
            break;
        case maintenance_set_baudrate:
            len = 4;
            break;
        case maintenance_set_com_passwd:
            len = 4;
            break;
        }
    }

    return len;
}

static U32Bit FP_protocol_get_recv_data_length(FP_recv_p recv)
{
    U16Bit length = recv->frame_head.length;
    length = (U32Bit)(*(U8Bit *)&length) * 0x100 + *((U8Bit *)&length + 1);

    return length - (sizeof(FP_recv_t) - sizeof(FP_frame_head_t)) - 1;
}

static U8Bit FP_protocol_get_checksum(U8Bit *data, U32Bit length)
{
    U32Bit i = 0;
    S8Bit sum = 0;

    for (i = 0; i < length; i++)
        sum += data[i];

    return (U8Bit)((~sum) + 1);
}

static S32Bit FP_protocol_checkout_recv_head_checksum(FP_frame_head_p head)
{
    U8Bit checksum = FP_protocol_get_checksum((U8Bit *)head, sizeof(FP_frame_head_t) - 1);
    if (*((U8Bit *)head + sizeof(FP_frame_head_t) - 1) != checksum)
        return FP_PROTOCOL_UART_HEAD_CHECKSUM_ERROR;
    else
        return FP_OK;
}

static S32Bit FP_protocol_checkout_recv_data_checksum(FP_recv_p recv)
{
    U8Bit checksum;

    U16Bit length = recv->frame_head.length;
    length = (U32Bit)(*(U8Bit *)&length) * 0x100 + *((U8Bit *)&length + 1);

    checksum = FP_protocol_get_checksum((U8Bit *)recv + sizeof(FP_frame_head_t), length - 1);

    if (*((U8Bit *)recv + sizeof(FP_frame_head_t) + length - 1) != checksum)
        return FP_PROTOCOL_DATA_CHECKSUM_ERROR;
    else
        return FP_OK;
}

/*
 --------------------------------------------------------------------------------------------------------------------
 |                         |            |          |              |          |          |               |          |
 |          forhead        |            |          |              |          |          |               |          |
 |                         |   length   | checksum |     pwd      | cmd_type | cmd_word |   data_area   | checksum |
 | F1 1F E2 2E B6 6B A8 8A |            |          |              |          |          |               |          |
 |                         |            |          |              |          |          |               |          |
 --------------------------------------------------------------------------------------------------------------------
 |                         |            |          |              |          |          |               |          |
 |           8byte         |   2 byte   |  1 byte  |    4 byte    |  1 byte  |  1 byte  |     n byte    |  1 byte  |
 |                         |            |          |              |          |          |               |          |
 --------------------------------------------------------------------------------------------------------------------
 |                                                 |                                    |
 | <<----------    FP_frame_head_t    ---------->> |                                    |
 |                                                 |                                    |
 |--------------------------------------------------------------------------------------|
 |
                                                                                      |
 | <<-----------------------------     FP_send_t    --------------------------------->> |
 |                                                                                      |
 ----------------------------------------------------------------------------------------
*/

static void FP_protocol_get_frame_head(FP_frame_head_p head, unsigned short length)
{
    /*
     * forhead
     */
    U32Bit i = 0;
    for (i = 0; i < 8; i++)
    {
        *((U8Bit *)head->forhead + i) = *(s_forhead + i);
    }

    /*
     * length
     */
    *((U8Bit *)&head->length) = length / 0x100;
    *((U8Bit *)&head->length + 1) = length % 0x100;

    /*
     * checksum
     */
    head->checksum = FP_protocol_get_checksum((U8Bit *)head, sizeof(FP_frame_head_t) - 1);
}

static S32Bit FP_protocol_get_complete_send_frame(FP_send_p send, FP_data_area_t data_area)
{
    U16Bit data_area_len, frame_data_len;
    U32Bit i;
    U8Bit *p;

    data_area_len = FP_protocol_get_data_area_length(send.);
    if (data_area.length != data_area_len)
        return FP_PROTOCOL_SEND_DATA_LENGTH_ERROR;

    /* FP_frame_head_t */
    frame_data_len = data_area_len + sizeof(FP_send_t) - sizeof(FP_frame_head_t) + 1;
    FP_protocol_get_frame_head((FP_frame_head_p)send, frame_data_len);

    /* pwd */
    send->pwd = 0x00;

    /* data_area */
    p = (U8Bit *)send + sizeof(FP_send_t);
    if (data_area.data != 0 && data_area.length != 0)
    {
        for (i = 0; i < data_area.length; i++)
        {
            *(p + i) = *(data_area.data + i);
        }
    }

    /* check sum */
    p = (U8Bit *)send;
    *(p + sizeof(FP_send_t) + data_area.length) = FP_protocol_get_checksum(p + sizeof(FP_frame_head_t), frame_data_len - 1);

    return FP_OK;
}

/*
 * get fingerprint frame
 */
S32Bit FP_protocol_get_fp_enroll_start_frame(FP_send_p send, U8Bit index)
{
    FP_data_area_t data_area;
    U8Bit idx = index;

    send->cmd_type = cmd_fingerprint;
    send->cmd_word = fp_enroll_start;

    data_area.data = &idx;
    data_area.length = 1;

    return FP_protocol_get_complete_send_frame(send, data_area);
}

S32Bit FP_protocol_get_fp_enroll_result_frame(FP_send_p send)
{

    FP_data_area_t data_area;

    send->cmd_type = cmd_fingerprint;
    send->cmd_word = fp_enroll_result;

    data_area.data = 0;
    data_area.length = 0;

    return FP_protocol_get_complete_send_frame(send, data_area);
}

S32Bit FP_protocol_get_fp_enroll_save_start_frame(FP_send_p send, U16Bit id)
{
    FP_data_area_t data_area;

    U8Bit buff[2];

    buff[0] = id / 0x100;
    buff[1] = id % 0x100;

    send->cmd_type = cmd_fingerprint;
    send->cmd_word = fp_enroll_save_start;

    data_area.data = buff;
    data_area.length = 2;

    return FP_protocol_get_complete_send_frame(send, data_area);
}

S32Bit FP_protocol_get_fp_enroll_save_result_frame(FP_send_p send)
{
    FP_data_area_t data_area;

    send->cmd_type = cmd_fingerprint;
    send->cmd_word = fp_enroll_save_result;

    data_area.data = 0;
    data_area.length = 0;

    return FP_protocol_get_complete_send_frame(send, data_area);
}

S32Bit FP_protocol_get_fp_enroll_cancel_frame(FP_send_p send)
{
    FP_data_area_t data_area;

    send->cmd_type = cmd_fingerprint;
    send->cmd_word = fp_enroll_cancel;

    data_area.data = 0;
    data_area.length = 0;

    return FP_protocol_get_complete_send_frame(send, data_area);
}

S32Bit FP_protocol_get_fp_match_start_frame(FP_send_p send)
{
    FP_data_area_t data_area;

    send->cmd_type = cmd_fingerprint;
    send->cmd_word = fp_match_start;

    data_area.data = 0;
    data_area.length = 0;

    return FP_protocol_get_complete_send_frame(send, data_area);
}

S32Bit FP_protocol_get_fp_match_result_frame(FP_send_p send)
{
    FP_data_area_t data_area;

    send->cmd_type = cmd_fingerprint;
    send->cmd_word = fp_match_result;

    data_area.data = 0;
    data_area.length = 0;

    return FP_protocol_get_complete_send_frame(send, data_area);
}

S32Bit FP_protocol_get_fp_delete_start_frame(FP_send_p send, S16Bit id)
{
    U8Bit buff[3];
    FP_data_area_t data_area;

    if (id < 0)
    {
        buff[0] = 0x01;
    }
    else
    {
        buff[0] = 0x00;
        buff[1] = id / 0x100;
        buff[2] = id % 0x100;
    }

    send->cmd_type = cmd_fingerprint;
    send->cmd_word = fp_delete_start;

    data_area.data = buff;
    data_area.length = 3;

    return FP_protocol_get_complete_send_frame(send, data_area);
}

S32Bit FP_protocol_get_fp_delete_result_frame(FP_send_p send)
{
    FP_data_area_t data_area;

    send->cmd_type = cmd_fingerprint;
    send->cmd_word = fp_delete_result;

    data_area.data = 0;
    data_area.length = 0;

    return FP_protocol_get_complete_send_frame(send, data_area);
}

S32Bit FP_protocol_get_fp_is_id_exist_frame(FP_send_p send, U16Bit id)
{
    U8Bit buff[2];
    FP_data_area_t data_area;

    buff[0] = id / 0x100;
    buff[1] = id % 0x100;

    send->cmd_type = cmd_fingerprint;
    send->cmd_word = fp_is_fp_id_exist;

    data_area.data = buff;
    data_area.length = 2;

    return FP_protocol_get_complete_send_frame(send, data_area);
}

S32Bit FP_protocol_get_fp_store_info_frame(FP_send_p send)
{
    FP_data_area_t data_area;

    send->cmd_type = cmd_fingerprint;
    send->cmd_word = fp_get_all_slots_status;

    data_area.data = 0;
    data_area.length = 0;

    return FP_protocol_get_complete_send_frame(send, data_area);
}

S32Bit FP_protocol_get_fp_is_touch_sensor_frame(FP_send_p send)
{
    FP_data_area_t data_area;

    send->cmd_type = cmd_fingerprint;
    send->cmd_word = fp_query_slot_status;

    data_area.data = 0;
    data_area.length = 0;

    return FP_protocol_get_complete_send_frame(send, data_area);
}

S32Bit FP_protocol_get_fp_enroll_verify_start_frame(FP_send_p send)
{
    FP_data_area_t data_area;

    send->cmd_type = cmd_fingerprint;
    send->cmd_word = fp_enroll_verify_start;

    data_area.data = 0;
    data_area.length = 0;

    return FP_protocol_get_complete_send_frame(send, data_area);
}

S32Bit FP_protocol_get_fp_enroll_verify_result_frame(FP_send_p send)
{
    FP_data_area_t data_area;

    send->cmd_type = cmd_fingerprint;
    send->cmd_word = fp_enroll_verify_result;

    data_area.data = 0;
    data_area.length = 0;

    return FP_protocol_get_complete_send_frame(send, data_area);
}

/*
 * get system frame
 */
S32Bit FP_protocol_get_sys_get_count_frame(FP_send_p send)
{
    FP_data_area_t data_area;

    send->cmd_type = cmd_system;
    send->cmd_word = sys_get_nb_templates;

    data_area.data = 0;
    data_area.length = 0;

    return FP_protocol_get_complete_send_frame(send, data_area);
}

S32Bit FP_protocol_get_sys_reset_frame(FP_send_p send)
{
    FP_data_area_t data_area;

    send->cmd_type = cmd_system;
    send->cmd_word = sys_reset;

    data_area.data = 0;
    data_area.length = 0;

    return FP_protocol_get_complete_send_frame(send, data_area);
}

S32Bit FP_protocol_get_sys_sleep_frame(FP_send_p send, FP_sleep_type type)
{
    U8Bit buff[1];
    FP_data_area_t data_area;

    send->cmd_type = cmd_system;
    send->cmd_word = sys_sleep;

    if (sleep_type_normal == type)
    {
        buff[0] = 0x00;
    }
    else if (sleep_type_deep == type)
    {
        buff[0] = 0x01;
    }

    data_area.data = buff;
    data_area.length = 1;

    return FP_protocol_get_complete_send_frame(send, data_area);
}

/*
 *
 */
S32Bit FP_protocol_get_fp_update_start_frame(FP_send_p send, S16Bit updateID)
{
    U8Bit buff[2];
   
    buff[0] = updateID / 0x100;
    buff[1] = updateID % 0x100;

    send->cmd_type = cmd_fingerprint;
    send->cmd_word = fp_update_start;

    bufferRx.data = rxData;
    bufferRx.length = 2;

    return FP_protocol_get_complete_send_frame(send, bufferRx);
}

/*
 *
 */
S32Bit FP_protocol_get_fp_update_result_frame(FP_send_p send)
{
    send->cmd_type = cmd_fingerprint;
    send->cmd_word = fp_update_result;

    bufferRx.data = rxData;
    bufferRx.length = 0;

    return FP_protocol_get_complete_send_frame(send, bufferRx);
}

S32Bit FP_protocol_fp_auto_enroll_frame(FP_send_p send, FP_auto_enroll_t enrollPara)
{
   
    send->cmd_type = cmd_fingerprint;
    send->cmd_word = fp_auto_enroll;

    bufferRx.data = (U8Bit *)&enrollPara;
    bufferRx.length = sizeof(FP_auto_enroll_t);

    return FP_protocol_get_complete_send_frame(send, bufferRx);
}

S32Bit FP_protocol_fp_match_syn_frame(FP_send_p send)
{
    send->cmd_type = cmd_fingerprint;
    send->cmd_word = fp_match_sync;

    bufferRx.data =rxData;
    bufferRx.length = 0;

    return FP_protocol_get_complete_send_frame(send, bufferRx);
}

S32Bit FP_protocol_fp_delete_syn_frame(FP_send_p send, S16Bit id)
{
    U8Bit buff[3];

    if (id < 0)
    {
        buff[0] = 1;
    }
    else
    {
        buff[0] = 0;
        buff[1] = ((id & 0xff00) >> 8);
        buff[2] = (id & 0xff);
    }

    send->cmd_type = cmd_fingerprint;
    send->cmd_word = fp_delete_templates;

    bufferRx.data = rxData;
    bufferRx.length = 3;

    return FP_protocol_get_complete_send_frame(send, bufferRx);
}

/*
 * get maintenance frame
 */
S32Bit FP_protocol_get_mtnce_read_id_frame(FP_send_p send)
{
    send->cmd_type = cmd_maintenance;
    send->cmd_word = maintenance_read_id;

    bufferRx.data = rxData;
    bufferRx.length = 0;

    return FP_protocol_get_complete_send_frame(send, bufferRx);
}

void FP_protocol_send_command(FP_send_p send, U32Bit timeout)
{
    size_t length = sizeof(FP_send_t) + FP_protocol_get_data_area_length(send) + 1;

    FP_device_write_data((unsigned char *)send, length);
}

static S32Bit FP_protocol_recv_forhead(FP_recv_p recv, U32Bit timeout)
{
    S32Bit ret;
    U8Bit *p = (U8Bit *)recv;
    U32Bit i = 0;

    while (1)
    {
        for (i = 0; i < sizeof(s_forhead); i++)
        {
            if (FP_OK != (ret = FP_device_read_one_byte(p + i, timeout)))
                return ret;
            else if (*(p + i) != s_forhead[i])
                break;
        }

        if (sizeof(s_forhead) == i)
            break;
    }

    return FP_OK;
}

/*
 --------------------------------------------------------------------------------------------------------------------
 |                         |            |          |              |          |          |               |          |
 |          forhead        |            |          |              |          |          |               |          |
 |                         |   length   | checksum |     pwd      | cmd_type | cmd_word |   data_area   | checksum |
 | F1 1F E2 2E B6 6B A8 8A |            |          |              |          |          |               |          |
 |                         |            |          |              |          |          |               |          |
 --------------------------------------------------------------------------------------------------------------------
 |                         |            |          |              |          |          |               |          |
 |           8byte         |   2 byte   |  1 byte  |    4 byte    |  1 byte  |  1 byte  |     n byte    |  1 byte  |
 |                         |            |          |              |          |          |               |          |
 --------------------------------------------------------------------------------------------------------------------
 |                                                 |                                    |
 | <<----------    FP_frame_head_t    ---------->> |                                    |
 |                                                 |                                    |
 |--------------------------------------------------------------------------------------|
 |
                                                                                      |
 | <<-----------------------------     FP_send_t    --------------------------------->> |
 |                                                                                      |
 ----------------------------------------------------------------------------------------



 ---------------------------------------------------------------------------------------------------------------------------
 |                         |            |          |              |          |          |           |           |          |
 |                         |            |          |              |          |          |           |           |          |
 | F1 1F E2 2E B6 6B A8 8A |   length   | checksum |   password   | cmd_type | cmd_word | errorCode | data_area | checksum |
 |                         |            |          |              |          |          |           |           |          |
 |                         |            |          |              |          |          |           |           |          |
 ---------------------------------------------------------------------------------------------------------------------------
 |                         |            |          |              |          |          |           |           |          |
 |           8byte         |   2 byte   |  1 byte  |    4 byte    |  1 byte  |  1 byte  |   4 byte  |   n byte  |  1 byte  |
 |                         |            |          |              |          |          |           |           |          |
 ---------------------------------------------------------------------------------------------------------------------------
 |                                                 |                                                |
 | <<----------    FP_frame_head_t    ---------->> |                                                |
 |                                                 |                                                |
 |--------------------------------------------------------------------------------------------------|
 |
                                                                                                  |
 | <<-----------------------------------     FP_recv_t    --------------------------------------->> |
 |                                                                                                  |
 ----------------------------------------------------------------------------------------------------
*/

S32Bit FP_protocol_recv_complete_frame(FP_recv_p recv, FP_data_area_p data_area, U32Bit timeout)
{
    S32Bit ret = 0;
    U32Bit frame_data_length = 0, tmp_len = 0;

    /* forhead */
    ret = FP_protocol_recv_forhead(recv, timeout);
    if (ret == FP_OK)
    {
        /* FP_frame_head_t */
        ret = FP_device_read_data((U8Bit *)recv + sizeof(s_forhead),
                                  sizeof(FP_frame_head_t) - sizeof(s_forhead), timeout);
        if (FP_OK == ret)
        {
            /* frame_head checksum */
            ret = FP_protocol_checkout_recv_head_checksum((FP_frame_head_p)recv);
            if (FP_OK == ret)
            {
                /* length */
                tmp_len = recv->frame_head.length;
                frame_data_length = (U32Bit)(*(U8Bit *)&tmp_len) * 0x100 + *((U8Bit *)&tmp_len + 1);

                /* FP_recv_t and data_area and check sum */
                ret = FP_device_read_data((U8Bit *)recv + sizeof(FP_frame_head_t), frame_data_length, timeout);
                if (FP_OK == ret)
                {
                    /*second checksum */
                    ret = FP_protocol_checkout_recv_data_checksum(recv);
                    if (FP_OK == ret)
                    {
                        data_area->data = (U8Bit *)recv + sizeof(FP_recv_t);
                        data_area->length = FP_protocol_get_recv_data_length(recv);
                    }
                }
            }
        }
    }
    return ret;
}
