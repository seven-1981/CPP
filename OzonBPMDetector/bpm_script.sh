#!/bin/bash
sleep 5
cd /home/pi/BPM
tvservice -s >hdmi.txt
sudo lxterminal --command=/home/pi/BPM/bpm --geometry=80x25 --working-directory=/home/pi/BPM &
