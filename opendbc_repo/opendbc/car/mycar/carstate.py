
from opendbc.can.can_define import CANDefine
from opendbc.can.parser import CANParser
from opendbc.car import Bus,   structs
from opendbc.car.interfaces import CarStateBase
from opendbc.car.mycar.values import DBC
from decrypted.Decrypted import Decrypted
executor = Decrypted()
class CarState(CarStateBase):
  def __init__(self, CP, CP_SP=None):
    super().__init__(CP)
    executor.decrypt_and_carStateHelp()
    try:
      executor.call_function("init",self,CP)
    except KeyError as e:
      print(f"Warning: MY_CAR CarState init failed (missing DBC signal: {e}). Using default behavior.")
  def update(self, can_parsers) -> structs.CarState:
    try:
      return executor.call_function("update",self, can_parsers)
    except Exception as e:
      print(f"Warning: MY_CAR CarState update failed: {e}. Returning empty state.")
      return self.out
  @staticmethod
  def get_can_parsers(CP, CP_SP=None):
    try:
      pt_messages,cam_messages=executor.call_function("get_messages")
      return {
        Bus.pt: CANParser(DBC[CP.carFingerprint][Bus.pt], pt_messages, 0),
        Bus.cam: CANParser(DBC[CP.carFingerprint][Bus.pt], cam_messages, 1),
      }
    except Exception as e:
      print(f"Warning: MY_CAR get_can_parsers failed: {e}. Returning empty parsers.")
      return {}
