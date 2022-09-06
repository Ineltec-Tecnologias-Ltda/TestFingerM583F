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
extern U8Bit rtxCommandLow;

bool heartbeat();
bool readId();
bool autoEnroll();
bool matchTemplate();
bool ledControl(uint8_t *params);
bool moduleReset();

extern S16Bit debugRxState;

/// Commands and command size
// @ see Command set summary pages 9-12
extern Command AutoEnroll;
extern Command HeartBeat;
extern Command LedControl;
extern Command ReadId;
extern Command MatchTemplate;
extern Command MatchResult;
extern Command FingerIsTouch;
extern Command Enroll;
extern Command EnrollResult;
extern Command ModuleReset;
extern Command SendTemplateStart;
extern Command SendTemplateData; // 0x89 is the maximum to be sent at each packet
extern Command ReceiveTemplateStart;
extern Command ReceiveTemplateData;
extern Command DeleteTemplates;
extern Command GetAllSlotStatus;

extern bool receiveCompleteResponse();