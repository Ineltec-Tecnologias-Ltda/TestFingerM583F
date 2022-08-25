
#ifndef FINGERPRINT_TYPE_H
#define FINGERPRINT_TYPE_H

#ifdef __cplusplus   
extern "C" {   
#endif

typedef    signed  char        S8Bit;               
typedef  unsigned  char        U8Bit;               
typedef    signed  short       S16Bit;           
typedef  unsigned  short       U16Bit;           
typedef    signed  int         S32Bit;           
typedef  unsigned  int         U32Bit;           
typedef    signed  long  long  S64Bit;           
typedef  unsigned  long  long  U64Bit;           


/*************************** function errorCode  ***************************/
#define FP_OK                                   (0)

#define FP_DEVICE_OTHER_ERROR                   (-0x100001)
#define FP_DEVICE_OPEN_ERROR                    (-0x100002)
#define FP_DEVICE_CONFIGURE_ERROR               (-0x100003)
#define FP_DEVICE_POLL_ERROR                    (-0x100004)
#define FP_DEVICE_TIMEOUT_ERROR                 (-0x100005)
#define FP_DEVICE_READ_ERROR                    (-0x100006)
#define FP_DEVICE_WRITE_ERROR                   (-0x100007)

#define FP_PROTOCOL_UART_HEAD_CHECKSUM_ERROR    (-0x200001)
#define FP_PROTOCOL_DATA_CHECKSUM_ERROR         (-0x200002)
#define FP_PROTOCOL_SEND_DATA_LENGTH_ERROR      (-0x200003)
#define FP_PROTOCOL_RECV_DATA_LENGTH_ERROR      (-0x200004)

#define FP_ACTION_BUFFER_IS_NULL                (-0x300001)

#ifdef __cplusplus   
}   
#endif


#endif //FINGERPRINT_TYPE_H

