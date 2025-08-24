#!/bin/bash

# JY62设备权限配置脚本
# 自动化配置系统以允许访问JY62 IMU设备

echo "开始配置JY62设备访问权限..."

# 检查是否以root权限运行
if [[ $EUID -ne 0 ]]; then
   echo "此脚本需要root权限才能正确配置系统" 
   echo "请使用 sudo 运行此脚本:"
   echo "sudo $0"
   exit 1
fi

# 1. 确保dialout组存在
if ! getent group dialout > /dev/null; then
    echo "创建dialout组..."
    groupadd dialout
fi

# 2. 将当前用户添加到dialout组
CURRENT_USER=$(logname 2>/dev/null || echo $SUDO_USER)
if [ -n "$CURRENT_USER" ]; then
    if ! id -nGz "$CURRENT_USER" | grep -qzxF "dialout"; then
        echo "将用户 $CURRENT_USER 添加到dialout组..."
        usermod -a -G dialout "$CURRENT_USER"
    else
        echo "用户 $CURRENT_USER 已在dialout组中"
    fi
else
    echo "警告: 无法确定当前用户，跳过添加到dialout组"
fi

# 3. 复制udev规则文件
echo "安装udev规则..."
cp /home/liruifeng/ajouatom/99-jy60.rules /etc/udev/rules.d/
chmod 644 /etc/udev/rules.d/99-jy60.rules

# 4. 重新加载udev规则
echo "重新加载udev规则..."
udevadm control --reload-rules
udevadm trigger

# 5. 显示设备信息
echo "当前连接的USB串口设备:"
ls -l /dev/ttyUSB* 2>/dev/null || echo "没有检测到USB串口设备"

echo ""
echo "配置完成！"
echo "请执行以下操作之一以使组权限生效："
echo "1. 重新登录系统"
echo "2. 运行命令: newgrp dialout"
echo ""
echo "之后您应该能够访问JY62设备"