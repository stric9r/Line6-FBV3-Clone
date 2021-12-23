#!/bin/bash

#used to flag that program was started
fbv3_started=0

#loop for ever
while true
do
  #see if the device is plugged in
  spider_v_pid_vid=$(lsusb | grep "0e41:5068")
  
  if [ -z "$spider_v_pid_vid" ]; 
  then
    # check if fbv3 is running, if so kill it
    # because the device is no longer plugged in
    if [ "$fbv3_started" -eq 1 ];
    then
      fbv3_running=$(ps -a | grep "fbv3")
      if [ -n "$fbv3_running" ];
      then 
        echo "fbv3 clone unplugged, terminate fbv3 and try again"
        $(killall fbv3)
        fbv3_started=0
      fi
    fi
  else
    # already commanded
    if [ "$fbv3_started" -eq 1 ];
    then
      # verify it's running
      fbv3_running=$(ps -a | grep "fbv3")
      if [ -z "$fbv3_running" ];
      then
        # something went wrong if commanded but not running
        # force try again
        fbv3_started=0
        echo "fbv3 clone not running anymore, retry"
      fi
    fi
  
    # only run if not running
    if [ "$fbv3_started" -eq 0 ];
    then
      # execute program
      $(/opt/fbv3_clone/fbv3 &>/dev/null &)
      fbv3_started=1
      echo "running fbv3 clone"
    fi
  fi
    # keep testing after 1 second delay
    sleep 1 
done

