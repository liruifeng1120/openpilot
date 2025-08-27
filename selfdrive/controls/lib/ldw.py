from cereal import log
from openpilot.common.realtime import DT_CTRL
from openpilot.common.conversions import Conversions as CV


CAMERA_OFFSET = 0.04
LDW_MIN_SPEED = 31 * CV.MPH_TO_MS
LANE_DEPARTURE_THRESHOLD = 0.1

class LaneDepartureWarning:
  def __init__(self):
    self.left = False
    self.right = False
    self.last_blinker_frame = 0

  def update(self, frame, modelV2, CS, CC):
    # LDW功能已禁用 - 始终设置为False
    self.left, self.right = False, False

  @property
  def warning(self) -> bool:
    return bool(self.left or self.right)
