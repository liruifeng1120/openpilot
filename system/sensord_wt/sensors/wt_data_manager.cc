#include "wt_data_manager.h"
#include "common/swaglog.h"
#include "common/timing.h"
#include "serial.h"
#include "wit_c_sdk.h"
#include <unistd.h>

std::unique_ptr<WTDataManager> WTDataManager::instance = nullptr;
std::mutex WTDataManager::instance_mutex;

// 静态回调函数实现
static void serial_write_callback(uint8_t *data, uint32_t len) {
  // 实现串口写功能 - 在这个实现中可能不需要写回功能
  // 如果需要，可以通过全局变量或其他方式访问串口文件描述符
  LOGD("WT SDK serial write callback called with %d bytes", len);
}

static void reg_update_callback(uint32_t reg, uint32_t reg_num) {
  // 实现寄存器更新回调 - 当WT SDK更新寄存器时调用
  LOGD("WT SDK register update: reg=%d, reg_num=%d", reg, reg_num);
}

static void delay_ms_callback(uint16_t ms) {
  // 实现延时功能
  usleep(ms * 1000);
}

WTDataManager::WTDataManager(const std::string& device, int baud)
    : device_path(device), serial_fd(-1),
      last_update_time(0), data_valid(false) {

  // 直接实现串口初始化，而不是依赖抽象类
  serial_fd = serial_open(device.c_str(), baud);
  if (serial_fd < 0) {
    LOGE("Failed to open WT serial device: %s", device.c_str());
  } else {
    LOGD("WT Data Manager initialized successfully on %s @ %d baud",
         device.c_str(), baud);

    // 添加 WT SDK 初始化
    WitInit(WIT_PROTOCOL_NORMAL, 0x50);  // 初始化协议和设备地址
    WitSerialWriteRegister(serial_write_callback);  // 设置串口写回调
    WitRegisterCallBack(reg_update_callback);  // 设置寄存器更新回调
    WitDelayMsRegister(delay_ms_callback);  // 设置延时回调

    LOGD("WT SDK initialized with protocol and callbacks");
  }
}

WTDataManager* WTDataManager::getInstance(const std::string& device, int baud) {
  std::lock_guard<std::mutex> lock(instance_mutex);
  if (!instance) {
    instance = std::unique_ptr<WTDataManager>(new WTDataManager(device, baud));
  }
  return instance.get();
}

WTDataManager* WTDataManager::getInstance() {
  std::lock_guard<std::mutex> lock(instance_mutex);
  return instance.get();
}

bool WTDataManager::updateData() {
  if (serial_fd < 0) return false;

  std::lock_guard<std::mutex> lock(data_mutex);

  // 直接实现串口数据读取
  unsigned char buffer[256];
  int len = serial_read_data(serial_fd, buffer, sizeof(buffer));

  if (len > 0) {
    // 将接收到的数据传递给 WT SDK 处理
    for (int i = 0; i < len; i++) {
      WitSerialDataIn(buffer[i]);
    }
    last_update_time = nanos_since_boot();
    data_valid = true;
    return true;
  }

  return false;
}

bool WTDataManager::isDataValid() {
  std::lock_guard<std::mutex> lock(data_mutex);
  return data_valid && (serial_fd >= 0);
}

uint64_t WTDataManager::getLastUpdateTime() {
  std::lock_guard<std::mutex> lock(data_mutex);
  return last_update_time;
}

WTDataManager::~WTDataManager() {
  if (serial_fd >= 0) {
    close(serial_fd);
  }
}