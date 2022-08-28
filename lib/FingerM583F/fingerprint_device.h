#include <Arduino.h>

#ifndef FINGERPRINT_DEVICE_H
#define FINGERPRINT_DEVICE_H
#define FINGER_PORT 0

#include "fingerprint_type.h"

#ifdef __cplusplus   
extern "C" {   
#endif

extern  S8Bit sum;
extern  U8Bit timeout;

/// txHeader + txHeader lenght
extern U8Bit txHeader[];

void commFingerInit(unsigned long  baud);
void FP_device_write_data(U8Bit *data, size_t length);
S32Bit FP_device_read_one_byte(U8Bit * data);
void writeBufferPlusCheckSum(U8Bit *data, size_t length);
void writeBuffer(U8Bit *data, size_t length);
void sendCommandHeader(U8Bit commandHigh,U8Bit commandLow);

#ifdef __cplusplus  
}
#endif

#endif //FINGERPRINT_DEVICE_H

