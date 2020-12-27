# Line-6-FBV3-Clone
Clone of USB interface between Line6 FBV3 (foot controller) and a Line6 Spider V amplifier. 
The FBV3 is simulated using a Raspberry Pi 4B.  
It's current state only allows for authentication and simple ON/OFF commands. 
This is still very much a work in progress.

## Platform
The hardware platform used is Raspberry Pi 4B (Linux 5.4.79-v7l+ armv7l).  
It using wiringPi and libUSB.  It's important to upgrade wiringPi to version 2.52 or higher for it to work.

The platform can easily change, all platform specific code is in main.  

## Schematic 
No schematic yet, just the code (currently using wires shorted together to simulate button presses).

## Building using GCC
gcc -o fbv3 main.c fbv3_clone.c -lwiringPi -lusb-1.0

## File descriptions
### fbv3_clone.c
### fbv3_clone.h
Floor Board Version 3 Clone code.
It contains basic functionality to turn on/off certain effects.

### fbv3_defines.h
Define file that holds all connection specific defines, array sizes, command values, etc.

### fbv3_startup.sh
Script that can be called at startup, just put in at the end of the /etc/rc.local (as a background process, it loops forever).
It's hardcoded to look in /opt/fbv3_clone/ for fbv3 application.
It will check if FBV3 Clone is running, if not it'll start it.
This is used to keep the program running incase the USB times out or the USB cable is unplugged then plugged in.
It's a crutch until there is a better way to handle the program exiting unexpectedly.  

### main.c
The main entry of the code.  It combines the FBV3 clone module with wirePi to get I/O from the Raspberry Pi 4B.
If moving to a different platform, the changes only need to be done here.






