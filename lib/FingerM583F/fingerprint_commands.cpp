#include <Arduino.h>
#include "fingerprint_type.h"
#include "fingerprint_protocol.h"
#include "fingerprint_device.h"
#include "fingerprint_commands.h"

/// @brief Slot Id info no Finger Lib
U8Bit slotID;
const char *errorMessage;

const char *EnrollOk = "Enroll OK";
const char *Enrolling = "Enrolling...";
const char *TimeoutError = "Timeout Error...";
const char *TryAgain = "Please Try Again";
const char *NoFinger = "No Finger detected!!";
const char *NoMatch = "No Match";

/// Command codes and extra data size
/// @see users manual Command set summary pages 9-12
Command Sleep{cmd_system, sys_sleep, 1};
Command AutoEnroll{cmd_fingerprint, fp_auto_enroll, 4};
Command HeartBeat{cmd_maintenance, maintenance_heart_beat, 0};
Command LedControl{cmd_system, sys_set_led, 5}; // @see users manual page 45
Command ReadId{cmd_maintenance, maintenance_read_id, 0};
Command MatchTemplate{cmd_fingerprint, fp_match_start, 0};
Command MatchResult{cmd_fingerprint, fp_match_result, 0};
Command FingerIsTouch{cmd_fingerprint, fp_query_slot_status, 0};
Command Enroll{cmd_fingerprint, fp_enroll_start, 1};
Command EnrollResult{cmd_fingerprint, fp_enroll_result, 0};
Command EnrollCancel{cmd_fingerprint, fp_enroll_cancel, 0};
Command ModuleReset{cmd_system, sys_reset, 0};
Command SendTemplateStart{cmd_fingerprint, fp_start_send_template, 4};   // @see users manual page 36
Command SendTemplateData{cmd_fingerprint, fp_send_template_data, 0x89};  // 0x89 is the maximum to be sent at each packet
Command ReceiveTemplateStart{cmd_fingerprint, fp_start_get_template, 2}; // @see users manual page 38
Command ReceiveTemplateData{cmd_fingerprint, fp_get_template_data, 2};
Command DeleteTemplates{cmd_fingerprint, fp_delete_templates, 3}; // @see users manual page 33
Command GetAllSlotStatus{cmd_fingerprint, fp_get_all_slots_status, 0};
Command GetSlotsWithData{cmd_system, sys_get_nb_templates, 0}; // @see users manual page 41

/// @brief Sends Commands with fixed extra data, and receives response from module
// @see page Command set summary on pages 9 to 12 on users manual
/// @param command Only commands with fixed extra data bytes after header
/// For commands with extra data bytes "dataBuffer" has to be filled with data( starting at index 6)
///  first 6 bytes are added by protocol methods with check password (4)+ command(2)
/// @return if true, sets "dataBuffer" and "answerDataLength" according to received data
///   Places all received data to "dataBuffer" starting at index 0
/// if false errorCode and  errorMessage are set
bool sendCommandReceiveResponse(Command command)
{
  sendCommandHeader(command, command[2]);
  writeBufferPlusCheckSum(command[2]);
  delay(100);
  return receiveCompleteResponse();
}

/// @brief Sends Commands with variable extra data, and receives response from module
// @see page Command set summary on pages 9 to 12 on users manual
/// @param command  Commands with variable extra data bytes after header(like 5.21 Fingerprint feature data download)
/// @param length number of extra bytes to send after
/// "dataBuffer" has to be filled with data( starting at index 6) to be sent
///  first 6 bytes are added by protocol methods with check password (4)+ command(2)
/// @return if true, sets "dataBuffer" and "answerDataLength" according to received data
///   Places all received data to "dataBuffer" starting at index 0
/// if false "errorCode" and  "errorMessage" are set
bool sendCommandReceiveResponse(Command command, U8Bit length)
{
  sendCommandHeader(command, length);
  writeBufferPlusCheckSum(length);
  delay(100);
  return receiveCompleteResponse();
}

/// @brief Tests if Finger Module is responsive
/// @see Users Manual page 49
/// This is an example of how to send a command without aditional data
/// @return  true if command was accepted from module
/// if false "errorCode" and  "errorMessage" are set
bool heartbeat()
{
  return sendCommandReceiveResponse(HeartBeat);
}

/// @brief This is an example of how to send a command with aditional data
/// @param params == 5 bytes as described on  Users Manual page 45
/// "dataBuffer" (starting at index 6) is filled with "params"
///  first 6 bytes are added by protocol methods with check password (4)+ command(2)
/// @return  true if command was accepted from module
/// if false "errorCode" and  "errorMessage" are set
bool ledControl(uint8_t *params)
{
  memcpy(dataBuffer + 6, params, LedControl[2]);
  return sendCommandReceiveResponse(LedControl);
}

