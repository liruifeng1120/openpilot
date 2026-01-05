#pragma once

#include "cereal/messaging/messaging.h"
#include <string>
#include <cstdint>

class WT_Temp {
private:
  bool enabled = true;
  double last_temperature = 0.0;
  uint64_t last_update_ts = 0;

public:
  WT_Temp(const std::string& device = "/dev/ttyUSB0", int baud = 115200);

  bool get_event(MessageBuilder &msg, uint64_t ts = 0);
  int init();
  int shutdown();
  bool is_data_valid(uint64_t current_ts);

protected:
  void update_temp_data();
};