/*
 * This is an example of how to use the library FingerM583F
 * using VSCode+ PlatformIO extension+ Arduino + ESP32 processor module
 */
#include <Arduino.h>
#include <WiFi.h>
#include "fingerprint_commands.h"
#include "fingerprint_device.h"
#include <iostream>
#include <string>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <stdlib.h>

// Set web server port number to 80
WiFiServer server(80);

#define Log Serial

void RxTemplate(int slotId);
void TxTemplate(int slotId);
bool getSlotInfos();
String inboxText = "";
String headerHttp = "";
char messageBuffer[100];
int pos = 0;
long inboxNumber = 0;
const char *CmdSent = "Command Sent";

/// @brief Buffer to upload/download template to/from module
char templateRx[4096];
U16Bit templateRxLen = 0;

void setup()
{
  Log.begin(9600);
  commFingerInit(57600);

  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Log.print("\r\nAP IP address: ");
  Log.println(IP);

  server.begin();
}

bool getInboxText(int maxSize)
{
  inboxText = headerHttp.substring(pos, maxSize - 1);
  int pos1 = inboxText.indexOf("=") + 1;
  int pos2 = inboxText.indexOf(" ");
  if (pos2 > pos1)
  {
    inboxText = inboxText.substring(pos1, pos2);
    Log.println(inboxText);
    inboxNumber = inboxText.toInt();
    Log.printf("Typed number= %d\r\n", inboxNumber);
    return true;
  }
  else
  {
    inboxText = "";
    Log.println("Please enter data for the command");
  }

  Log.printf(" %d    %d Typed text= %s", pos1, pos2);

  return false;
}

void loop()
{
  WiFiClient client = server.available(); // Listen for incoming clients

  if (client)
  {                             // If a new client connects,
    Log.println("New Client."); // print a message out in the serial port
    String currentLine = "";    // make a String to hold incoming data from the client
    headerHttp = "";
    sprintf(messageBuffer, CmdSent);

    int i = 0;

    while (client.connected())
    { // loop while the client's connected
      if (client.available())
      {                         // if there's bytes to read from the client,
        char c = client.read(); // read a byte, then
        Log.write(c);           // print it out the serial monitor
        if (i < 50)
        { // actions to execute are on first chars
          headerHttp += c;
          i++;
        }

        if (c == '\n')
        { // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0)
          {
            Log.println();

            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            if ((pos = headerHttp.indexOf("Enroll=")) >= 0)
            {
              if (getInboxText(i) && inboxNumber > 0)
                autoEnroll();
              else
                sprintf(messageBuffer, "Must entre registration number > 0 to associate with slotId");
            }
            else if ((pos = headerHttp.indexOf("TxTemplate=")) >= 0)
            {
              Log.println("TxTemplate");
              if (templateRxLen == 0)
                sprintf(messageBuffer, "No template data to send: get one with RxTemplate");
              else if (getInboxText(i))
                TxTemplate(inboxNumber);
              else
                sprintf(messageBuffer, "Must enter slot number");
            }
            else if ((pos = headerHttp.indexOf("RxTemplate=")) >= 0)
            {
              Log.println("RxTemplate");
              if (getInboxText(i))
                RxTemplate(inboxNumber);
              else
                sprintf(messageBuffer, "Must enter slot number");
            }
            else if (headerHttp.indexOf("Match") >= 0)
            {
              if (getSlotInfos())
              {
                if (matchTemplate())
                  sprintf(messageBuffer, "Match on slot: %d", slotID);
                else
                  sprintf(messageBuffer, "%s", errorMessage);
              }
            }

            else if (headerHttp.indexOf("Heartbeat") >= 0)
            {
              if (heartbeat())
              {
                moduleReset(); // This is only to test this command
                sprintf(messageBuffer, "Finger Module ok");
              }
              else
                sprintf(messageBuffer, "No module response!!");
            }
            else if (headerHttp.indexOf("Module") >= 0)
            {
              if (readId())
                sprintf(messageBuffer, "Module Id: %s", dataBuffer);
              else
                sprintf(messageBuffer, "Failed to get Module Id");
            }
            else if (headerHttp.indexOf("Leds") >= 0)
            {
              uint8_t buffer[] = {4,    // Control mode
                                  1,    // Light color
                                  0x14, // // Parameter 1
                                  0x14, // Parameter 2
                                  5};   // Parameter 3
              ledControl(buffer);
            }
            else if (headerHttp.indexOf("Delete") >= 0)
            {
              dataBuffer[6] = 1; // Flag for delete all
              dataBuffer[7] = 0; // slot id
              dataBuffer[8] = 1; // slot id
              if (sendCommandReceiveResponse(DeleteTemplates))
                sprintf(messageBuffer, "All templates deleted");
            }
            Log.println(messageBuffer);
            Log.printf("\r\nsum debug Tx: %d\r\n", sumTxDebug);
            Log.printf("Rx debug  State: %d\r\n", debugRxState);

            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");

            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println("text-decoration: none; font-size: 20px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style>");
            client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"></head>");
            client.println("<body><h1 style=\"color:Tomato;\">Finger M583F Server</h1>");

            client.println("<form action=\"/finger.php\">");
            client.println("<input type=\"text\" id=\"Enroll\" name=\"Enroll\">");
            client.println("<input type=\"submit\" value=\"Enroll\">");
            client.println("</form>");

            client.println("<form action=\"/finger.php\">");
            client.println("<input type=\"text\" id=\"RxTemplate\" name=\"RxTemplate\">");
            client.println("<input type=\"submit\" value=\"RxTemplate\">");
            client.println("</form>");

            client.println("<form action=\"/finger.php\">");
            client.println("<input type=\"text\" id=\"TxTemplate\" name=\"TxTemplate\">");
            client.println("<input type=\"submit\" value=\"TxTemplate\">");
            client.println("</form>");

            client.println("<div class=\"btn-group\">");
            client.println(" <a href=\"Heartbeat\"><button class=\"button\">Heartbeat</button></a>");
            client.println(" <a href=\"Leds\"><button class=\"button\">Leds</button></a>");
            client.println("<a href=\"Module Id\"><button class=\"button\">Module Id</button></a>");
            client.println("</div>");

            client.println("<div class=\"btn-group\">");
            client.println(" <a href=\"Match\"><button class=\"button\">Match</button></a>");
            client.println(" <a href=\"DeleteAll\"><button class=\"button\">DeleteAll</button></a>");
            client.println("</div>");
            client.printf("<p> %s </p>\r\n", messageBuffer);

            client.println("</body></html>");

            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            headerHttp = "";
            break;
          }
          else
          { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        }
        else if (c != '\r')
        {                   // if you got anything else but a carriage return character,
          currentLine += c; // add it to the end of the currentLine
        }
      }
    }
    // Clear the headerHttp variable
    headerHttp = "";
    // Close the connection
    client.stop();
    Log.println("Client disconnected.");
  }
}

