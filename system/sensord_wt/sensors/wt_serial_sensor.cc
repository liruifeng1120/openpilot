#include "wt_serial_sensor.h"
#include "common/swaglog.h"
#include "common/timing.h"
#include <unistd.h>
#include <cstring>

// 静态实例指针
WTSerialSensor* WTSerialSensor::instance_ = nullptr;

WTSerialSensor::WTSerialSensor(const std::string& device, int baud, uint8_t addr)
    : device_path(device), baud_rate(baud), serial_fd(-1), device_addr(addr) {
  instance_ = this;
}

WTSerialSensor::~WTSerialSensor() {
  shutdown();
  instance_ = nullptr;
}

void WTSerialSensor::serial_write_callback(uint8_t *data, uint32_t len) {
  if (instance_ && instance_->serial_fd >= 0) {
    serial_write_data(instance_->serial_fd, data, len);
  }
}

void WTSerialSensor::delay_ms_callback(uint16_t ms) {
  usleep(ms * 1000);
}

void WTSerialSensor::reg_update_callback(uint32_t reg, uint32_t reg_num) {
  // 数据更新回调，子类可以重写此方法来处理特定的数据更新
  if (instance_) {
    // 基类只记录数据更新时间
    instance_->start_ts = nanos_since_boot();
  }
}

int WTSerialSensor::init() {
  LOGD("Initializing WT serial sensor: %s @ %d baud", device_path.c_str(), baud_rate);

  // 打开串口
  serial_fd = serial_open(device_path.c_str(), baud_rate);
  if (serial_fd < 0) {
    LOGE("Failed to open serial device: %s", device_path.c_str());
    return -1;
  }

  // 初始化 WT SDK
  if (init_wt_sdk() < 0) {
    LOGE("Failed to initialize WT SDK");
    serial_close(serial_fd);
    serial_fd = -1;
    return -1;
  }

  enabled = true;
  LOGD("WT serial sensor initialized successfully");
  return 0;
}

int WTSerialSensor::init_wt_sdk() {
  // 注册回调函数
  WitSerialWriteRegister(serial_write_callback);
  WitDelayMsRegister(delay_ms_callback);
  WitRegisterCallBack(reg_update_callback);

  // 初始化 SDK - PC环境固定使用JY61协议
  int result = WitInit(WIT_PROTOCOL_JY61, device_addr);
  if (result != WIT_HAL_OK) {
    LOGE("WitInit failed with error: %d", result);
    return -1;
  }

  return 0;
}

void WTSerialSensor::cleanup_wt_sdk() {
  WitDeInit();
}

int WTSerialSensor::shutdown() {
  if (enabled) {
    cleanup_wt_sdk();

    if (serial_fd >= 0) {
      serial_close(serial_fd);
      serial_fd = -1;
    }

    enabled = false;
    LOGD("WT serial sensor shut down");
  }
  return 0;
}

bool WTSerialSensor::process_serial_data() {
  if (!enabled || serial_fd < 0) {
    return false;
  }

  unsigned char buffer[256];
  int len = serial_read_data(serial_fd, buffer, sizeof(buffer));

  if (len > 0) {
    // 将接收到的数据传递给 WT SDK 处理
    for (int i = 0; i < len; i++) {
      WitSerialDataIn(buffer[i]);
    }
    return true;
  }

  return false;
}