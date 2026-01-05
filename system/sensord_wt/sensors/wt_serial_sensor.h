#pragma once

#include "wt_sensor.h"
#include "wit_c_sdk.h"
#include "serial.h"
#include <string>
#include <memory>

class WTSerialSensor : public WTSensor {
protected:
  std::string device_path;
  int baud_rate;
  int serial_fd;
  uint8_t device_addr;

  // WT SDK 回调函数
  static void serial_write_callback(uint8_t *data, uint32_t len);
  static void delay_ms_callback(uint16_t ms);
  static void reg_update_callback(uint32_t reg, uint32_t reg_num);

  // 实例管理
  static WTSerialSensor* instance_;

public:
  WTSerialSensor(const std::string& device, int baud, uint8_t addr = 0x50);
  virtual ~WTSerialSensor();

  virtual int init() override;
  virtual int shutdown() override;
  virtual bool has_interrupt_enabled() override { return false; } // 使用轮询模式

protected:
  bool process_serial_data();
  int init_wt_sdk();
  void cleanup_wt_sdk();
};