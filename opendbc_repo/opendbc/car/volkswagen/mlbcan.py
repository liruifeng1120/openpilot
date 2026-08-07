# MLB platform CAN helpers are not ported to this fork (no MLB vehicles are
# supported here). This stub exists so `from opendbc.car.volkswagen import mlbcan`
# resolves for the controller's import line; MLB code paths are never entered.

def create_steering_control(packer, bus, apply_steer, lkas_enabled):
  raise NotImplementedError("MLB platform is not supported in this fork")


def create_lka_hud_control(packer, bus, ldw_stock_values, enabled, steering_pressed, hud_alert, hud_control):
  raise NotImplementedError("MLB platform is not supported in this fork")
