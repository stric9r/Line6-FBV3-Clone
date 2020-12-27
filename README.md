# Line-6-FBV3-Clone
Clone of USB interface between linux (Raspberry Pi 4B) and a Line6 Spider V amplifier
It's current state only allows for authentication and simple ON/OFF commands.
This is still very much a work in progress.

# Platform
The hardware platform used is Raspberry Pi 4B.  
It using wiringPi.  It's important to upgrad wiringPi to version 2.52 or higher for it to work.

# Building using GCC
gcc -o fbv3 main.c fbv3_clone.c -lwiringPi -lusb-1.0




