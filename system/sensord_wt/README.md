# WT 陀螺仪传感器服务 (sensord_wt)

## 概述

`sensord_wt` 是专为 PC 环境设计的 WT 陀螺仪传感器服务，完全参照 `system/sensord` 的实现方式。该服务使用纯 C++ 实现，通过串口协议（JY61）与 WT 陀螺仪传感器通信，并通过 cereal 消息系统发布传感器数据。

## 项目结构

```
system/sensord_wt/
├── sensors/              # 传感器实现
│   ├── wt_sensor.h      # 传感器基类接口
│   ├── wt_serial_sensor.h  # 串口传感器类声明
│   ├── wt_serial_sensor.cc # 串口传感器类实现
│   └── wt_motion_sdk/   # WT Motion C SDK
├── sensord_wt.cc        # 主服务程序
├── SConscript           # 构建配置
└── README.md            # 说明文档
```

## 技术特性

- **纯 C++ 实现**: 避免 Python 和 C 混用的复杂性
- **串口通信**: PC 环境专用串口协议（JY61）
- **模块化设计**: 基于继承的传感器类架构
- **cereal 集成**: 与 openpilot 消息系统无缝集成
- **SCons 构建**: 集成到项目构建系统
- **简单管理**: 完全按照 sensord 的方式管理，无额外配置文件

## 服务管理

服务完全按照原来 sensord 的方式在 `system/manager/process_config.py` 中管理：

```python
# WT 陀螺仪传感器服务（仅PC环境）
NativeProcess("sensord_wt", "system/sensord_wt", ["./sensord_wt"], only_onroad, enabled=PC)
```

## 编译和运行

### 编译

```bash
# 在项目根目录下编译
scons -j8
```

### 运行

服务会自动通过 manager 启动，也可以手动启动：

```bash
cd system/sensord_wt
./sensord_wt
```

## 消息发布

服务发布以下 cereal 消息：

- `sensorEvents`: 包含陀螺仪和加速度计数据
- `gyrometer`: 陀螺仪角速度数据
- `accelerometer`: 加速度计数据

## 开发说明

### 添加新传感器类型

1. 继承 `WTSensor` 基类
2. 实现必要的虚函数
3. 在 `sensord_wt.cc` 中注册新传感器

### 调试

```bash
# 查看日志
tail -f /tmp/sensord_wt.log

# 查看消息
cereal-log sensorEvents
```