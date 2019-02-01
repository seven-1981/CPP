#!/bin/bash
sudo g++ *.cpp -o bpm -lwiringPi -std=c++11 -lpthread -lasound -lrt -lxdo -O3