/// @brief This is an example of how to send a command without aditional data
/// @return  true if command was accepted from module
/// if false "errorCode" and  "errorMessage" are set
bool moduleReset()
{
  commFingerInit(57600);
  if (sendCommandReceiveResponse(ModuleReset))
  {
    Log("Module reseted!!!");
    // Flashing red LED light
    uint8_t buffer[] = {4, 2, 30, 10, 1};
    ledControl(buffer);
    return true;
  }
  return false;
}

/// @brief see Users Manual page 48
/// This is an example of how to send a command without aditional data and receive data from module
/// @return  true if command was accepted from module,  "errorCode" has to be == 0 and "answerDataLength" > 0
/// if false "errorCode" and  "errorMessage" are set
bool readId()
{
  if (sendCommandReceiveResponse(ReadId) == true && errorCode == FP_OK && answerDataLength > 0)
  {
    debugRxState = -1000;
    /* gets Ascii value module id */
    dataBuffer[answerDataLength] = 0;
    LogF("Module Id...: %s\r\n", dataBuffer);
    return true;
  }
  LogF("Module Id Error:  0x%04X\r\n", errorCode);
  return false;
}

bool fingerDetection()
{
  Log("Waiting for Finger...");
  int timeout = 600;
  fingerInterrupt = false;

  uint8_t buffer[] = {3, 3, 100, 100, 2};
  ledControl(buffer);
  while (timeout-- > 0)
  {
    //  esp_task_wdt_reset();
    if (fingerInterrupt)
    {
      fingerInterrupt = false;
      delay(30);
      // @see Users Manual page 32
      if (sendCommandReceiveResponse(FingerIsTouch) == true && errorCode == FP_OK)
        if (dataBuffer[0] == 1)
        { // Finger is placed on module
          Log("Finger detected!!");
          return true;
        }
      delay(30);
    }
    vTaskDelay(10);
  }
  // Turn LED off
  uint8_t buffer1[] = {0, 0, 0, 0, 0};
  ledControl(buffer1);
  delay(30);

  errorMessage = NoFinger;
  errorCode = COMP_CODE_NO_FINGER_DETECT;
  Log(errorMessage);
  return false;
}

/// @brief Returns true if match ok
// @see Users Manual pages 22 and 23
/// @param messageBuffer Buffer for feedback message to user
/// @param slot  if  0xFFFF ,will be automatically assigned by the system
/// @param cancel to to stop enroll
/// @param callBack to report enroll progression
/// @return if true "slotID" is where the saved finger template was saved inside module
/// if false errorCode and  errorMessage are set
bool autoEnroll(char *messageBuffer, U16Bit slot, bool cancel, void (*callBack)(int stage))
{
  if (!fingerDetection())
  {
    sprintf(messageBuffer, "No Finger detected!!");
    return false;
  }

  errorMessage = TryAgain;

  sendCommandHeader(AutoEnroll, AutoEnroll[2]);

  // enrollPara.enroll_mode = 0x01 will indicate that user must lift finger and press again during enrollment
  dataBuffer[6] = 1;

  // enrollPara.times is the number of presses (can be set to 1~6 times)
  dataBuffer[7] = 4;

  // enrollPara.slotID = 0xFFFF ,will be automatically assigned by the system
  dataBuffer[8] = slot >> 8;
  dataBuffer[9] = (U8Bit)slot;
  writeBufferPlusCheckSum(AutoEnroll[2]);

  ulong tmp = millis();

  Log("enroll...");
  bool timeoutError = false;
  while (!cancel && !timeoutError)
  {
    if (millis() - tmp >= (ulong)8000)
      timeoutError = true;
    if (receiveCompleteResponse())
    {
      if (errorCode == 0)
      {
        if (answerDataLength == 4)
        {
          LogF("State: %d    Enroll Progress: %d %\r\n", dataBuffer[0], dataBuffer[3]);
          if (dataBuffer[3] == 100)
          {
            if (dataBuffer[0] == 0xff)
            {
              slotID = dataBuffer[2];
              LogF("Template slot: %d\r\n", slotID);
              sprintf(messageBuffer, "Template enrolled on slot: %d", slotID);
              errorMessage = EnrollOk;
              delay(100);
              moduleReset();
              return true;
            }
            else
              waitCharAvailable();
          }
          else
          {
            errorMessage = Enrolling;
            Log("Reposition Finger!!");
            callBack(dataBuffer[3]);
            waitCharAvailable();
          }
        }
        else
          LogF("State: %d %\r\n", dataBuffer[0]);
      }
      else if (errorCode == FP_DEVICE_TIMEOUT_ERROR)
      {
        Log("Response Timeout...");
        delay(100);
        continue;
      }
      else if (errorCode == COMP_CODE_SAME_ID)
      {
        Log("Template already exists")
            sprintf(messageBuffer, "Template already exists");
        timeoutError = false;
        break;
      }
      else
      {
        errorMessage = TryAgain;
        if (errorCode == COMP_CODE_NO_FINGER_DETECT)
        {
          Log(NoFinger);
          delay(100);
        }
        else
        {
          sprintf(messageBuffer, "Enroll Error:  0x%04X\r\n", errorCode);
          timeoutError = false;
          break;
        }
      }
    }
    else
    {
      // TODO implement callback to show Enroll Progress!!!
      Log("Place Finger!!");
      delay(100);
    }
  }

  delay(200);
  sendCommandReceiveResponse(EnrollCancel);
  delay(100);
  moduleReset();

  if (timeoutError)
  {
    Log("Timeout Error");
    errorMessage = TimeoutError;
    sprintf(messageBuffer, "Timeout Error");
  }

  return false;
}

