#include "fingerprint_type.h"
#include <Arduino.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef ENABLE_DEBUG_FINGER
#define LOG(X)             \
    {                      \
        Serial.println(X); \
    }
#define LOGINIT(...) Serial.begin(57600, SERIAL_8N1, 3, 1);
#define LOGF(...) Serial.printf(__VA_ARGS__);
#else
#define LOGINIT(...) ;
#define LOG(...) ;
#define LOGF(...) ;
#endif // DEBUG

    typedef enum command_type
    {
        cmd_fingerprint = 0x01,
        cmd_system = 0x02,
        cmd_maintenance = 0x03
    } FP_cmd_type;

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

/******************************************   errorCode   ***************************************/
///@see users manual -  6.1 Error Information Definition Table   page 53
#define COMP_CODE_OK (0x00)
#define COMP_CODE_UNKNOWN_CMD (0x01)
#define COMP_CODE_CMD_DATA_LEN_ERROR (0x02)
#define COMP_CODE_CMD_DATA_ERROR (0x03)
#define COMP_CODE_CMD_NOT_FINISHED (0x04)
#define COMP_CODE_NO_REQ_CMD (0x05)
#define COMP_CODE_SYS_SOFT_ERROR (0x06)
#define COMP_CODE_HARDWARE_ERROR (0x07)
#define COMP_CODE_NO_FINGER_DETECT (0x08)
#define COMP_CODE_FINGER_EXTRACT_ERROR (0x09)
#define COMP_CODE_FINGER_MATCH_ERROR (0x0A)
#define COMP_CODE_STORAGE_IS_FULL (0x0B)
#define COMP_CODE_STORAGE_WRITE_ERROR (0x0C)
#define COMP_CODE_STORAGE_READ_ERROR (0x0D)
#define COMP_CODE_UNQUALIFIED_IMAGE_ERROR (0x0E)
#define COMP_CODE_SAME_ID (0x0F)
#define COMP_CODE_IMAGE_LOW_COVERAGE_ERROR (0x10)
#define COMP_CODE_CAPTURE_LARGE_MOVE (0x11)
#define COMP_CODE_CAPTURE_NO_MOVE (0x12)
#define COMP_CODE_STORAGE_REPEAT_FINGERPRINT (0x13)
#define COMP_CODE_CAPTURE_IMAGE_FAIL (0x14)
#define COMP_CODE_FORCE_QUIT (0x15)
#define COMP_CODE_NONE_UPDATE (0x16)
#define COMP_CODE_INVALID_FINGERPRINT_ID (0x17)
#define COMP_CODE_GAIN_ADJUSTMENT_FAILED (0x18)
#define COMP_CODE_DATA_BUFFER_OVERFLOW (0x19)
#define COMP_CODE_OTHER_ERROR (0xFF)

    static S32Bit FP_action_get_errorCode(U8Bit *buffer);

    extern U8Bit dataBuffer[];
    extern U8Bit answerDataLength;
    extern S32Bit errorCode;
    extern int sum;
    extern S16Bit timeout;
    extern U8Bit txHeader[];
    extern bool fingerInterrupt;
 
#ifdef __cplusplus
}
#endif
