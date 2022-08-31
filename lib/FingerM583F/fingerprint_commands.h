#include "fingerprint_type.h"

extern const char *errorMessage;
extern U8Bit slotID;
extern S32Bit errorCode;

extern const char *ssid;
extern const char *password;
extern const char *EnrollOk;
extern const char *Enrolling;
extern const char *TimeoutError;
extern const char *TryAgain;

extern U8Bit dataBuffer[];
extern U8Bit answerDataLength;

bool heartbeat();
bool readId();
bool autoEnroll();
bool matchTemplate();
bool ledControl(uint8_t *params);
bool moduleReset();

extern S16Bit debugRxState;