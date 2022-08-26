
#ifndef FINGERPRINT_ACTION_H
#define FINGERPRINT_ACTION_H

#include "fingerprint_type.h"
#include "fingerprint_protocol.h"

#ifdef __cplusplus   
extern "C" {   
#endif

extern U8Bit rxData[128];
extern FP_data_area_t bufferRx;

typedef struct {
    U8Bit * ID;
    U32Bit length;
} FP_moduleID_t, * FP_moduleID_p;

typedef struct {
    U8Bit enroll_mode;
    U8Bit times;
	U16Bit slotID;
} FP_auto_enroll_t, *FP_auto_enroll_p;

typedef struct {
    U8Bit state;
    U16Bit slotID;
    U8Bit progress;
} FP_enrollResult_t, *FP_enrollResult_p;

typedef struct {
    U16Bit isPass;
    U16Bit matchScore;
    U16Bit slotID;
} FP_matchResult_t, *FP_matchResult_p;

void Heartbeat();
void FP_action_set_send_buffer(U8Bit *buffer);
void FP_action_set_recv_buffer(U8Bit *buffer);

S32Bit FP_protocol_fp_auto_enroll_frame(FP_send_p send , FP_auto_enroll_t enrollPara);

S32Bit FP_protocol_fp_match_syn_frame(FP_send_p send);

S32Bit FP_protocol_fp_delete_syn_frame(FP_send_p send , S16Bit id);

/* 
 * module ID
 */
FP_data_area_t FP_action_getID();

/*
 * finger touch
 */
S32Bit FP_action_check_finger_is_touch(U8Bit * isTouch, U32Bit * errorCode);

/*
 * enroll
 */
S32Bit FP_action_enroll_start(U8Bit index, U32Bit * errorCode);
S32Bit FP_action_get_enroll_result(FP_enrollResult_p enrollResult, U32Bit * errorCode);

/*
 * save
 */
S32Bit FP_action_save_start(S16Bit id, U32Bit * errorCode);

S32Bit FP_action_get_save_result(U32Bit * errorCode, U16Bit *SaveID);


/*
 * delete
 */
S32Bit FP_action_delete_start(S16Bit id, U32Bit * errorCode);
S32Bit FP_action_get_delete_result(U32Bit * errorCode);

/*
 * match
 */
S32Bit FP_action_match_start();
S32Bit FP_action_get_match_result(FP_matchResult_p matchResult);


/*
 * sleep
 */
S32Bit FP_action_sleep(FP_sleep_type type);


/*
 * reset
 */
S32Bit FP_action_reset(U32Bit * errorCode);

/*
 * update
 */
S32Bit FP_action_update_start(S16Bit updateID);
S32Bit FP_action_get_update_result();

/*
 * auto enroll
 */
S32Bit FP_action_auto_enroll_send(FP_auto_enroll_t enrollPara);
S32Bit FP_action_auto_enroll_recv(FP_enrollResult_p enrollResult);

/*
 * match syn
 */
S32Bit FP_action_match_syn_send(void);
S32Bit FP_action_match_syn_recv(FP_matchResult_p matchResult);


/*
 * delete syn
 */
S32Bit FP_action_delete_syn_send(S16Bit id);
S32Bit FP_action_delete_syn_recv(U32Bit * errorCode);



#ifdef __cplusplus   
}   
#endif


#endif //FINGERPRINT_ACTION_H

