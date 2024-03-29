# Line-6-FBV3-Clone
Clone of USB interface between Line6 FBV3 (foot controller) and a Line6 Spider V amplifier. 
The FBV3 is simulated using a Raspberry Pi.  
It's current state only allows for authentication, changing presets, and simple ON/OFF commands for effects. 
This is still very much a work in progress, mainly need to decipher feedback to get ON/OFF status.
Regardless, without the feedback, I consider this project done.  It is to my liking in regards of use.  
Ready to move on to something else.

## Problems
The USB interface is slow, even with debouncing at 100ms and the USB timeout at 100ms.  All this was deciphered from the sister application Spider V Remote.  
The Spider V Remote normally takes ~1 sec from a button press on the PC for an action on the amp.  Similar results happen with this pedal.  
This leads me to believe that the amp itself does not prioritize USB communications in the same manner as the RS-422 interface.  
So it's not something that could be used in a professional setting.  This project was more for learning Linux, libUSB, and Wireshark protocol analysis.  
Either way, it's a fun thing to play with.  If seriously considering creating a mock up FBV3 clone, consider using the RS-422 interface instead in your project.  

## Platform
The hardware platforms tested are Raspberry Pi 4B (Linux 5.4.79-v7l+ armv7l) and Raspberry Pi Zero W (Linux 5.4.83+ armv6l)

It using wiringPi and libUSB.  It's important to upgrade wiringPi to version 2.52 or higher for it to work.

The platform can easily change, all platform specific code is in main. 

## Schematic 
Just momentary buttons with software debouncing, also a MAX7219 to control the LED for status feedback.  
Schematic is uploaded but not tested/finished, and no code written for the MAX7219 yet (as of 3/25/2021).

## Prereqs
There are only two prereqs, and that's Libusb and updating WiringPi (mainly for Raspberry Pi 4B to work). Below are how you get them.

Libusb: 

sudo apt-get install libusb-1.0-0-dev

WiringPi 2.52 or higher:

cd /tmp

wget https://project-downloads.drogon.net/wiringpi-latest.deb

sudo dpkg -i wiringpi-latest.deb

## Building using GCC
gcc -o fbv3 main.c fbv3_store.c fbv3.c max7219.c -lwiringPi -lusb-1.0

## Execution
Make sure to run with root access.
To automagically run, place fbv3_startup.sh, default.bin, store.bin, and fbv3 (the program) in /opt/fbv3_clone directory.
Then add "sh /opt/fbv3_clone/fbv3_startup.sh" to the end of the /etc/rc.local 

## File descriptions
### fbv3_clone.c
### fbv3_clone.h
Floor Board Version 3 Clone code.
It contains basic functionality to turn on/off certain effects, and switch presets (banks) up and down.

### fbv3_defines.h
Define file that holds all connection specific defines, array sizes, command values, etc.

### fbv3_startup.sh
Script that can be called at startup, just put in at the end of the /etc/rc.local (as a background process, it loops forever).
It's hardcoded to look in /opt/fbv3_clone/ for fbv3 application.
It will check if FBV3 Clone is running, if not it'll start it.
This is used to keep the program running incase the USB times out or the USB cable is unplugged then plugged in.
It's a crutch until there is a better way to handle the program exiting unexpectedly.  

### max7219.c
### max7219.h
The display driver for all the led's on the pedal board.  This module heavily exposes all the features of the max7219
serial interface 8 digit led display driver.

### main.c
The main entry of the code.  It combines the FBV3 clone module with wirePi to get I/O from the Raspberry Pi 4B.
If moving to a different platform, the changes only need to be done here.



### fbv3_store.c
### fbv3_store.h
Reads in and writes to binary file store.bin.
This file holds the configuration settings for the application.
In any event that store.bin is corrupted, copy default.bin as store.bin to get default settings back.
 


