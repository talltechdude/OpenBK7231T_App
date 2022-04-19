# Introduction

OpenBK7231T/OpenBeken is a Tasmota replacement for new Tuya modules featuring MQTT and Home Assistant compatibility.
This repository is named "OpenBK7231T_App", but now it's a multiplatform app, supporting build for 4 separate chips:
- BK7231T (WB3S, WB2S, WB2L, etc)
- BK7231N (CB2S, CB2L, etc)
- XR809 (XR3, etc)
- BL602

Please use automatically compiled binaries from the Releases tab. To build yourself for a given platform, just checkout first our version of SDK and then checkout this app repository into it, details later.

# Features

OpenBeken features:
- Tasmota-like setup, configuration and experience on all supported platforms
- OTA firwmare upgrade system (for Beken chips)
- MQTT compatibility with Home Assistant
- Support for multiple relays, buttons, leds, inputs and PWMs
- Driver system for custom peripherals, including TuyaMCU, I2C bus and Bl0942 power metering chip
- NTP time from network (can be used with TH06 and other TuyaMCU devices)
- RGBCW LED lighting control compatible with Home Assistant
- LittleFS integration for large files (resides in OTA memory, so you have to backup it every time you OTA)
- Command line system for starting and configuring drivers
- Short startup command (up to 512 characters) storage in flash config, so you can easily init your drivers (eg. BL0942) without LittleFS
- and much more

# Building for BK7231T

Get the SDK repo:
https://github.com/openshwprojects/OpenBK7231T
Clone it to a folder, e.g. bk7231sdk/

Clone the app repo into bk7231sdk/apps/<appname> - e.g. bk7231sdk\apps\openbk7231app

On Windows, start a cygwin prompt.

go to the SDK folder.

build using:
`./b.sh`
  
you can also do advanced build by build_app.sh:
`./build_app.sh apps/<appname> <appname> <appversion>`
(appname must be identical to foldername in apps/ folder)
  
e.g. `./build_app.sh apps/openbk7231app openbk7231app 1.0.0`

# flashing for BK7231T


## UART (Windows only)

get BKwriter 1.60 exe (extract zip) from here https://github.com/openshwprojects/OpenBK7231T
  
Use USB to TTL converter with 3.3V logic levels, like HW 597

connect the PC to TX1 and RX1 on the bk7231 (TX2 and RX2 are optional, only for log)
  
start flash in BKwriter 1.60 (select COM port, etc)
then re-power the device (or reset with CEN by temporary connecting CEN to ground) until the flashing program continues, repeat if required.
  
## UART (multiplatform method, Python required)

clone the repo https://github.com/OpenBekenIOT/hid_download_py
  
Use USB to TTL converter with 3.3V logic levels, like HW 597 

connect the PC to TX1 and RX1 on the bk7231 (TX2 and RX2 are optional, only for log)

start flash using:
`python uartprogram <sdk folder>\apps\<folder>\output\1.0.0\<appname>_UA_<appversion>.bin -d <port> -w`
then re-power the device (or reset with CEN temporary connecting CEN to ground) until the flashing program continues, repeat if required.

e.g.
`python uartprogram C:\DataNoBackup\tuya\tuya-iotos-embeded-sdk-wifi-ble-bk7231t\apps\my_alpha_demo\output\1.0.0\my_alpha_demo_UA_1.0.0.bin -d com4 -w`

## SPI

see https://github.com/OpenBekenIOT/hid_download_py/blob/master/SPIFlash.md

## OTA

Once the firmware has been flashed for the first time, it can be flashed over wifi (note: change hardcoded firmware URL in new_http.c)

Setup a simple webserver to serve `<sdk folder>\apps\<folder>\output\1.0.0\<appname>_<appversion>.rbl`

Visit <ip>/ota - here start the flashing process.

## First run

At first boot, if the new firmware does not find your wifi SSID and password in the Tuya flash, it will start as an access point.

The access point will come up on 192.168.4.1, however some machines may not get an ip from it - you may need to configure your connecting for a staitc IP on that network, e.g. 192.168.4.10

Once you are connected and have an IP, go to http://192.168.4.1/index , select config then wifi, and setup your wifi.

After a reboot, the device should connect to your lan.

 
# Building for BK7231N

Same as for BK7231T, but use BK7231N SDK:
https://github.com/openshwprojects/OpenBK7231N


# Flashing for BK7231N

BKwriter 1.60 doesn't work for BK7231N for me, in BK7231 mode it errors with "invalid CRC" and in BK7231N mode it fails to unprotect the device.
For BK7231N, one should use:
  
https://github.com/OpenBekenIOT/hid_download_py
  
Flash BK7231N QIO binary, like that:
  
