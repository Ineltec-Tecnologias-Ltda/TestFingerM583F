# Table of Contents

- [Table of Contents](#table-of-contents)
  - [About M583F Fingerprint Module](#about-m583f-fingerprint-module)
  - [About this Library and example](#about-this-library-and-example)
  - [Hardware](#hardware)
  - [Installation](#installation)
  - [Libray Methods](#libray-methods)
  - [Informations from sendCommandReceiveResponse() on public variables](#informations-from-sendcommandreceiveresponse-on-public-variables)
  - [Credits](#credits)

## About M583F Fingerprint Module

This capacitive fingerprint sensor M583F supports fingerprint capture, image processing, fingerprint storage, fingerprint comparison and so on and  is widely used and suitable for various fingerprint identification systems, such as:

* Fingerprint door locks, safes, gun boxes, finance and other security fields
* Access control system, industrial control computer, POS machine, driving school
training, attendance and other ide ntity fields;
* Private clubs, management software, licensing and other management fields

## About this Library and example

This library contains a complete working project example for VSCode+ PlatformIO extension+ Arduino + ESP32 processor module

This software is based on documents received from the manufaturer [Guangzhou Gouku Technology Co., Ltd](https://gocool.en.alibaba.com/company_profile.html?spm=a2700.details.0.0.6c8b5b8emXWpxZ).

Document extensivily referenced on methods:[Fingerprit user's manual](docs/user's%20manual_Gouku.pdf)

The ESP32 module acts as a Wi-Fi station access point :

![access point](docs/ESP32-access-point.webp)

Is uses a connected Browser to input commands:

![Command Screen](docs/browserCommandScreen.png)

## Hardware

On file platformio.ini is defined on what pins module M583F is connected to ESP32 module, change it according to your hardware circuit design. 
The settings below are for ESP32-DevKit.

build_flags = <br>
    -D ENABLE_DEBUG_FINGER=1 <br>
    -D FINGER_UART_RX=4 <br>
    -D FINGER_UART_TX=5 <br>
    -D FINGER_INT_GPIO=6 <br>

Document reference to connect Finger module to ESP32:  [Fingerprint product specification](docs/M583F-Gouku.pdf)

![ESP32-C3-DevKit connected to Finger Module](docs/ESP32C3_FingerModule.jpeg)

## Installation

This project is ready for running:

* Clone the project from GitHub
* Make the hardware connections
* Compile and upload software to module
* Open Plaformio serial monitor
* On configurations/Wi-Fi of your cell phone choose "FingerTests" network, password = "123456789"
* Open your cell phone brower on address 192.168.4.1 (may be another, check it on debug serial monitor screen)

## Libray Methods

Below are the higher level protocol methods implemented.
All module commands methods can be written just using these protocol methods.

There are many methods implementing module commands given as example that uses these methods such as:

* bool heartbeat();
* bool ledControl(uint8_t *params);
* bool moduleReset();
* bool autoEnroll();
* bool matchTemplate();
* void TxTemplate();
* void RxTemplate();
* bool fingerDetection();
* bool getSlotInfos();

```C++

/// Command codes and extra data size
/// @see users manual Command set summary pages 9-12
Command AutoEnroll{cmd_fingerprint, fp_auto_enroll, 4};
Command HeartBeat{cmd_maintenance, maintenance_heart_beat, 0};
Command LedControl{cmd_system, sys_set_led, 5};
Command ReadId{cmd_maintenance, maintenance_read_id, 0};
Command MatchTemplate{cmd_fingerprint, fp_match_start, 0};
Command MatchResult{cmd_fingerprint, fp_match_result, 0};
Command FingerIsTouch{cmd_fingerprint, fp_query_slot_status, 0};
Command Enroll{cmd_fingerprint, fp_enroll_start, 1};
Command EnrollResult{cmd_fingerprint, fp_enroll_result, 1};
Command ModuleReset{cmd_system, sys_reset, 0};
Command SendTemplateStart{cmd_fingerprint, fp_start_send_template, 4};// @see users manual page 36
Command SendTemplateData{cmd_fingerprint, fp_send_template_data, 0x89}; //0x89 is the maximum to be sent at each packet
Command ReceiveTemplateStart{cmd_fingerprint, fp_start_get_template, 2}; // @see users manual page 38
Command ReceiveTemplateData{cmd_fingerprint, fp_get_template_data, 2}; 
Command DeleteTemplates{cmd_fingerprint, fp_delete_templates, 3}; // @see users manual page 33

/// @brief Sends Commands with fixed extra data, and receives response from module
// @see page Command set summary on pages 9 to 12 on users manual
/// @param command Only commands with fixed extra data bytes after header
/// For commands with extra data bytes "dataBuffer" has to be filled with data( starting at index 6)
///  first 6 bytes are added by protocol methods with check password (4)+ command(2) 
/// @return if true, sets "dataBuffer" and "answerDataLength" according to received data
///   Places all received data to "dataBuffer" starting at index 0
/// if false errorCode and  errorMessage are set
bool sendCommandReceiveResponse(Command command);


/// @brief Sends Commands with variable extra data, and receives response from module
// @see page Command set summary on pages 9 to 12 on users manual
/// @param command  Commands with variable extra data bytes after header(like 5.21 Fingerprint feature data download)
/// @param length number of extra bytes to send after
/// "dataBuffer" has to be filled with data( starting at index 6)
///  first 6 bytes are added by protocol methods with check password (4)+ command(2) 
/// @return if true, sets "dataBuffer" and "answerDataLength" according to received data
///   Places all received data to "dataBuffer" starting at index 0
/// if false "errorCode" and  "errorMessage" are set
bool sendCommandReceiveResponse(Command command,size_t length);
```

## Informations from sendCommandReceiveResponse() on public variables

```C++

/// Used for tx and rx data to/from Finger Module
U8Bit dataBuffer[140];

/// Total received data lenght from finger module
U16Bit answerDataLength;

/// Answer command received from finger module
U8Bit rtxCommandHigh;
/// Answer command received from finger module
U8Bit rtxCommandLow;

/// Error code received from finger module: must be == zero
S32Bit errorCode;
```

## Credits

Written by Roberto O Fonseca[roberto@ineltec.com.br]

Many lines of code came from "Elock_DemoCode" received from  [Guangzhou Gouku Technology Co., Ltd](http://www.zyjjhome.com/), and free code examples for Arduino and ESP32 came from other sources.

Guangzhou Gouku contact person: Mark[mark@zyjjhome.com]
