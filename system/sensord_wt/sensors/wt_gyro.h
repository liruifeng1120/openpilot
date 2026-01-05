#pragma once

#include "cereal/messaging/messaging.h"
#include <string>
#include <cstdint>

class WT_Gyro {
private:
  bool enabled = true;
  double last_gyro_x = 0.0;
  double last_gyro_y = 0.0;
  double last_gyro_z = 0.0;
  uint64_t last_update_ts = 0;

public:
  WT_Gyro(const std::string& device = "/dev/ttyUSB0", int baud = 115200);

  bool get_event(MessageBuilder &msg, uint64_t ts = 0);
  int init() { enabled = true; return 0; }
  int shutdown() { enabled = false; return 0; }
  bool is_data_valid(uint64_t current_ts) { return enabled; }

protected:
  void update_gyro_data();
};