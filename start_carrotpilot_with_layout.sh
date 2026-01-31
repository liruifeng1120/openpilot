#!/bin/bash

# 设置 USB 权限
sudo chmod -R 777 /dev/bus/usb/*

# 进入 MinerU 目录
cd ~/111

# 设置环境变量并激活虚拟环境
export FINGERPRINT="HONDA_ACCORD"
export SKIP_FW_QUERY="1"
source .venv/bin/activate

# 在后台启动系统管理器
SCALE=1 USE_WEBCAM=1 ROAD_CAM=0 python3 system/manager/manager.py &

# 等待窗口创建
sleep 8

# 获取屏幕尺寸
SCREEN_WIDTH=$(xrandr | grep '*' | awk '{print $1}' | head -n1 | cut -d'x' -f1)
SCREEN_HEIGHT=$(xrandr | grep '*' | awk '{print $1}' | head -n1 | cut -d'x' -f2)

# 输出调试信息
echo "Screen resolution: ${SCREEN_WIDTH}x${SCREEN_HEIGHT}"

# 定义预设位置 - 根据当前实际窗口位置
UI_WINDOW_X=720      # UI窗口的X坐标 (从当前位置获取)
UI_WINDOW_Y=60      # UI窗口的Y坐标 (从当前位置获取)
UI_WINDOW_WIDTH=2160 # UI窗口的宽度
UI_WINDOW_HEIGHT=1080 # UI窗口的高度

# 等待UI窗口出现并移动到预设位置
for i in {1..30}; do
    if wmctrl -l | grep -q "ui"; then
        echo "Found ui window, moving to (${UI_WINDOW_X}, ${UI_WINDOW_Y})"
        wmctrl -r "ui" -e 0,$UI_WINDOW_X,$UI_WINDOW_Y,$UI_WINDOW_WIDTH,$UI_WINDOW_HEIGHT
        break
    else
        echo "Waiting for ui window... ($i/30)"
    fi
    sleep 1
done

# 定义摄像头窗口位置
CAMERA_WINDOW_HEIGHT=528
CAMERA_WINDOW_WIDTH=640

# 定义窗口排列函数
position_camera_window() {
    local cam_num=$1
    local target_x=$2
    local target_y=$3

    # 等待窗口出现
    for i in {1..30}; do
        if wmctrl -l | grep -q "Camera $cam_num"; then
            echo "Moving Camera $cam_num to ($target_x, $target_y)"
            wmctrl -r "Camera $cam_num" -e 0,$target_x,$target_y,$CAMERA_WINDOW_WIDTH,$CAMERA_WINDOW_HEIGHT
            break
        else
            echo "Waiting for Camera $cam_num... ($i/30)"
        fi
        sleep 1
    done
}

# 最后将UI窗口保持在最前面
sleep 2
if wmctrl -l | grep -q "ui"; then
    wmctrl -a "ui"
fi

# 等待所有后台进程结束
wait
