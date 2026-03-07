"""
Copyright © IQ.Lvbs, apart of Project Teal Lvbs, All Rights Reserved, licensed under https://konn3kt.com/tos
"""
from cereal import messaging
from numpy import interp
from opendbc.car import structs
from openpilot.common.params import Params
from openpilot.common.realtime import DT_MDL
from openpilot.iqpilot.selfdrive.controls.lib.iq_dynamic.imahelper import (
  IQConstants,
  IQFilterEngine,
  IQModeEngine,
  IQ_DYNAMIC_MODE_PARAM,
  compute_slowdown_need,
)

S_Y = 33


class IQDynamicController:
  def __init__(self, CP: structs.CarParams, mpc, params=None):
    self.IQS = CP
    self._mpc = mpc
    self.IQParams = params or Params()

    self.IQDynamicStatus = self.IQParams.get_bool(IQ_DYNAMIC_MODE_PARAM)
    self.IQDynamicA = False
    self.IQDynamicF = 0
    self.IQDynamicU = 0.0

    self.IQEngineManager = IQModeEngine()

    self.IQFilterL = IQFilterEngine(measurement_noise=0.17, process_noise=0.04, process_decay=1.03, smoothing_floor=0.9)
    self.IQFilterSDL = IQFilterEngine(measurement_noise=0.12, process_noise=0.098, process_decay=1.01, smoothing_floor=0.8)
    self.IQFilterSFL = IQFilterEngine(measurement_noise=0.11, process_noise=0.06, process_decay=1.000, smoothing_floor=0.90)
    self.IQFilterFCW = IQFilterEngine(measurement_noise=0.19, process_noise=0.11, process_decay=1.11, smoothing_floor=0.4)

    self.hasIQFilterLED = False
    self.hasIQSDL = False
    self.hasIQSFL = False
    self.hasIQL = False

    self.kph = 0.0
    self.cruise_kph = 0.0
    self.ss = False
    self.aeb = 0
    self.aeb_c = 0
    self.ss_c = 0
    self.e_x = float('inf')
    self.e_d = 0.0
    self.t_follow_v = False

  def _readIQParams(self) -> None:
    if self.IQDynamicF % int(1. / DT_MDL) == 0:
      self.IQDynamicStatus = self.IQParams.get_bool(IQ_DYNAMIC_MODE_PARAM)

  def mode(self) -> str:
    return self.IQEngineManager.get_mode()

  def enabled(self) -> bool:
    return self.IQDynamicStatus

  def active(self) -> bool:
    return self.IQDynamicA

  def setaeb(self) -> None:
    self.aeb = self.aeb_c

  def _update_vehicle_state(self, sm: messaging.SubMaster) -> None:
    car_state = sm['carState']
    self.kph = car_state.vEgo * 3.6
    self.cruise_kph = car_state.vCruise
    self.ss = car_state.standstill
    if self.ss:
      self.ss_c = min(20, self.ss_c + 1)
    else:
      self.ss_c = max(0, self.ss_c - 1)

  def _update_lead_state(self, lead_status: float) -> None:
    self.IQFilterL.push(float(lead_status))
    lead_value = self.IQFilterL.value() or 0.0
    self.hasIQFilterLED = lead_value > IQConstants.LEAD_LOCK_GATE

  def _update_fcw_state(self) -> None:
    previous_fcw = self.IQFilterFCW.value() or 0.0
    self.IQFilterFCW.push(float(self.aeb > 0))
    self.hasIQL = previous_fcw > 0.5

  def _update_curve_slowdown_state(self, model) -> None:
    self.e_x = float('inf')
    self.t_follow_v = False

    valid_position = len(model.position.x) == S_Y
    valid_orientation = len(model.orientation.x) == S_Y
    if not (valid_position and valid_orientation):
      need = 0.3 if self.kph > 20.0 else 0.0
      self.IQFilterSDL.push(need)
      urgency = self.IQFilterSDL.value() or 0.0
      self.hasIQSDL = urgency > IQConstants.BRAKE_CURVE_GATE
      self.IQDynamicU = urgency
      return

    self.t_follow_v = True
    horizon_dist = model.position.x[S_Y - 1]
    desired_dist = interp(self.kph, IQConstants.BRAKE_CURVE_SPEED_AXIS, IQConstants.BRAKE_CURVE_DISTANCE_AXIS)

    self.e_x = horizon_dist
    self.e_d = desired_dist

    need = compute_slowdown_need(self.kph, horizon_dist, desired_dist)
    self.IQFilterSDL.push(need)
    urgency = self.IQFilterSDL.value() or 0.0

    self.hasIQSDL = urgency > (IQConstants.BRAKE_CURVE_GATE * 0.8)
    self.IQDynamicU = urgency

  def _update_slowness_state(self) -> None:
    if self.ss_c > 5 or self.hasIQSDL:
      return

    slowness_observed = float(self.kph <= (self.cruise_kph * IQConstants.CRUISE_LAG_RATIO_GATE))
    self.IQFilterSFL.push(slowness_observed)
    slowness_value = self.IQFilterSFL.value() or 0.0
    threshold = IQConstants.CRUISE_LAG_GATE * (0.8 if self.hasIQSFL else 1.1)
    self.hasIQSFL = slowness_value > threshold

  def IQSCL(self, model) -> None:
    self._update_curve_slowdown_state(model)

  def IQDynamicEngine(self, sm: messaging.SubMaster) -> None:
    self._update_vehicle_state(sm)
    self._update_lead_state(sm['radarState'].leadOne.status)
    self._update_fcw_state()
    self._update_curve_slowdown_state(sm['modelV2'])
    self._update_slowness_state()

  def IQStateEngine(self) -> None:
    if self.hasIQL:
      self.IQEngineManager.request('blended', urgency=1.0, emergency=True)
      return

    if self.ss_c > 3:
      self.IQEngineManager.request('blended', urgency=0.9)
      return

    if self.hasIQSDL:
      if self.IQDynamicU > 0.7:
        self.IQEngineManager.request('blended', urgency=1.0, emergency=True)
      else:
        self.IQEngineManager.request('blended', urgency=min(1.0, self.IQDynamicU * 1.5))
      return

    if self.hasIQSFL and not self.hasIQSDL:
      self.IQEngineManager.request('acc', urgency=0.8)
      return

    self.IQEngineManager.request('acc', urgency=0.7)

  def IQStateEngine_R(self) -> None:
    if self.hasIQL:
      self.IQEngineManager.request('blended', urgency=1.0, emergency=True)
      return

    if self.hasIQFilterLED and not (self.ss_c > 3):
      self.IQEngineManager.request('acc', urgency=1.0)
      return

    if self.hasIQSDL:
      if self.IQDynamicU > 0.7:
        self.IQEngineManager.request('blended', urgency=1.0, emergency=True)
      else:
        self.IQEngineManager.request('blended', urgency=min(1.0, self.IQDynamicU * 1.3))
      return

    if self.ss_c > 3:
      self.IQEngineManager.request('blended', urgency=0.9)
      return

    if self.hasIQSFL and not self.hasIQSDL:
      self.IQEngineManager.request('acc', urgency=0.8)
      return

    self.IQEngineManager.request('acc', urgency=0.7)

  def update(self, sm: messaging.SubMaster) -> None:
    self._readIQParams()
    self.setaeb()
    self.IQDynamicEngine(sm)

    if self.IQS.radarUnavailable:
      self.IQStateEngine()
    else:
      self.IQStateEngine_R()

    self.IQEngineManager.update()
    self.IQDynamicA = sm['selfdriveState'].experimentalMode and self.IQDynamicStatus
    self.IQDynamicF += 1