/// Returns true if match ok, and sets "slotId" with template position inside finger module
// otherwise sets errorCode and errorMessage
// @see Users Manual pages 23 and 24
bool matchTemplate(char *messageBuffer)
{
  errorMessage = TryAgain;
  if (!fingerDetection())
  {
    sprintf(messageBuffer, NoFinger);
    return false;
  }

  int retry = 10;
  bool start = true;
  errorCode = 0;
  delay(200);

  while (retry-- > 0)
  {
    if (start)
    {
      sum = 0;
      if (sendCommandReceiveResponse(MatchTemplate) && errorCode == 0)
      {
        retry = 10;
        start = false;
      }
      else
        errorCode = 0;

      delay(300);
    }
    else
    {
      sum = 0;
      if (!sendCommandReceiveResponse(MatchResult))
        delay(100);
      else
      {
        if (errorCode == 0)
        {
          if (dataBuffer[1] == 1)
          {
            // pass ok == 1)
            uint16_t score = (uint16_t)dataBuffer[2] << 8 + dataBuffer[3];
            slotID = dataBuffer[5]; // slotID;
            LogF(" Match ok   Score: %d   SlotId: %d\r\n", score, slotID);
            return true;
          }
          else
          {
            errorMessage = NoMatch;
            sprintf(messageBuffer, "No Match");
            return false;
          }
        }
        else if (errorCode == COMP_CODE_CMD_NOT_FINISHED || errorCode == FP_DEVICE_TIMEOUT_ERROR)
          delay(200);
        else
        {
          sprintf(messageBuffer, "Match Error:  0x%04X\r\n", errorCode);
          return false;
        }
      }
    }
  }
  sprintf(messageBuffer, "No Module response");
  Log("Timeout Error");
  errorMessage = TimeoutError;
  errorCode = FP_DEVICE_TIMEOUT_ERROR;
  return false;
}

/// @brief This method Gets a template from Finger module sets "templateRxLen" and "templateRx"
/// @param slotId  Template slot to get data
/// @param templateRx  buffer to place received template data
/// @param templateRxLen pointer to received template data length
/// @param messageBuffer Buffer for feedback message to user
/// @return true complete template received ok from Finger Module
bool RxTemplate(int slotId, char *templateRx, U16Bit *templateRxLen, char *messageBuffer)
{

  int i = 0;
  // template slot id
  dataBuffer[6] = 0;
  dataBuffer[7] = slotId;
  if (sendCommandReceiveResponse(ReceiveTemplateStart) && errorCode == FP_OK && answerDataLength > 0)
  {
    U16Bit templateSize = (((U16Bit)dataBuffer[0]) << 8) + dataBuffer[1];
    U16Bit templateSizeSaved = templateSize;
    U8Bit maxFrames = (templateSize / 128);
    if (templateSize % 128 == 0)
      --maxFrames;
    LogF("Template size: %d   frames to Rx: %d\r\n", templateSize, maxFrames);
    delay(100);
    if (templateSize > 64)
    {
      U16Bit index = 0;
      U8Bit frame = 0;
      int retry = 10;
      bool resp = false;
      *templateRxLen = 0;

      while (retry-- > 0 && frame <= maxFrames)
      {
        dataBuffer[6] = 0;
        dataBuffer[7] = frame;

        resp = sendCommandReceiveResponse(ReceiveTemplateData);
        if (resp && errorCode == FP_OK && answerDataLength > 2 && rtxCommandLow == 0x54 &&
            dataBuffer[0] == 0 && dataBuffer[1] == frame)
        {
          if (frame == 0 || frame == maxFrames) // Print to log first  and  last frames
          {
            Log("First or last template frame");
            i = 2; // template data starts after frame counter
            while (i < answerDataLength && templateSize-- > 0)
            {
              LogF("%02X ", dataBuffer[i]);
              templateRx[index++] = dataBuffer[i++];
            }
            Log(" ");
          }
          else
          {
            answerDataLength -= 2;
            memcpy(templateRx + index, dataBuffer + 2, answerDataLength);
            index += answerDataLength;
            templateSize -= 128;
            LogF("Received frame %d\r\n", frame);
          }

          delay(5);
          retry = 4;
          frame++;
        }
        else
        {
          LogF("frame: %d   resp:%s error:%04X  answerDataLength : %d\r\n", frame, resp ? "true" : "false", resp ? errorCode : 0, resp ? answerDataLength : 0);
          delay(200);
        }
      }
      if (retry > 0)
      {
        *templateRxLen = templateSizeSaved;
        return true;
      }
      else
        sprintf(messageBuffer, "Error getting template");
    }
    else
      sprintf(messageBuffer, "No Template on slot %d", slotId);
  }
  return false;
}

