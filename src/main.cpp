/*
 * This is an example of how to use the library FingerM583F
 * using VSCode+ PlatformIO extension+ Arduino + ESP32 processor module
 */
#include <Arduino.h>
#include <WiFi.h>
#include "fingerprint_commands.h"
#include "fingerprint_device.h"
#include "fingerprint_protocol.h"
#include <iostream>
#include <string>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <stdlib.h>

// Set web server port number to 80
WiFiServer server(80);

#define Log Serial

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
  const char *ssid = "FingerTests";
  Log.begin(9600);
  commFingerInit(57600);

  WiFi.softAP(ssid, NULL);

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

WiFiClient client;
void loop()
{

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
                autoEnroll(messageBuffer,0xffff);
              else
                sprintf(messageBuffer, "Must entre registration number > 0 to associate with slotId");
            }
            else if ((pos = headerHttp.indexOf("TxTemplate=")) >= 0)
            {
              Log.println("TxTemplate");
              if (templateRxLen == 0)
                sprintf(messageBuffer, "No template data to send: get one with RxTemplate");

              else if (getInboxText(i))
              {
                if (TxTemplate(inboxNumber, templateRx, &templateRxLen, messageBuffer))
                {
                  sprintf(messageBuffer, "All frames sent frames to slot %d", inboxNumber);
                  Log.printf(messageBuffer);
                  getSlotInfos(messageBuffer);
                }
              }
              else
                sprintf(messageBuffer, "Must enter slot number");
            }
            else if ((pos = headerHttp.indexOf("RxTemplate=")) >= 0)
            {
              Log.println("RxTemplate");
              if (getInboxText(i))
              {
                if (getSlotInfos(messageBuffer))
                  if (RxTemplate(inboxNumber, templateRx, &templateRxLen, messageBuffer))
                    sprintf(messageBuffer, "Now we have a template to send back to Finger module to test TxTemplate()");
              }
              else
                sprintf(messageBuffer, "Must enter slot number");
            }
            else if (headerHttp.indexOf("Match") >= 0)
            {
              if (getSlotInfos(messageBuffer))
                if (matchTemplate(messageBuffer)){
                  sprintf(messageBuffer, "Match on slot: %d", slotID);    
                  // Flashing red Green light
                  uint8_t buffer[] = {4, 1, 30, 10, 1};
                  ledControl(buffer);
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
  else
    client = server.available(); // Listen for incoming clients
}
