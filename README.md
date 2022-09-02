# About M583F Fingerprint Module

This capacitive fingerprint sensor M583F supports fingerprint capture, image processing, fingerprint storage, fingerprint comparison and so on and  is widely used and suitable for various fingerprint identification systems, such as:

* Fingerprint door locks, safes, gun boxes, finance and other security fields
* Access control system, industrial control computer, POS machine, driving school
training, attendance and other ide ntity fields;
* Private clubs, management software, licensing and other management fields

# About this Library and example

This software is based on documents received from the manufaturer [Guangzhou Gouku Technology Co., Ltd](https://gocool.en.alibaba.com/company_profile.html?spm=a2700.details.0.0.6c8b5b8emXWpxZ).
also  [Guangzhou Gouku Technology Co., Ltd](http://www.zyjjhome.com/)

This software is a working example for Arduino + ESP32 processor module + VSCode + PlatformIO extension

The ESP32 module act as a Wi-Fi station access poin :

![access point](assets/ESP32-access-point.webp)

Is uses a connected Browser to input commands:

![Command Screen](assets/browserCommandScreen.jpeg)

## Table of Contents

* [Hardware Connections](#hardware)
* [Installation](#installation)
* [Lib Methods](#methods)
* [History](#history)
* [Credits](#credits)

## Hardware Connections

On file platformio.ini is defined on what pins module M583F is connected to ESP32 module, change according to your hardware circuit design:

build_flags =
    -D ENABLE_DEBUG_FINGER=1
    -D FINGER_PORT=0
    -D FINGER_VIN_GPIO=13
    -D FINGER_INT_GPIO=5

## Installation

This project is ready for running:

* Clone the project from GitHub
* Make the hardware connections
* Compile and upload software to module
* Open Plaformio serial monitor
* On configurations/Wi-Fi of your cell phone choose "FingerTests" network
* Open your cell phone brower on address 192.168.4.1 (may be another, check it on debug serial monitor screen)

## Lib Methods




