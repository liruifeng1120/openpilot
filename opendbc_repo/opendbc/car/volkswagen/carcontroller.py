import numpy as np
from opendbc.can import CANPacker
from opendbc.car import Bus, DT_CTRL, apply_driver_steer_torque_limits, structs
from opendbc.car.lateral import apply_std_curvature_limits
from opendbc.car.common.conversions import Conversions as CV
from opendbc.car.interfaces import CarControllerBase
from opendbc.car.volkswagen import mqbcan, pqcan, mebcan, mlbcan
from opendbc.car.volkswagen.values import CANBUS, CanBus, CarControllerParams, VolkswagenFlags
from opendbc.car.volkswagen.mebutils import LongControlJerk, LongControlLimit, LatControlCurvature

VisualAlert = structs.CarControl.HUDControl.VisualAlert
LongCtrlState = structs.CarControl.Actuators.LongControlState


class CarController(CarControllerBase):
  def __init__(self, dbc_names, CP):
    super().__init__(dbc_names, CP)
    self.CCP = CarControllerParams(CP)
    self.CAN = CanBus(CP)
    self.packer_pt = CANPacker(dbc_names[Bus.pt])

    if CP.flags & VolkswagenFlags.PQ:
      self.CCS = pqcan
    elif CP.flags & VolkswagenFlags.MLB:
      self.CCS = mlbcan
    elif CP.flags & (VolkswagenFlags.MEB | VolkswagenFlags.MQB_EVO):
      self.CCS = mebcan
    else:
      self.CCS = mqbcan

    # SD uses the CANBUS constants for the legacy MQB/PQ torque paths
    self.ext_bus = CANBUS.pt if CP.networkLocation == structs.CarParams.NetworkLocation.fwdCamera else CANBUS.cam
    self.aeb_available = not CP.flags & VolkswagenFlags.PQ

    self.apply_torque_last = 0
    self.apply_curvature_last = 0.
    self.steering_power_last = 0
    self.accel_last = 0.
    self.gra_acc_counter_last = None
    self.klr_counter_last = None
    self.lead_distance_bars_last = None
    self.distance_bar_frame = 0
    self.long_override_counter = 0
    self.long_disabled_counter = 0
    self.hide_ea_error = False
    self.eps_timer_soft_disable_alert = False
    self.hca_frame_timer_running = 0
    self.hca_frame_same_torque = 0

    self.is_evo = bool(CP.flags & (VolkswagenFlags.MEB | VolkswagenFlags.MQB_EVO))
    self.long_jerk_control = LongControlJerk(dt=(DT_CTRL * self.CCP.ACC_CONTROL_STEP)) if self.is_evo else None
    self.long_limit_control = LongControlLimit(dt=(DT_CTRL * self.CCP.ACC_CONTROL_STEP)) if self.is_evo else None
    self.LateralController = (
      LatControlCurvature(self.CCP.CURVATURE_PID, self.CCP.CURVATURE_LIMITS.CURVATURE_MAX, 1 / (DT_CTRL * self.CCP.STEER_STEP))
      if self.is_evo else None
    )

  def update(self, CC, CS, now_nanos):
    actuators = CC.actuators
    hud_control = CC.hudControl
    can_sends = []

    # **** Steering Controls ************************************************ #

    if self.frame % self.CCP.STEER_STEP == 0:
      if self.is_evo:
        if CC.latActive:
          hca_enabled = True
          if CC.curvatureControllerActive:
            apply_curvature = self.LateralController.update(CS.out, CC, actuators.curvature)
            apply_curvature = apply_curvature + (CS.out.steeringCurvature - (CC.currentCurvature - CC.rollCompensation))
          else:
            apply_curvature = actuators.curvature + (CS.out.steeringCurvature - CC.currentCurvature)
          apply_curvature = apply_std_curvature_limits(
              apply_curvature,
              self.apply_curvature_last,
              CS.out.vEgoRaw,
              CS.out.steeringCurvature,
              CS.out.steeringPressed,
              self.CCP.STEER_STEP,
              CC.latActive,
              self.CCP.CURVATURE_LIMITS,
          )

          # Low speed curvature filter below 40 km/h to reduce oscillation
          if CS.out.vEgo < 11.1:
            alpha = np.interp(CS.out.vEgo, [0.0, 5.0, 11.1], [0.30, 0.55, 1.00])
            apply_curvature = alpha * apply_curvature + (1.0 - alpha) * self.apply_curvature_last

          min_power = max(self.steering_power_last - self.CCP.STEERING_POWER_STEP, self.CCP.STEERING_POWER_MIN)
          max_power = min(self.steering_power_last + self.CCP.STEERING_POWER_STEP, self.CCP.STEERING_POWER_MAX)
          target_power_driver = int(np.interp(CS.out.steeringTorque, [self.CCP.STEER_DRIVER_ALLOWANCE, self.CCP.STEER_DRIVER_MAX],
                                              [self.CCP.STEERING_POWER_MAX, self.CCP.STEERING_POWER_MIN]))
          target_power = int(np.interp(CS.out.vEgo, [0., 0.5], [self.CCP.STEERING_POWER_MIN, target_power_driver]))
          steering_power = min(max(target_power, min_power), max_power)
        else:
          self.LateralController.reset()
          if self.steering_power_last > 0:
            hca_enabled = True
            apply_curvature = float(np.clip(CS.out.steeringCurvature, -self.CCP.CURVATURE_LIMITS.CURVATURE_MAX, self.CCP.CURVATURE_LIMITS.CURVATURE_MAX))
            steering_power = max(self.steering_power_last - self.CCP.STEERING_POWER_STEP, 0)
          else:
            hca_enabled = False
            apply_curvature = 0.
            steering_power = 0

        can_sends.append(self.CCS.create_steering_control(self.packer_pt, self.CAN.pt, apply_curvature, hca_enabled, steering_power))
        self.apply_curvature_last = apply_curvature
        self.steering_power_last = steering_power

      else:
        # Legacy MQB/PQ torque path
        apply_torque = 0
        if CC.latActive:
          new_torque = int(round(actuators.torque * self.CCP.STEER_MAX))
          apply_torque = apply_driver_steer_torque_limits(new_torque, self.apply_torque_last, CS.out.steeringTorque, self.CCP)
          self.hca_frame_timer_running += self.CCP.STEER_STEP
          if self.apply_torque_last == apply_torque:
            self.hca_frame_same_torque += self.CCP.STEER_STEP
            if self.hca_frame_same_torque > self.CCP.STEER_TIME_STUCK_TORQUE / DT_CTRL:
              apply_torque -= (1, -1)[apply_torque < 0]
              self.hca_frame_same_torque = 0
          else:
            self.hca_frame_same_torque = 0
          hca_enabled = abs(apply_torque) > 0
        else:
          hca_enabled = False

        if not hca_enabled:
          self.hca_frame_timer_running = 0
        self.eps_timer_soft_disable_alert = self.hca_frame_timer_running > self.CCP.STEER_TIME_ALERT / DT_CTRL
        self.apply_torque_last = apply_torque
        can_sends.append(self.CCS.create_steering_control(self.packer_pt, CANBUS.pt, apply_torque, hca_enabled))

        if self.CP.flags & VolkswagenFlags.STOCK_HCA_PRESENT:
          ea_simulated_torque = float(np.clip(apply_torque * 2, -self.CCP.STEER_MAX, self.CCP.STEER_MAX))
          if abs(CS.out.steeringTorque) > abs(ea_simulated_torque):
            ea_simulated_torque = CS.out.steeringTorque
          can_sends.append(mqbcan.create_eps_update(self.packer_pt, CANBUS.cam, CS.eps_stock_values, ea_simulated_torque))

    # Emergency Assist capacitive wheel touch (MEB / MQB Evo)
    if self.is_evo and self.CP.flags & VolkswagenFlags.STOCK_KLR_PRESENT:
      klr_send_ready = CS.klr_stock_values["COUNTER"] != self.klr_counter_last
      if klr_send_ready:
        can_sends.append(mebcan.create_capacitive_wheel_touch(self.packer_pt, self.CAN.cam, CC.latActive, CS.klr_stock_values))
        can_sends.append(mebcan.create_capacitive_wheel_touch(self.packer_pt, self.CAN.pt, CC.latActive, CS.klr_stock_values))
      self.klr_counter_last = CS.klr_stock_values["COUNTER"]

    # Wechselblinken only on non-Gen2 MEB/Evo (EA blinker)
    if self.is_evo and not (self.CP.flags & VolkswagenFlags.MQB_EVO_GEN2):
      if self.frame % 2 == 0:
        blinker_active = CS.left_blinker_active or CS.right_blinker_active
        left_blinker = CC.leftBlinker if not blinker_active else False
        right_blinker = CC.rightBlinker if not blinker_active else False
        can_sends.append(mebcan.create_blinker_control(self.packer_pt, self.CAN.pt, CS.ea_hud_stock_values, CS.ea_control_stock_values,
                                                       left_blinker, right_blinker, self.hide_ea_error))

    # **** Acceleration Controls ******************************************** #

    if self.frame % self.CCP.ACC_CONTROL_STEP == 0 and self.CP.openpilotLongitudinalControl and not CS.out.radarDisableFailed:
      stopping = actuators.longControlState == LongCtrlState.stopping

      if self.is_evo:
        starting = actuators.longControlState == LongCtrlState.starting and CS.out.vEgo <= self.CP.vEgoStarting
        accel = float(np.clip(actuators.accel, self.CCP.ACCEL_MIN, self.CCP.ACCEL_MAX) if CC.enabled else 0)

        long_override = CC.cruiseControl.override or CS.out.gasPressed
        self.long_override_counter = min(self.long_override_counter + 1, 5) if long_override else 0
        long_override_begin = long_override and self.long_override_counter < 5
        self.long_disabled_counter = min(self.long_disabled_counter + 1, 5) if not CC.enabled else 0
        long_disabling = not CC.enabled and self.long_disabled_counter < 5

        # longComfortMode is not exposed on this fork's carControl; default comfort off
        acc_control = self.CCS.acc_control_value(CS.out.cruiseState.available, CS.out.accFaulted, CC.enabled, long_override)
        acc_hold_type = self.CCS.acc_hold_type(CS.out.cruiseState.available, CS.out.accFaulted, CC.enabled, starting, stopping,
                                               CS.esp_hold_confirmation, long_override, long_override_begin, long_disabling)
        can_sends.extend(self.CCS.create_acc_accel_control(self.packer_pt, self.CAN.pt, self.CP, CS.acc_type, CC.enabled,
                                                           4.0, 4.0, 0., 0.,
                                                           accel, acc_control, acc_hold_type, stopping, starting, CS.esp_hold_confirmation,
                                                           CS.out.vEgoRaw * CV.MS_TO_KPH, long_override, CS.travel_assist_available))
      else:
        starting = actuators.longControlState == LongCtrlState.pid and (CS.esp_hold_confirmation or CS.out.vEgo < self.CP.vEgoStopping)
        accel = float(np.clip(actuators.accel, self.CCP.ACCEL_MIN, self.CCP.ACCEL_MAX) if CC.longActive else 0)
        acc_control = self.CCS.acc_control_value(CS.out.cruiseState.available, CS.out.accFaulted, CC.longActive)
        can_sends.extend(self.CCS.create_acc_accel_control(self.packer_pt, CANBUS.pt, CS.acc_type, CC.longActive, accel,
                                                           acc_control, stopping, starting, CS.esp_hold_confirmation))
      self.accel_last = accel

    # Note: the MEB radar-disable (AEB/FCW replacement messages + UDS tester present)
    # workflow from upstream is NOT ported here. On gateway-harness cars that rely on
    # radar disable for openpilot longitudinal, this will not behave correctly.

    # **** HUD Controls ***************************************************** #

    if self.frame % self.CCP.LDW_STEP == 0:
      hud_alert = 0
      if hud_control.visualAlert in (VisualAlert.steerRequired, VisualAlert.ldw):
        hud_alert = self.CCP.LDW_MESSAGES["laneAssistTakeOver"]

      if self.is_evo:
        # disableCarSteerAlerts is not exposed on this fork's carControl
        sound_alert = self.CCP.LDW_SOUNDS["Chime"] if hud_alert == self.CCP.LDW_MESSAGES["laneAssistTakeOver"] else self.CCP.LDW_SOUNDS["None"]
        can_sends.append(self.CCS.create_lka_hud_control(self.packer_pt, self.CAN.pt, self.CP, CS.ldw_stock_values, CC.latActive,
                                                         CS.out.steeringPressed, hud_alert, hud_control, sound_alert))
      else:
        can_sends.append(self.CCS.create_lka_hud_control(self.packer_pt, CANBUS.pt, CS.ldw_stock_values, CC.latActive,
                                                         CS.out.steeringPressed, hud_alert, hud_control))

    if hud_control.leadDistanceBars != self.lead_distance_bars_last:
      self.distance_bar_frame = self.frame

    if self.frame % self.CCP.ACC_HUD_STEP == 0 and self.CP.openpilotLongitudinalControl and not CS.out.radarDisableFailed:
      if self.is_evo:
        fcw_alert = hud_control.visualAlert == VisualAlert.fcw
        show_distance_bars = self.frame - self.distance_bar_frame < 400
        # leadFollowTime is not exposed on this fork's HUDControl, use a fixed gap
        gap = max(8, CS.out.vEgo * 1.8)
        distance = max(8, hud_control.leadDistance) if hud_control.leadDistance != 0 else 0
        acc_hud_status = self.CCS.acc_hud_status_value(CS.out.cruiseState.available, CS.out.accFaulted, CC.enabled,
                                                       CC.cruiseControl.override or CS.out.gasPressed)
        # speed-limit HUD driven by cruiseState.speedLimit (zeroed upstream since speed_limit_manager is not ported)
        sl_predicative_active = False
        sl_active = False
        speed_limit = 0
        acc_hud_event = self.CCS.acc_hud_event(acc_hud_status, CS.esp_hold_confirmation, sl_predicative_active, CS.speed_limit_predicative_type, sl_active)
        can_sends.append(self.CCS.create_acc_hud_control(self.packer_pt, self.CAN.pt, acc_hud_status, hud_control.setSpeed * CV.MS_TO_KPH,
                                                         hud_control.leadVisible, hud_control.leadDistanceBars + 1, show_distance_bars,
                                                         CS.esp_hold_confirmation, distance, gap, fcw_alert,
                                                         acc_hud_event, speed_limit, self.CP))
      else:
        lead_distance = 0
        if hud_control.leadVisible and self.frame * DT_CTRL > 1.0:
          lead_distance = 512 if CS.upscale_lead_car_signal else 8
        acc_hud_status = self.CCS.acc_hud_status_value(CS.out.cruiseState.available, CS.out.accFaulted, CC.longActive)
        set_speed = hud_control.setSpeed * CV.MS_TO_KPH
        can_sends.append(self.CCS.create_acc_hud_control(self.packer_pt, CANBUS.pt, acc_hud_status, set_speed,
                                                         lead_distance, hud_control.leadDistanceBars))

    # **** Stock ACC Button Controls **************************************** #

    gra_send_ready = CS.gra_stock_values["COUNTER"] != self.gra_acc_counter_last
    if gra_send_ready:
      if self.CP.pcmCruise and (CC.cruiseControl.cancel or CC.cruiseControl.resume):
        bus_send = self.CAN.ext if self.is_evo else self.ext_bus
        can_sends.append(self.CCS.create_acc_buttons_control(self.packer_pt, bus_send, CS.gra_stock_values,
                                                             cancel=CC.cruiseControl.cancel, resume=CC.cruiseControl.resume))

    new_actuators = actuators.as_builder()
    new_actuators.torque = self.apply_torque_last / self.CCP.STEER_MAX
    new_actuators.torqueOutputCan = self.apply_torque_last
    new_actuators.curvature = float(self.apply_curvature_last)
    new_actuators.accel = self.accel_last
    new_actuators.speed = actuators.speed

    self.lead_distance_bars_last = hud_control.leadDistanceBars
    self.gra_acc_counter_last = CS.gra_stock_values["COUNTER"]
    self.frame += 1
    return new_actuators, can_sends