#include <Arduino.h>
#include <WiFi.h>
#include "fingerprint_commands.h"
#include "fingerprint_device.h"
#include <iostream>

// Set web server port number to 80
WiFiServer server(80);

HardwareSerial Log(0);

void setup()
{
  Log.begin(57600, SERIAL_8N1, 3, 1);
  commFingerInit(57600);

  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Log.printf("AP IP address: ");
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
            else if (headerHttp.indexOf("Upload") >= 0)
            {
              Log.println("Upload");
            }
            else if (headerHttp.indexOf("Download") >= 0)
            {
              Log.println("Download");
            }
            else if (headerHttp.indexOf("Heartbeat") >= 0)
            {
              if (heartbeat())
              Log.println("Heartbeat");
              else    Log.println("No module response!!");
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
            client.println(" <a href=\"Upload\"><button class=\"button\">Upload</button></a>");
            client.println("<a href=\"Download\"><button class=\"button\">Download</button></a>");
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
