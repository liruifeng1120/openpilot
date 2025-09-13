## ⚠️ 法律声明 / Legal Notice

本软件仅供研究和教育用途。开发者不对实际安装和使用承担任何责任。

In accordance with the amended **Korean Motor Vehicle Management Act** (effective August 14, 2025),
**modifying or installing software that affects the safe operation of a vehicle** is prohibited.

This software is provided **for research and educational use only**.
The developer does **not take any responsibility** for real-world installation or usage.
## ⚠️ 安装libicu66，注意安装完成删掉deb http://security.ubuntu.com/ubuntu focal-security main。
**libicu66安装步骤1**
``` bash
sudo apt-get update
sudo apt-get install gedit
sudo gedit /etc/apt/sources.list
```
**libicu66安装步骤2 粘贴下段内容到第2行并保存**
``` bash
deb http://security.ubuntu.com/ubuntu focal-security main
```
**libicu66安装步骤3**
``` bash
sudo apt-get update
sudo apt-get install libicu66
```
## ⚠️ 编译后执行根目录下setup_jy62_permissions.sh脚本，如果没有权限请执行下列代码
``` bash
chmod +x /home/$LOGNAME/ajouatom/setup_jy62_permissions.sh
```
**运行simple_jy62_test.py脚本检查JY62设备是否正常工作**
``` bash
cd /home/$LOGNAME/ajouatom && python3 simple_jy62_test.py
```
**检查/dev/ttyUSB0设备是否存在以及权限设置**
``` bash
ls -l /dev/ttyUSB*
```
**检查当前用户是否属于dialout组**
``` bash
groups
```
**将当前用户添加到dialout组**
``` bash
sudo usermod -a -G dialout $LOGNAME
```
**应用组权限更改**
``` bash
newgrp dialout
```
**检查当前用户所属的组**
``` bash
groups
```
**重新登录shell以应用组更改**
``` bash
exec su -l $LOGNAME
```
**重启**
``` bash
sudo reboot
```

**运行JY62测试脚本检查设备是否正常工作**
``` bash
cd /home/$LOGNAME/ajouatom && python3 simple_jy62_test.py
```
**使用更详细的测试脚本检查JY62设备**
``` bash
cd /home/$LOGNAME/ajouatom && python3 test_jy62_device.py
```
**启动打印accelerometer, gyroscope的xyz参数**
``` bash
python3 selfdrive/sensord/sensors/jy62_sensor.py


<div align="center" style="text-align: center;">

<h1>ajouatom</h1>

<h3>
  基于 openpilot 和 JY62 IMU 设备的自动驾驶研究平台
</h3>

![openpilot tests](https://github.com/commaai/openpilot/actions/workflows/selfdrive_tests.yaml/badge.svg)
[![codecov](https://codecov.io/gh/commaai/openpilot/branch/master/graph/badge.svg)](https://codecov.io/gh/commaai/openpilot)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![X Follow](https://img.shields.io/twitter/follow/comma_ai)](https://x.com/comma_ai)
[![Discord](https://img.shields.io/discord/469524606043160576)](https://discord.comma.ai)

</div>

## 简介

ajouatom 是一个基于 [openpilot](https://github.com/commaai/openpilot) 的自动驾驶研究平台，专门针对使用 JY62 IMU 设备的开发和测试环境。该项目主要用于科研、实验和仿真目的。

## 功能特性

- 支持 JY62 IMU 设备的数据采集与处理
- 基于 openpilot 的自动驾驶功能
- 完整的传感器测试和验证工具
- 适用于研究和开发的模块化架构

## 硬件要求

- JY62 IMU 设备
- USB 转串口适配器（如果需要）
- 运行 Ubuntu 20.04 或 24.04 的计算机

## 软件要求

- Python 3.8+
- Docker (可选)
- SCons 构建系统

## 安装指南

### 1. 克隆仓库

```bash
git clone https://github.com/liruifeng1120/ajouatom.git
cd ajouatom
```

### 2. 配置 JY62 设备权限

按照上面的说明配置设备权限。


## 项目结构

```
ajouatom/
├── cereal/              # 消息传递和通信功能
├── common/              # 通用工具和库
├── selfdrive/           # 核心自动驾驶功能模块
├── system/              # 系统级服务和管理模块
├── tools/               # 开发和调试工具脚本
├── panda/               # 与硬件通信的模块
├── opendbc_repo/        # 车辆数据库
├── README.md            # 项目说明文档
└── ...
```

## 开发指南

### 代码规范

- 遵循 PEP 8 Python 代码规范
- 使用类型提示
- 编写单元测试

### 贡献流程

1. Fork 项目
2. 创建功能分支
3. 提交更改
4. 发起 Pull Request

## 许可证

本项目基于 MIT 许可证发布。有关详细信息，请参阅 [LICENSE](LICENSE) 文件。

## 免责声明

⚠️ 本软件仅供研究和教育用途。在实际车辆上使用本软件可能违反当地法律法规。使用本软件的风险由您自行承担。