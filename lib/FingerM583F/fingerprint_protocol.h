#include "fingerprint_type.h"
#include <Arduino.h>

#ifdef __cplusplus   
extern "C" {   
#endif


#ifdef ENABLE_DEBUG_FINGER
HardwareSerial Log(DEBUG_PORT); 
#define LOG(X) { Log.println( X );}
#define LOGF(...) Log.printf( __VA_ARGS__ );
#else
#define LOG(...);
#define LOGF(...);
#endif // DEBUG

typedef  struct 
{
    U8Bit  *data;
    U32Bit length;
}__attribute__ ((packed)) FP_data_area_t, *FP_data_area_p;

typedef  struct 
{
    U8Bit  forhead[8]= {0xF1,0x1F,0xE2,0x2E,0xB6,0x6B,0xA8,0x8A};
    U16Bit length=0;
    U8Bit  checksum=0;
}__attribute__ ((packed)) FP_frame_head_t, *FP_frame_head_p;

typedef  struct 
{
    FP_frame_head_t frame_head{};
    U32Bit pwd;
    U8Bit  cmd_type;
    U8Bit  cmd_word;
}__attribute__ ((packed))FP_send_t, *FP_send_p;

typedef  struct 
{
    FP_frame_head_t frame_head;
    U32Bit pwd;
    U8Bit  cmd_type;
    U8Bit  cmd_word;
    U8Bit  error_code[4];
}__attribute__ ((packed)) FP_recv_t, *FP_recv_p;

typedef enum
{
    sleep_type_normal = 0,
    sleep_type_deep   = 1
}FP_sleep_type;


/*
 --------------------------------------------------------------------------------------------------------------------
 |                         |            |          |              |          |          |               |          |
 |          forhead        |            |          |   password   |          |          |               |          |
 |                         |   length   | checksum |              | cmd_type | cmd_word |   data_area   | checksum |
 | F1 1F E2 2E B6 6B A8 8A |            |          |  0x00000000  |          |          |               |          |
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
 |          forhead        |            |          |   password   |          |          |           |           |          |
 |                         |   length   | checksum |              | cmd_type | cmd_word | errorCode | data_area | checksum |
 | F1 1F E2 2E B6 6B A8 8A |            |          |  0x00000000  |          |          |           |           |          |
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




/******************************************   errorCode   ***************************************/
#define COMP_CODE_OK                      (0x00)
#define COMP_CODE_UNKNOWN_CMD             (0x01)
#define COMP_CODE_CMD_DATA_LEN_ERROR      (0x02)
#define COMP_CODE_CMD_DATA_ERROR          (0x03)
#define COMP_CODE_CMD_NOT_FINISHED        (0x04)
#define COMP_CODE_NO_REQ_CMD              (0x05)
#define COMP_CODE_SYS_SOFT_ERROR          (0x06)
#define COMP_CODE_HARDWARE_ERROR          (0x07)
#define COMP_CODE_NO_FINGER_DETECT        (0x08)
#define COMP_CODE_FINGER_EXTRACT_ERROR    (0x09)
#define COMP_CODE_FINGER_MATCH_ERROR      (0x0A)
#define COMP_CODE_STORAGE_IS_FULL         (0x0B)
#define COMP_CODE_STORAGE_WRITE_ERROR     (0x0C)
#define COMP_CODE_STORAGE_READ_ERROR      (0x0D)
#define COMP_CODE_UNQUALIFIED_IMAGE_ERROR (0x0E)
#define COMP_CODE_SAME_ID				  (0x0F)
#define COMP_CODE_IMAGE_LOW_COVERAGE_ERROR      (0x10)  //2����????yD?
#define COMP_CODE_CAPTURE_LARGE_MOVE            (0x11)  //��??����??��1y�䨮
#define COMP_CODE_CAPTURE_NO_MOVE               (0x12)  //��??����??��1yD?
#define COMP_CODE_STORAGE_REPEAT_FINGERPRINT    (0x13)  //???��????
#define COMP_CODE_CAPTURE_IMAGE_FAIL            (0x14)  //2����?����㨹
#define COMP_CODE_FORCE_QUIT                    (0x15)  //????��?3?
#define COMP_CODE_NONE_UPDATE                   (0x16)  //??��D?��D?
#define COMP_CODE_INVALID_FINGERPRINT_ID        (0x17)  //?TD��????ID
#define COMP_CODE_DATA_BUFFER_OVERFLOW          (0x18)  //��y?Y?o3???��?3?
#define COMP_CODE_OTHER_ERROR                   (0xFF)  //????�䨪?��


/* get fingerprint frame*/
S32Bit FP_protocol_get_fp_enroll_start_frame(FP_send_p send, U8Bit index);
S32Bit FP_protocol_get_fp_enroll_result_frame(FP_send_p send);
S32Bit FP_protocol_get_fp_enroll_save_start_frame(FP_send_p send, U16Bit id);
S32Bit FP_protocol_get_fp_enroll_save_result_frame(FP_send_p send);
S32Bit FP_protocol_get_fp_enroll_cancel_frame(FP_send_p send);
S32Bit FP_protocol_get_fp_match_start_frame(FP_send_p send);
S32Bit FP_protocol_get_fp_match_result_frame(FP_send_p send);
S32Bit FP_protocol_get_fp_delete_start_frame(FP_send_p send, S16Bit id);
S32Bit FP_protocol_get_fp_delete_result_frame(FP_send_p send);
S32Bit FP_protocol_get_fp_is_touch_sensor_frame(FP_send_p send);
S32Bit FP_protocol_get_fp_enroll_verify_start_frame(FP_send_p send);
S32Bit FP_protocol_get_fp_enroll_verify_result_frame(FP_send_p send);

S32Bit FP_protocol_get_fp_update_start_frame(FP_send_p send, S16Bit updateID);
S32Bit FP_protocol_get_fp_update_result_frame(FP_send_p send);

S32Bit FP_protocol_auto_enroll_frame(FP_send_p send , void * data);
S32Bit FP_protocol_match_syn_frame(FP_send_p send);
S32Bit FP_protocol_delete_syn_frame(FP_send_p send , S16Bit id);


/* get system frame */
S32Bit FP_protocol_get_sys_reset_frame(FP_send_p send);
S32Bit FP_protocol_get_sys_sleep_frame(FP_send_p send, FP_sleep_type type);

S32Bit FP_protocol_get_sys_pulse_frame(FP_send_p send);

/* get maintenace frame */
S32Bit FP_protocol_get_mtnce_read_id_frame(FP_send_p send);

S32Bit FP_protocol_get_fp_is_id_exist_frame(FP_send_p send, U16Bit id);

/* send mesg */
void FP_protocol_send_mesg(FP_send_p send, U32Bit timeout);
S32Bit FP_protocol_send_delete_mesg(FP_send_p send, S32Bit id_len, U32Bit timeout);

/* recv complete frame */
S32Bit FP_protocol_recv_complete_frame(FP_recv_p recv,FP_data_area_p data_area, U32Bit timeout);


#ifdef __cplusplus   
}   
#endif
