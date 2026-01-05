#pragma once

#include "cereal/messaging/messaging.h"
#include <string>
#include <cstdint>

class WT_Magn {
private:
  bool enabled = true;
  double last_magn_x = 0.0;
  double last_magn_y = 0.0;
  double last_magn_z = 0.0;
  uint64_t last_update_ts = 0;

public:
  WT_Magn(const std::string& device = "/dev/ttyUSB0", int baud = 115200);

  bool get_event(MessageBuilder &msg, uint64_t ts = 0);
  int init();
  int shutdown();
  bool is_data_valid(uint64_t current_ts);

protected:
  void update_magn_data();
};