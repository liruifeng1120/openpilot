#pragma once

#include <string>
#include <mutex>
#include <memory>
#include <cstdint>

class WTDataManager {
private:
  static std::unique_ptr<WTDataManager> instance;
  static std::mutex instance_mutex;

  std::string device_path;
  int serial_fd;
  mutable std::mutex data_mutex;  // 添加 mutable
  uint64_t last_update_time;
  bool data_valid;

  WTDataManager(const std::string& device, int baud);

public:
  static WTDataManager* getInstance(const std::string& device, int baud);
  static WTDataManager* getInstance();  // 无参数版本

  bool updateData();
  bool isDataValid();  // 移除 const
  uint64_t getLastUpdateTime();  // 移除 const

  ~WTDataManager();

  // 禁止拷贝和赋值
  WTDataManager(const WTDataManager&) = delete;
  WTDataManager& operator=(const WTDataManager&) = delete;
};