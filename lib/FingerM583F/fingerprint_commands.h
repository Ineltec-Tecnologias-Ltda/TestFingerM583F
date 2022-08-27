#include "fingerprint_type.h"

extern const char *errorMessage;
extern   U16Bit slotID;
extern S32Bit errorCode;

extern const char *ssid;
extern const char *password;
extern const char *EnrollOk;
extern const char *Enrolling;
extern const char *TimeoutError;
extern const char *TryAgain;

bool heartbeat();
char* readId();
bool autoEnroll();
bool matchTemplate();
