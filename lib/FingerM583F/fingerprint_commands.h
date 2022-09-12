#include "fingerprint_type.h"

extern const char *errorMessage;
extern U8Bit slotID;

extern const char *ssid;
extern const char *password;
extern const char *EnrollOk;
extern const char *Enrolling;
extern const char *TimeoutError;
extern const char *TryAgain;
extern U8Bit rtxCommandLow;

extern bool sendCommandReceiveResponse(Command command);
extern bool sendCommandReceiveResponse(Command command, U8Bit length);
extern bool receiveCompleteResponse();

bool heartbeat();
bool readId();
bool autoEnroll( char *messageBuffer);
bool matchTemplate( char *messageBuffer);
bool ledControl(uint8_t *params);
bool moduleReset();
bool RxTemplate(int slotId,char *templateRx,U16Bit *templateRxLen,char *messageBuffer );
bool TxTemplate(int slotId,char *templateRx,U16Bit *templateRxLen,char *messageBuffer );
bool getSlotInfos(char *messageBuffer );

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
extern Command GetSlotsWithData;
