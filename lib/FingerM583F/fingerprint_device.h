#include <Arduino.h>

#ifndef FINGERPRINT_DEVICE_H
#define FINGERPRINT_DEVICE_H
#define FINGER_PORT 0

#include "fingerprint_type.h"

#ifdef __cplusplus   
extern "C" {   
#endif


void commFingerInit(unsigned long  baud);
S32Bit FP_device_read_one_byte(U8Bit * data);
void writeBufferPlusCheckSum(unsigned char length);
void sendCommandHeader(Command command,const unsigned char length);
bool sendCommandReceiveResponse(Command command);

extern int sum;
extern U8Bit sumTxDebug;
#ifdef __cplusplus  
}
#endif

#endif //FINGERPRINT_DEVICE_H

