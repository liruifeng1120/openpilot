#!/bin/bash

echo "检查并更新AMD驱动和OpenCL库"

# 检查当前系统版本
echo "当前系统版本:"
lsb_release -a

# 检查当前AMD驱动版本
echo -e "\n当前AMD驱动信息:"
clinfo

# 检查是否有可用的系统更新
echo -e "\n正在更新系统包列表..."
sudo apt update

# 安装或更新AMD驱动相关包
echo -e "\n正在安装/更新AMD驱动和OpenCL相关包..."

# 添加Ryzen Developer Mode PPA (如果尚未添加)
if ! grep -q "oibaf/graphics-drivers" /etc/apt/sources.list.d/* 2>/dev/null; then
    echo "添加图形驱动PPA..."
    sudo add-apt-repository -y ppa:oibaf/graphics-drivers
fi

# 更新包列表
sudo apt update

# 安装AMD驱动和OpenCL相关包
sudo apt install -y mesa-vulkan-drivers mesa-vulkan-drivers:i386
sudo apt install -y libvulkan1 libvulkan1:i386
sudo apt install -y libdrm-amdgpu1 xserver-xorg-video-amdgpu

# 检查是否安装了ROCm (AMD的机器学习平台)
if command -v rocm-smi &> /dev/null; then
    echo -e "\n检测到ROCm已安装，版本信息:"
    rocm-smi --version
else
    echo -e "\n未检测到ROCm。如需进行机器学习相关开发，建议安装ROCm。"
    echo "可从 https://rocmdocs.amd.com/en/latest/Installation_Guide/Installation_new.html 获取安装指南"
fi

# 安装或更新OpenCL相关包
sudo apt install -y opencl-headers ocl-icd-opencl-dev clinfo

# 检查OpenCL安装状态
echo -e "\n更新后的OpenCL信息:"
clinfo

echo -e "\nAMD驱动和OpenCL更新完成！"
echo "建议重启系统以确保所有更改生效。"