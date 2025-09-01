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
cp /home/$LOGNAME/ajouatom/99-jy62.rules /etc/udev/rules.d/
chmod 644 /etc/udev/rules.d/99-jy62.rules

# 4. 重新加载udev规则
echo "重新加载udev规则..."
udevadm control --reload-rules
udevadm trigger

echo "配置完成！"
echo ""
echo "请执行以下步骤来验证配置："
echo "1. 运行测试脚本验证设备连接:"
echo "   cd /home/$LOGNAME/ajouatom && python3 simple_jy62_test.py"
echo ""
echo "2. 检查设备权限:"
echo "   ls -l /dev/ttyUSB*"
echo ""
echo "3. 检查用户组:"
echo "   groups"
echo ""
echo "如果仍有权限问题，请尝试以下操作："
echo "sudo usermod -a -G dialout $LOGNAME"
echo "newgrp dialout"
echo ""
echo "然后再次运行测试："
echo "cd /home/$LOGNAME/ajouatom && python3 simple_jy62_test.py"
echo "cd /home/$LOGNAME/ajouatom && python3 test_jy62_device.py"