`python uartprogram W:\GIT\OpenBK7231N\apps\OpenBK7231N_App\output\1.0.0\OpenBK7231N_app_QIO_1.0.0.bin --unprotect -d com10 -w --startaddr 0x0`
  
  Remember - QIO binary with --unprotect and --startaddr 0x0, this is for N chip, not for T.
 
You can see an example of detailed teardown and BK7231N flashing here (use google translate): https://www.elektroda.pl/rtvforum/viewtopic.php?t=3874289&highlight=
  
# Building for XR809

Get XR809 SDK:
https://github.com/openshwprojects/OpenXR809

Checkout this repository to openxr809/project/oxr_sharedApp/shared/
  
Run ./build_oxr_sharedapp.sh
  
# Flashing for XR809
  
Get USB to UART converter, start phoenixMC.exe from OpenXR809 repository and follow this guide: https://www.elektroda.com/rtvforum/topic3806769.html
  
# Testing HTTP server on Windows
  
It is also possible to build a part of our App for Windows platform. It basically creates a Windows .exe for our HTTP server, so developers can create our configurator, etc, pages faster, without having any Tuya modules at hand. For building on Windows, use MSVC projects in the app directory. It is using Winsock and creates a TCP listening socket on port 80, so make sure your machine has it free to use.
  
# Pin roles
 
You can set pin roles in "Configure Module" section or use one of predefined templates from "Quick config" subpage.
For each pin, you also set coresponding channel value. This is needed for modules with multiple relays. If you have 3 relays and 3 buttons, you need to use channel values like 1, 2, and 3. Just enter '1' in the text field, etc.
Currently available pin roles:
- Button 
- Button_n (as Button but pin logical value is inversed)
- Relay 
- Relay_n (as Relay but pin logical value is inversed)
- LED 
- LED_n (as Led but pin logical value is inversed)
- Button Toggle All - this button toggles all channels at once
- Button Toggle All_n (as above but pin logical value is inversed)
- PWM - pulse width modulation output for LED dimmers (with MQTT dimming support from Home Assistant)
- WiFi LED - special LED to indicate WLan connection state. LED states are following: LED on = client mode successfully connected to your Router. Half a second blink - connecting to your router, please wait (or connection problem). Fast blink (200ms) - open access point mode. In safe mode (after failed boots), LED might not work.
- DigitalInput - this is a simple digital input pin, it sets the linked channel to current logical value on it, just like digitalRead( ) from Arduino. This input has a internal pull up resistor.
- DigitalInput_n (as above but inversed)
  
# Safe mode
  
  Device is counting full boots (full boot is a boot after which device worked for 30 seconds). If you power off and on device multiple times, it will enter open access point mode and safe mode (safe mode means even pin systems are not initialized). Those modes are used to recover devices from bad configs and errors.
  
# RGBCW Tuya 5 PWMs LED bulb control compatible with Home Assistant
  
  RGBCW light bulbs are now supported and they are compatible with HA by rgb_command_template, brightness_value_template, color_temp_value_template commands. Please follow the guide below showing how to flash, setup and pair them with HA by MQTT:
  
  https://www.elektroda.com/rtvforum/topic3880540.html#19938487
  
# I2C drivers system with support for MCP23017 bus expander, TC74 temperature sensor and more
  
  Detailed description and step by step tutorial on Elektroda.com is coming soon.
  
# BL0942 power metering plug (UART communication mode) support and calibration process
    
  Detailed description and step by step tutorial on Elektroda.com is coming soon.
  
# Detailed flashing guides along with device teardowns
  
 I have prepared several detailed teardowns and flashing guides for multiple supported devices.
  
  Outdoor two relays smart switch CCWFIO232PK (BK7231T): 
  https://www.elektroda.com/rtvforum/topic3875654.html
  https://www.elektroda.pl/rtvforum/viewtopic.php?p=19906670#19906670
  
  Qiachip Smart Switch module (BK7231N/CB2S): 
  https://www.elektroda.com/rtvforum/topic3874289.html
  https://www.elektroda.pl/rtvforum/viewtopic.php?t=3874289&highlight=
  
  Tuya RGBCW 12W light bulb (raw PWMs, no I2C, BK7231N)
  https://www.elektroda.com/rtvforum/topic3880540.html#19938487
  https://www.elektroda.pl/rtvforum/viewtopic.php?t=3880540&highlight=
  
# Futher reading
  
For technical insights and generic SDK information related to Bekken and XRadio modules, please refer:
  
https://www.elektroda.com/rtvforum/topic3850712.html
  
https://www.elektroda.com/rtvforum/topic3866123.html
  
https://www.elektroda.com/rtvforum/topic3806769.html
  
# Support project
  
If you want to support project, please donate at: https://www.paypal.com/paypalme/openshwprojects
  
