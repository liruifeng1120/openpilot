#!/bin/bash

# 启动带有JY62设备支持的SunnyPilot
# 确保JY62设备已连接并配置好权限

echo "启动 SunnyPilot with JY62 IMU 支持"

# 检查项目目录
if [ ! -d "/home/liruifeng/ajouatom" ]; then
    echo "错误: 项目目录不存在"
    exit 1
fi

# 检查虚拟环境
if [ ! -d "/home/liruifeng/ajouatom/.venv" ]; then
    echo "错误: 虚拟环境不存在，请先创建"
    exit 1
fi

# 检查必要文件
if [ ! -f "/home/liruifeng/ajouatom/system/manager/manager.py" ]; then
    echo "错误: 系统管理器不存在"
    exit 1
fi

# 检查设备
if [ ! -e "/dev/ttyUSB0" ]; then
    echo "警告: JY62设备未连接到/dev/ttyUSB0"
    echo "请连接设备或修改配置"
fi

# 激活虚拟环境并启动系统
cd /home/liruifeng/ajouatom
source .venv/bin/activate

# 设置环境变量以启用JY62设备支持
export IMU_TYPE=jy62
export IMU_DEVICE=/dev/ttyUSB0
export IMU_BAUD=9600

echo "环境变量设置:"
echo "  IMU_TYPE=$IMU_TYPE"
echo "  IMU_DEVICE=$IMU_DEVICE"
echo "  IMU_BAUD=$IMU_BAUD"
echo "  USE_WEBCAM=$USE_WEBCAM"
echo "  ROAD_CAM=$ROAD_CAM"
echo ""

echo "启动系统管理器..."
echo "按 Ctrl+C 停止系统"

# 启动系统管理器
USE_WEBCAM=1 ROAD_CAM=2 python system/manager/manager.py