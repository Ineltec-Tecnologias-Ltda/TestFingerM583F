
#ifndef FINGERPRINT_DEVICE_H
#define FINGERPRINT_DEVICE_H

#include "fingerprint_type.h"

#ifdef __cplusplus   
extern "C" {   
#endif

void commFingerInit(unsigned long  baud);
S32Bit FP_device_read_data(U8Bit * data, size_t length, U32Bit timeout);
void FP_device_write_data(U8Bit *data, size_t length);
S32Bit FP_device_read_one_byte(U8Bit * data, U32Bit timeout);

#ifdef __cplusplus  
}
#endif

#endif //FINGERPRINT_DEVICE_H

