#!/usr/bin/env python3
import os
import numpy as np
import capnp
from collections import deque
from functools import partial

import cereal.messaging as messaging
from cereal import car, log
from cereal.services import SERVICE_LIST
from openpilot.common.params import Params
from openpilot.common.realtime import config_realtime_process
from openpilot.common.swaglog import cloudlog
from openpilot.selfdrive.locationd.helpers import fft_next_good_size, parabolic_peak_interp

BLOCK_SIZE = 100
BLOCK_NUM = 50
BLOCK_NUM_NEEDED = 5
MOVING_WINDOW_SEC = 60.0
MIN_OKAY_WINDOW_SEC = 25.0
MIN_RECOVERY_BUFFER_SEC = 2.0

# 基础参数设置
BASE_PARAMS = {
  'max_lag': 1.0,
  'max_lag_std': 0.1,
  'min_ncc': 0.95,
  'min_vego': 15.0,
  'max_lat_accel': 2.0,
  'max_lat_accel_diff': 0.6,
  'min_confidence': 0.7,
  'corr_border_offset': 5,
  'lag_candidate_corr_threshold': 0.9,
}

# 针对不同车型的参数调整
CAR_PARAMS = {
  'HONDA_ACCORD_HYBRID': {
    'min_vego': 10.0,  # 降低最小速度要求
    'max_lag': 0.5,    # 缩小最大延迟范围
    'min_ncc': 0.90,   # 稍微降低相关性阈值
  },
  'HONDA_CIVIC': {
    'min_vego': 12.0,
    'max_lag': 0.6,
    'min_ncc': 0.92,
  },
  # 可以在这里添加更多车型的特定参数
}


def update_estimate(self):
    if not self.points_enough():
      return

    times, desired, actual, okay = self.points.get()
    # check if there are any new valid data points since the last update
    is_valid = self.points_valid()
    if self.last_estimate_t != 0 and times[0] <= self.last_estimate_t:
      new_values_start_idx = next(-i for i, t in enumerate(reversed(times)) if t <= self.last_estimate_t)
      is_valid = is_valid and not (new_values_start_idx == 0 or not np.any(okay[new_values_start_idx:]))

    # 针对本田雅阁混动车的特殊处理
    if self.CP.carFingerprint == "HONDA ACCORD" and "Hybrid" in [car_docs.car_name for car_docs in self.CP.carDocs]:
      # 混动车的转向系统响应特性可能不同，使用更严格的参数
      max_lag = HONDA_ACCORD_HYBRID_LAG_PARAMS['max_lag']
      min_ncc = HONDA_ACCORD_HYBRID_LAG_PARAMS['min_ncc']
    else:
      max_lag = MAX_LAG
      min_ncc = MIN_NCC

    delay, corr, confidence = self.actuator_delay(desired, actual, okay, self.dt, max_lag)
    if corr < min_ncc or confidence < MIN_CONFIDENCE or not is_valid:
      return

    self.block_avg.update(delay)
    self.last_estimate_t = self.t
