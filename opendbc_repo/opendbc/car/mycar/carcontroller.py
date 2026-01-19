
from opendbc.car.interfaces import CarControllerBase
from decrypted.Decrypted import Decrypted
executor = Decrypted()
class CarController(CarControllerBase):
  def __init__(self, dbc_names, CP, CP_SP=None):
    super().__init__(dbc_names, CP)
    try:
      executor.decrypt_and_carcontrollerHelp()
      self.instance = executor.get_class_instance("CarController", dbc_names)
    except Exception as e:
      print(f"Warning: MY_CAR CarController init failed: {e}. Using default behavior.")
      self.instance = None
  def update(self, CC, CC_SP, CS, now_nanos):
    return self.instance.update(CC, CS)
