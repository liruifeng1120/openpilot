#!/bin/bash

# 启动openpilot系统管理器并测试JY62设备支持
# 此脚本演示如何使用openpilot的原生运行方式

echo "启动openpilot系统以测试JY62设备支持"

# 检查是否以root权限运行（可选，用于设备访问）
if [[ $EUID -ne 0 ]]; then
    echo "注意: 建议以root权限运行以确保设备访问"
fi

# 确保我们在项目根目录
cd /home/liruifeng/ajouatom

# 检查必要的文件是否存在
if [ ! -f "./selfdrive/locationd/locationd" ]; then
    echo "错误: locationd可执行文件不存在，请先编译"
    echo "运行: scons --minimal selfdrive/locationd/locationd"
    exit 1
fi

echo "正在启动system manager..."
echo "您可以通过以下方式监控系统运行:"
echo "  1. 查看日志文件: cat /tmp/systemd.log"
echo "  2. 检查进程状态: ps aux | grep locationd"
echo "  3. 检查设备数据: python3 test_locationd_jy62.py"
echo ""
echo "按 Ctrl+C 停止系统"

# 启动系统管理器（简化版本）
# 在实际的openpilot系统中，这会更复杂，涉及多个进程和状态管理
PYTHONPATH=/home/liruifeng/ajouatom python3 -c "
import time
import subprocess
import signal
import sys

def signal_handler(sig, frame):
    print('正在停止系统...')
    if 'proc' in globals():
        proc.terminate()
        proc.wait()
    sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)

# 启动locationd进程
proc = subprocess.Popen([
    './selfdrive/locationd/locationd', 
    '--type=jy62',
    '--device=/dev/ttyUSB0',
    '--baud=9600'
], cwd='/home/liruifeng/ajouatom')

print(f'locationd进程已启动，PID: {proc.pid}')

try:
    # 等待进程运行
    while True:
        if proc.poll() is not None:
            print('locationd进程已退出')
            break
        time.sleep(1)
except KeyboardInterrupt:
    signal_handler(signal.SIGINT, None)
"

echo "系统已停止"