/// @brief This method Sends a template to Finger module
/// @param slotId  Template slot to send data
/// @param templateRx  buffer to get template data
/// @param templateRxLen pointer to template data length
/// @param messageBuffer Buffer for feedback message to user
/// @return true complete template sent ok to Finger module
bool TxTemplate(int slotId, char *templateRx, U16Bit *templateRxLen, char *messageBuffer)
{
  // template slot id
  dataBuffer[6] = 0;
  dataBuffer[7] = slotId;

  // Total template lenght to send
  dataBuffer[8] = *templateRxLen / 256;
  dataBuffer[9] = *templateRxLen % 256;
  if (sendCommandReceiveResponse(SendTemplateStart) && errorCode == FP_OK)
  {
    U16Bit templateSizeSaved = *templateRxLen;
    U8Bit maxFrames = *templateRxLen / 128;
    if (*templateRxLen % 128 == 0)
      --maxFrames;
    U8Bit frame = 0;
    U8Bit lenTx = 130;
    U16Bit totalLen = *templateRxLen;
    U16Bit index = 0;
    int retry = 4;

    int i = 0;
    LogF("Template size: %d ,   Sending %d frames to slot %d\r\n", templateRxLen, maxFrames, slotId);
    while (frame <= maxFrames && retry-- > 0)
    {
      dataBuffer[6] = 0;
      dataBuffer[7] = frame;
      memcpy(dataBuffer + 8, templateRx + index, 128);

      if (templateSizeSaved < 128) // Last frame < 128 bytes
        lenTx = templateSizeSaved + 2;

      if (sendCommandReceiveResponse(SendTemplateData, lenTx) && errorCode == FP_OK)
      {
        if (frame == 0 || frame == maxFrames) // Print to log first  and last frames
        {
          i = 0;
          Log("First or last template frame");
          while (i++ < 128 && templateSizeSaved-- > 0)
          {
            LogF("%02X ", templateRx[index]);
            index++;
          }
          Log(" ");
        }
        else
        {
          templateSizeSaved -= 128;
          index += 128;
        }

        LogF("Sent frame %d   ", frame);
        frame++;
        totalLen -= 128;
        retry = 5;
      }
    }
    if (retry > 0)
      return true;
    else
      sprintf(messageBuffer, "Error sending template");
  }
  else
    sprintf(messageBuffer, "No Module response");
  return false;
}

/// @brief Gets slots states on module
/// @param messageBuffer
/// @return  if true  dataBuffer[1] == number os templates on module
///  if dataBuffer[1] < 100 , "slotID" is the first free slot if <> 0xff
bool getSlotInfos(char *messageBuffer)
{
  slotID = 0xff;
  if (!sendCommandReceiveResponse(GetSlotsWithData) || errorCode != FP_OK)
  {
    sprintf(messageBuffer, "No Module response");
    return false;
  }

  U8Bit templates = dataBuffer[1];
  if (templates == 100)
  {
    sprintf(messageBuffer, "No free template slots on Finger Module");
    return false;
  }
  else
  {
    delay(100);
    if (sendCommandReceiveResponse(GetAllSlotStatus) && errorCode == FP_OK && answerDataLength > 0)
    {
      int i = 2;
      LogF("%d Templates on Module\r\n", templates);
      Log("Slot Map:");
      U8Bit pos = 0, j = 0, k = 0;
      while (answerDataLength-- > 0)
      {
        pos = dataBuffer[i++];
        LogF("%02X ", pos);
        for (k = 0; k < 8; k++)
        {
          if ((pos & 1) == 0)
          {
            slotID = j;
            LogF("\r\nFirst Free Slot: %d\r\n", j);
            return true;
          }
          pos = pos >> 1;
          j++;
        }
      }
    }
    return false;
  }
}
