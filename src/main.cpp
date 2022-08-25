#include <Arduino.h>
#include "fingerprint_device.h"
#include <WiFi.h>
#include "fingerprint_action.h"
#include "fingerprint_type.h"
#include "fingerprint_protocol.h"
#include <iostream>

const char *ssid = "FingerTests";
const char *password = "123456789";
const char *EnrollOk = "Enroll OK";
const char *Enrolling = "Enrolling...";
const char *TimeoutError = "Timeout Error...";
const char *TryAgain = "Please Try Again";

const char *errorMessage;

// Set web server port number to 80
WiFiServer server(80);
// Variable to store the HTTP request
String header;

HardwareSerial Log(DEBUG_PORT);

void setup()
{
  Log.begin(115200, SERIAL_8N1, 3, 1);
  commFingerInit(57600);
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.begin();
}

void loop()
{
  WiFiClient client = server.available(); // Listen for incoming clients

  if (client)
  {                                // If a new client connects,
    Serial.println("New Client."); // print a message out in the serial port
    String currentLine = "";       // make a String to hold incoming data from the client
    while (client.connected())
    { // loop while the client's connected
      if (client.available())
      {                         // if there's bytes to read from the client,
        char c = client.read(); // read a byte, then
        Log.write(c);           // print it out the serial monitor
        header += c;

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

            if (header.indexOf("Enroll") >= 0)
            {
              autoEnroll();
            }
            else if (header.indexOf("Match") >= 0)
            {
              matchTemplate();
            }
            else if (header.indexOf("Upload") >= 0)
            {
              Log.println("Upload");
            }
            else if (header.indexOf("Download") >= 0)
            {
               Log.println("Download");
            }

            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");

            client.println('<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}');

            client.println('text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}');
            client.println('.button2 {background-color: #555555;}</style></head>');
            client.println('<body><h1 style="color:Tomato;">Finger M583F Server</h1>');

            client.println('<input type="text" id="fname" name="fname">');
            client.println('  <a href="Enroll"><button class=\"button\">Enroll</button></a><br><br>');

            client.println('<div class="btn-group">');
            client.println(' <a href="Match"><button class="button">Match</button></a>');
            client.println(' <a href="Upload"><button class="button">Upload</button></a>');
            client.println('<a href="Download"><button class="button">Download</button></a>');
            client.println('</div>');

            client.println("</body></html>");

            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
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
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

bool autoEnroll()
{
  U32Bit errorCode = 0;
  S32Bit ret = 0;
  U8Bit enroll_mode;
  U8Bit times;
  U16Bit slotID;

  // enrollPara.enroll_mode = 0x01 will indicate that user must lift finger and press again during enrollment
  // enrollPara.times is the number of presses (can be set to 1~6 times)
  // enrollPara.slotID = 0xFFFF ,will be automatically assigned by the system
  const FP_auto_enroll_t enrollPara = FP_auto_enroll_t{1, 6, 0xFFFF};
  FP_enrollResult_p pEnrollResult;
 
  int8_t retry = 3;
  bool start = true;

  while (retry-- > 0)
  {
    if (start)
    {
      ret = FP_action_auto_enroll_send(enrollPara);
      if (FP_OK == ret)
      {
        retry = 6; // max 6 enroll updates
        start = false;
      }
      else
        Log.printf("Error: %d  retry: %d", ret, retry);
    }
    else
    {
      ret = FP_action_auto_enroll_recv(pEnrollResult);
      if (ret == FP_OK)
      {
        Log.printf("State: %d    Enroll Progress: %d %\r\n", pEnrollResult->state, pEnrollResult->progress);
        if ((100 == pEnrollResult->progress) && (pEnrollResult->state == 0xff))
        {
          Log.printf("Template slot: %d\r\n", pEnrollResult->slotID);
          errorMessage = EnrollOk;
          return true;
        }
        else
          errorMessage = Enrolling;
      }
      else
      {
        errorMessage = TryAgain;
        Log.print("Enroll Error: ");
        Log.println(errorCode);
        return false;
      }
    }
  }
  Log.print("Timeout Error");
  errorMessage = TimeoutError;

  return false;
}

void matchTemplate()
{
  U32Bit errorCode;
  S32Bit ret;
  int8_t retry = 100;
  FP_matchResult_t matchResult;
  bool start = true;

  while (retry-- > 0)
  {
    if (start)
    {
      ret = FP_action_match_start();
      if (ret == FP_OK)
      {
        retry = 10;
        start = false;
        vTaskDelay(200);
      }
      else
        vTaskDelay(50);
    }
    else
    {
      ret = FP_action_get_match_result(&matchResult);
      if (FP_DEVICE_TIMEOUT_ERROR == ret)
        vTaskDelay(100);
      else
      {
        if (ret != FP_OK)
        {
          Log.printf("Match Error: %d\r\n", ret);
          return;
        }

        if (matchResult.isPass == 1)
        {
          Log.printf("Match ok: %d\r\n", matchResult.slotID);
          return;
        }
      }
    }
  Log.print("Timeout Error");
  errorMessage = TimeoutError;
}
