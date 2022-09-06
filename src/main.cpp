#include <Arduino.h>
#include <WiFi.h>
#include "fingerprint_commands.h"
#include "fingerprint_device.h"
#include <iostream>
#include <WiFiClient.h>
#include <WiFiAP.h>

// Set web server port number to 80
WiFiServer server(80);

#define Log Serial

void RxTemplate();
void TxTemplate();

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

void loop()
{
  WiFiClient client = server.available(); // Listen for incoming clients

  if (client)
  {                             // If a new client connects,
    Log.println("New Client."); // print a message out in the serial port
    String currentLine = "";    // make a String to hold incoming data from the client
    String headerHttp = "";
    int i = 0;
    int pos = 0;
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
              headerHttp = headerHttp.substring(pos, i - 1);
              int pos2 = headerHttp.indexOf(" ");
              Log.print("Typed text=");
              Log.println(headerHttp.substring(7, pos2));
              autoEnroll();
            }
            else if (headerHttp.indexOf("Match") >= 0)
            {
              if (matchTemplate())
              {
                if (slotID == 0xff)
                  Log.println("No Match");
                else
                  Log.printf("Match on slot: %d", slotID);
              }
            }
            else if (headerHttp.indexOf("TxTemplate") >= 0)
            {
              Log.println("TxTemplate");
              TxTemplate();
            }
            else if (headerHttp.indexOf("RxTemplate") >= 0)
            {
              Log.println("RxTemplate");
              RxTemplate();
            }
            else if (headerHttp.indexOf("Heartbeat") >= 0)
            {
              if (heartbeat())
              {
                moduleReset(); // This is only to test this command
                Log.println("Finger Module ok");
              }
              else
                Log.println("No module response!!");
            }
            else if (headerHttp.indexOf("Module") >= 0)
            {
              if (readId())
                Log.printf("\r\nModule Id: %s\r\n", dataBuffer);
              else
                Log.println("\r\nFailed to get Module Id\r\r");
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
              sendCommandReceiveResponse(DeleteTemplates);
            }
            Log.printf("\r\nsum debug Tx: %d\r\n", sumTxDebug);
            Log.printf("Rx debug  State: %d\r\n", debugRxState);

            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");

            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");

            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");
            client.println("<body><h1 style=\"color:Tomato;\">Finger M583F Server</h1>");

            client.println("<form action=\"/finger.php\">");
            client.println("<input type=\"text\" id=\"Enroll\" name=\"Enroll\">");
            client.println("<input type=\"submit\" value=\"Enroll\">");
            client.println("</form>");

            client.println("<div class=\"btn-group\">");
            client.println(" <a href=\"Heartbeat\"><button class=\"button\">Heartbeat</button></a>");
            client.println(" <a href=\"Leds\"><button class=\"button\">Leds</button></a>");
            client.println("<a href=\"Module Id\"><button class=\"button\">Module Id</button></a>");
            client.println("</div>");

            client.println("<div class=\"btn-group\">");
            client.println(" <a href=\"Match\"><button class=\"button\">Match</button></a>");
            client.println(" <a href=\"TxTemplate\"><button class=\"button\">TxTemplate</button></a>");
            client.println("<a href=\"RxTemplate\"><button class=\"button\">RxTemplate</button></a>");
            client.println("</div>");

            client.println("<div class=\"btn-group\">");
            client.println(" <a href=\"DeleteAll\"><button class=\"button\">DeleteAll</button></a>");
            client.println("</div>");

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

S8Bit frame = -16;
/// @brief This method is not working as specified on users manual page 39
/// ReceiveTemplateStart is ok and can receive the template size
/// But ReceiveTemplateData not working
/// The problem on the value to send on databuffer
/// If value == 0, module gives no response and enters on a buged state(leaves only after hardware reset)
/// If value == 64, module gives erroCode == 0x19 ==  COMP_CODE_DATA_BUFFER_OVERFLOW
///
void RxTemplate()
{
  if (sendCommandReceiveResponse(GetAllSlotStatus) && errorCode == FP_OK && answerDataLength > 0)

  {
    // template slot id
    dataBuffer[6] = 0;
    dataBuffer[7] = 0;

    if (sendCommandReceiveResponse(ReceiveTemplateStart) && errorCode == FP_OK && answerDataLength > 0)
    {
      u16_t templateSize = (((u16_t)dataBuffer[0]) << 8) + dataBuffer[1];
      Log.printf("Template size: %d\r\n", templateSize);
      delay(500);

      if (templateSize > 64)
      {

        U8Bit maxFrames = templateSize / 128;

        int retry = 10;
        bool first = true;
        bool resp = false;
        frame = 0;

        while (retry-- > 0 && frame < maxFrames)
        {
          dataBuffer[6] = 0;
          dataBuffer[7] = frame;

          resp = sendCommandReceiveResponse(ReceiveTemplateData);
          if (resp && errorCode == FP_OK && answerDataLength > 0)
          {
            if (rtxCommandLow == 0x54)
            {
              Log.printf(" answerDataLength : %d\r\n", answerDataLength);
              templateSize -= answerDataLength;
              int i = 0;
              while (answerDataLength-- > 0)
              {
                if (first)
                  Log.printf("%02X ", dataBuffer[i++]);
              }
              first = false;
              Log.println();
              delay(10);
              retry = 4;
              frame++;
            }
          }
          else
          {
            Log.printf("frame: %d   resp:%s error:%04X  answerDataLength : %d\r\n", frame, resp ? "true" : "false", resp ? errorCode : 0, resp ? answerDataLength : 0);
            delay(200);
          }
        }
      }
      else
        Log.println("??????????");
    }
  }
}

void TxTemplate()
{
}