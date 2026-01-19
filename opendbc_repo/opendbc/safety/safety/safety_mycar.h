#pragma once

#include "safety_declarations.h"

#define ACC_MPC_STATE     0x340
#define ACC_ADAS          0x260
#define PEDAL             0x220
#define CARSPEED          0x366
#define ACCELRATORPEDAL   0x140


static void car_rx_hook(const CANPacket_t *to_push) {
    int bus = GET_BUS(to_push);
  int addr = GET_ADDR(to_push);
  if (bus == 0) {
    if (addr == PEDAL) {
      brake_pressed = ((GET_BYTE(to_push, 0) & 0b11111100) != 0U);
    } else if (addr == ACCELRATORPEDAL) {
       gas_pressed = (GET_BYTE(to_push, 1) != 0U);
    }  else if (addr == CARSPEED) {
      int speed_raw = GET_BYTE(to_push, 5);
      vehicle_moving = (speed_raw != 0);
    }else if (addr == ACC_ADAS) {
      bool gac_cruise_engaged= (GET_BYTE(to_push, 3) & 0b00001100) != 0U ;
      pcm_cruise_check(gac_cruise_engaged);
    }
  }
}


static bool car_tx_hook(const CANPacket_t *to_send) {
  const TorqueSteeringLimits LIMITS = { //values to be check
    .max_steer = 300,
    .max_rate_up = 17,
    .max_rate_down = 17,
    .max_rt_delta = 243,
    .max_torque_error = 80,
    .type = TorqueMotorLimited,
  };

  bool tx = true;
  int bus = GET_BUS(to_send);

  if (bus == 0) {
    int addr = GET_ADDR(to_send);
    if (addr == ACC_MPC_STATE) {
      int desired_torque = ((GET_BYTE(to_send, 3) & 0x03U) << 8U) | GET_BYTE(to_send, 2);
      desired_torque=1024-desired_torque;
      desired_torque=0;
      if (steer_torque_cmd_checks(desired_torque, -1, LIMITS)) {
        tx = true;
      }
    }

  }

  return tx;
}

static int car_fwd_hook(int bus, int addr) {
  int block_msg = 0;
  if ( bus == 2 ) {
     block_msg = (addr == ACC_MPC_STATE);
  }
  return block_msg;
}

static safety_config car_init(uint16_t param) {

  static const CanMsg CAR_MSGS[] = {
     {ACC_MPC_STATE,    0, 8},
  };
  static RxCheck car_checks[] = {
      {.msg = {{ACC_ADAS,     0, 8, .ignore_checksum = true, .ignore_counter = true, .frequency = 100U}, { 0 }, { 0 }}},
      {.msg = {{PEDAL,      0, 8, .ignore_checksum = true, .ignore_counter = true, .frequency = 100U}, { 0 }, { 0 }}},
      {.msg = {{CARSPEED,    0, 8, .ignore_checksum = true, .ignore_counter = true, .frequency = 100U}, { 0 }, { 0 }}},
      {.msg = {{ACCELRATORPEDAL,  0, 8, .ignore_checksum = true, .ignore_counter = true, .frequency = 100U}, { 0 }, { 0 }}},
    };
  UNUSED(param);
  return BUILD_SAFETY_CFG(car_checks, CAR_MSGS);
}

const safety_hooks car_hooks = {
  .init = car_init,
  .rx = car_rx_hook,
  .tx = car_tx_hook,
  .fwd = car_fwd_hook,
};

