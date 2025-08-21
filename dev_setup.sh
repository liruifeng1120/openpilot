#!/bin/bash

# 开发环境快速设置脚本
# 注意：此脚本会降低系统安全性，仅适用于开发/测试环境

echo "设置开发环境..."

# 检查是否以root权限运行
if [[ $EUID -ne 0 ]]; then
   echo "此脚本需要root权限" 
   echo "请使用 sudo 运行此脚本:"
   echo "sudo $0"
   exit 1
fi

# 为所有用户开放串口设备访问权限（仅限开发环境）
echo 'SUBSYSTEM=="tty", KERNEL=="ttyUSB[0-9]*", MODE="0666"' > /etc/udev/rules.d/99-dev-serial.rules

# 重新加载udev规则
udevadm control --reload-rules
udevadm trigger

echo "开发环境设置完成！"
echo "现在所有用户都可以访问串口设备"
echo "警告：这会降低系统安全性，仅在开发/测试环境中使用"