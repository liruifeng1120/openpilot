#!/bin/bash
sudo chmod 777 /dev/bus/usb/*
cd /data/openpilot && 
source .venv/bin/activate && 
USE_WEBCAM=1 ROAD_CAM=0 NO_DM=0 system/manager/manager.py