bool getSlotInfos()
{
  if (!sendCommandReceiveResponse(GetSlotsWithData) || errorCode != FP_OK)
  {
    sprintf(messageBuffer, "No Module response");
    return false;
  }

  if (dataBuffer[1] == 0)
  {
    sprintf(messageBuffer, "No templates on Finger Module");
    return false;
  }
  else
  {
    Log.printf("%d Templates on Module", dataBuffer[1]);
    if (sendCommandReceiveResponse(GetAllSlotStatus) && errorCode == FP_OK && answerDataLength > 0)
    {
      int i = 0;
      Log.println("Slot Map:");
      while (answerDataLength-- > 0)
        Log.printf("%02X ", dataBuffer[i++]);
      Log.println();
    }
    return true;
  }
}

/// @brief This method Gets a template from Finger module sets "templateRxLen" and "templateRx"
///   "templateRx" can send back to Finger module for TxTemplate() test
///
void RxTemplate(int slotId)
{
  if (getSlotInfos())
  {
    int i = 0;
    // template slot id
    dataBuffer[6] = 0;
    dataBuffer[7] = slotId;
    if (sendCommandReceiveResponse(ReceiveTemplateStart) && errorCode == FP_OK && answerDataLength > 0)
    {
      u16_t templateSize = (((u16_t)dataBuffer[0]) << 8) + dataBuffer[1];
      U8Bit maxFrames = (templateSize / 128) - 1;
      Log.printf("Template size: %d   frames to Rx: %d\r\n", templateSize, maxFrames);
      delay(100);
      if (templateSize > 64)
      {
        U16Bit index = 0;
        U8Bit frame = 0;
        int retry = 10;
        bool resp = false;
        templateRxLen = 0;

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
              Log.println("First or last template frame");
              i = 2; // template data after frame counter
              while (i < answerDataLength)
              {
                Log.printf("%02X ", dataBuffer[i]);
                templateRx[index++] = dataBuffer[i++];
              }
              Log.println();
            }
            else
            {
              answerDataLength -= 2;
              memcpy(templateRx + index, dataBuffer + 2, answerDataLength);
              index += answerDataLength;
              Log.printf("Received frame %d\r\n", frame);
            }

            delay(5);
            retry = 4;
            frame++;
          }
          else
          {
            Log.printf("frame: %d   resp:%s error:%04X  answerDataLength : %d\r\n", frame, resp ? "true" : "false", resp ? errorCode : 0, resp ? answerDataLength : 0);
            delay(200);
          }
        }
        if (retry > 0)
        {
          templateRxLen = templateSize;
          sprintf(messageBuffer, "Now we have a template to send back to Finger module to test TxTemplate()");
        }
      }
    }
  }
}

/// @brief To test send template data to some other slot
/// @param slotId
void TxTemplate(int slotId)
{
  // template slot id
  dataBuffer[6] = 0;
  dataBuffer[7] = slotId;

  // Total template lenght to send
  dataBuffer[8] = templateRxLen / 256;
  dataBuffer[9] = templateRxLen % 256;
  if (sendCommandReceiveResponse(SendTemplateStart) && errorCode == FP_OK)
  {
    U8Bit maxFrames = templateRxLen / 128 - 1;
    U8Bit frame = 0;
    U16Bit totalLen = templateRxLen;
    U16Bit index = 0;
    int retry = 4;
    U8Bit len = 128;
    int i = 0;
    Log.printf("Template size: %d ,   Sending %d frames to slot %d\r\n", templateRxLen, maxFrames, slotId);
    while (frame <= maxFrames && retry-- > 0)
    {
      // len = 128;
      //  if (totalLen < 128)
      //    len = totalLen;
      dataBuffer[6] = 0;
      dataBuffer[7] = frame;
      memcpy(dataBuffer + 8, templateRx + index, 128);

      if (sendCommandReceiveResponse(SendTemplateData) && errorCode == FP_OK)
      {
        if (frame == 0 || frame == maxFrames) // Print to log first  and last frames
        {
          i = 0;
          Log.println("First or last template frame");
          while (i++ < 128)
            Log.printf("%02X ", templateRx[index++]);
          Log.println();
        }
        else
          index += 128;
        Log.printf("Sent frame %d   ", frame);
        frame++;
        totalLen -= 128;
        retry = 5;
      }
    }
    if (retry > 0)
    {
      sprintf(messageBuffer, "All frames sent frames to slot %d", slotId);
      Log.println(messageBuffer);
      getSlotInfos();
    }
  }
  else
    sprintf(messageBuffer, "No Module response");
}