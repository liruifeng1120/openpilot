#!/bin/bash

# 设置 USB 权限
sudo chmod -R 777 /dev/bus/usb/*

# 进入本项目所在目录（不论文件夹名称）
OP_DIR=$(dirname "$(readlink -f "$0")")
cd "$OP_DIR"
export PILOT_ROOT="$OP_DIR"

# 设置环境变量并激活虚拟环境
export FINGERPRINT="HONDA_ACCORD"
export SKIP_FW_QUERY="1"
source .venv/bin/activate

# 在前台启动系统管理器，保证 Ctrl+C (SIGINT) 能传递到 manager 使其正常退出
# SCALE=1 QT=1 USE_WEBCAM=1 USE_CAMPATH=1 \
#   ROAD_CAM=pci-0000:c4:00.3-usb-0:1.1.1:1.0-video-index0\
#   WIDE_CAM=pci-0000:c6:00.4-usb-0:1.2:1.0-video-index0 \
#   openpilot/system/manager/manager.py
SCALE=1 QT=1 USE_WEBCAM=1 ROAD_CAM=0 openpilot/system/manager/manager.py
