# rfid-musicbox

Create an easy way for your kids (or you!) to play music through your Sonos speakers by combining the excitement of RFID, ESP8266, Spotify & Sonos.

This project was built to predominantly allow my kids to play music, using simple RFID cards tapped against a reader to allow them to play the songs that they love without me being around.

1. [Repository File Structure](#repository-file-structure)
2. [Requirements](#requirements)
3. [Hardware](#hardware)
4. [Installation/Build](#installationbuild)
5. [Setup](#setup)
6. [Usage](#usage)
7. [References](#references)

## Repository File Structure
- /Fritzing - sketch of the wiring diagram
- /html - reference html of the pages served from the musicbox webserver
- /images - supporting images for documentation & build
- /src - source files for the project

## Requirements
This project should be extendable to additional music services, but was built to support playing Spotify songs through a network connected Sonos speaker. So at a minimum you will need:
1. Spotify Premium subscription
2. At least one Sonos speaker, connected to the same wifi network
3. 1 x NodeMCU ESP8266 board - I suggest the NodeMCU v3 Compatible ESP8266 CH340 Board, and make sure you check the version
4. 1 x MFRC-522 RFID reader board - you can sometimes pick up a kit for this that includes cables, and an RFID card
5. 7 x jumper wires to connect the MFRC-522 to the NodeMCU board
6. RFID cards - as many as you need, but at least one for testing. I bought a pack of 20 to start
7. 1 x Micro USB cable, and 1 USB Power Supply - I don't think the power requirements of the NodeMCU are that great, but your smartphone power supply will easily do

There are a few additional items I purchased for my particular set-up:
8. Something to put the assembled electronics in to. I used a simple plastic project box from a local electronics supplier.
9. A Micro USB 5Pin Male to Female w/Screw Hole Panel. I used this to put a female USB plug on the outside of the project box, and make it easier to unplug/move as needed.

## Hardware
To start with, you'll need to connect the electronics as shown below, or detailed in the [Fritzing wiring diagram](Fritzing/RFID-Musicbox.fzz).

![rfid-musicbox-Fritzing](images/fritzing.png)

Once connected, connect the Micro USB cable and power supply to the NodeMCU board.

## Installation/Build
The source code for this project was originally written uisng VS Code, and compiled using the [Platform IO](https://platformio.org/) plugin. If you'd prefer to use the Arduino IDE, a quick Google search should show you how to (relatively) easily adapt the source files to work as an .ino format.

*Note: Before you start the below steps, make sure your NodeMCU/ESP8266 board is connected to and powered by the USB port of your computer, not to the USB Power Supply.*

If you have already have VS Code installed:
1. Install [Platform IO IDE for VS Code](https://platformio.org/install/ide?install=vscode)
2. Add a new workspace using the **rfid-musicbox** folder
3. Build and upload the firmware using the IDE [this guide](https://docs.platformio.org/en/latest/integration/ide/vscode.html#ide-vscode) is helpful to get started

If you just want to install Platform IO to run on the command line:
1. Install [Platform IO Core](https://platformio.org/install/cli)
2. Open a command line prompt
3. Change to the **rfid-musicbox** folder
4. Run `platformio run --target upload`

By this point, you should have compiled the firmware and uploaded it to the NodeMCU.

## Setup
1. After the firmware is uploaded and installed, look for a new wifi hotspot named `musicbox_setup` using your phone or computer
2. Connect your device to the wifi hotspot, using the password `musicbox_setup`
3. The hotspot should automatically connect you using a captive portal style web page, otherwise just point your browser to the IP `http://192.168.4.1/`
4. Enter the details of/select your home Wifi network (along with network password) in the provided form, then click save
5. The rfid-musicbox should now try to connect to your home Wifi network
6. After a few seconds (/minutes) try to connect to `http://musicbox.local/` using your browser

*Note: If you use an Android phone to connect to the ESP8266, you might find you have trouble connecting due to inconsistent support for mDNS in Android phones. As a workaround, try connecting using the IP address of the device which you can find using your Router or using an app like [Fing](https://www.fing.com/products/fing-app)*

## Usage

## References
I used a lot of different references to build this project, and am thankful to a massive amount of online resources. Key resources I found extremely helpful included:
- https://lastminuteengineers.com/how-rfid-works-rc522-arduino-tutorial/ - a great reference on how RFID works, and how to combine it with an Arduino or equivalent device
- 
-
