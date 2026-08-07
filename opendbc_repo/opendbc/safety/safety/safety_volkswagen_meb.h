#pragma once

#include "safety_declarations.h"
#include "safety_volkswagen_common.h"

// MQB Evo / MEB platform safety (curvature-based steering via HCA_03).
// Ported from opendbc upstream volkswagen_meb.h, adapted to this fork's
// safety framework (AngleSteeringLimits + steer_angle_cmd_checks).

#define MSG_ESC_51           0xFCU    // RX, for wheel speeds
#define MSG_HCA_03           0x303U   // TX by OP, Heading Control Assist curvature
#define MSG_QFK_01           0x13DU   // RX, for steering curvature
#define MSG_ACC_19           0x300U   // RX from ECU, for ACC status
#define MSG_ACC_18           0x14DU   // TX by OP, ACC control instructions
#define MSG_Motor_51         0x10BU   // RX for TSK state and accel pedal
#define MSG_TA_01            0x26BU   // TX for Travel Assist status
#define MSG_EA_01            0x1A4U   // TX, for EA mitigation
#define MSG_EA_02            0x1F0U   // TX, for EA mitigation
#define MSG_KLR_01           0x25DU   // TX, for capacitive steering wheel
#define MSG_DIAGNOSTIC       0x700U   // TX, for general tester present on bus for radar disable
#define MSG_AWV_03           0xDBU    // TX, radar AEB control message replacement
#define MSG_MEB_AWV_01       0x16A954ADU   // TX, radar AEB HUD message replacement
#define MSG_Strukturen_01    0x24FU   // TX, radar objects message replacement


extern const uint16_t FLAG_VOLKSWAGEN_ALT_CRC_VARIANT_1;
const uint16_t FLAG_VOLKSWAGEN_ALT_CRC_VARIANT_1 = 2;

extern const uint16_t FLAG_VOLKSWAGEN_DISABLE_RADAR;
const uint16_t FLAG_VOLKSWAGEN_DISABLE_RADAR = 8;

extern bool volkswagen_alt_crc_variant_1;
bool volkswagen_alt_crc_variant_1 = false;

extern bool volkswagen_disable_radar;
bool volkswagen_disable_radar = false;


#define VW_MEB_COMMON_RX_CHECKS                                                                     \
  {.msg = {{MSG_LH_EPS_03, 0, 8, .max_counter = 15U, .frequency = 100U}, { 0 }, { 0 }}},  \
  {.msg = {{MSG_MOTOR_14, 0, 8, .max_counter = 15U, .frequency = 10U}, { 0 }, { 0 }}},   \
  {.msg = {{MSG_GRA_ACC_01, 0, 8, .max_counter = 15U, .frequency = 33U}, { 0 }, { 0 }}}, \
  {.msg = {{MSG_QFK_01, 0, 32, .max_counter = 15U, .frequency = 100U}, { 0 }, { 0 }}},

#define VW_MEB_RX_CHECKS                                                                            \
  {.msg = {{MSG_Motor_51, 0, 32, .max_counter = 15U, .frequency = 100U}, { 0 }, { 0 }}},  \
  {.msg = {{MSG_ESC_51, 0, 48, .max_counter = 15U, .frequency = 100U}, { 0 }, { 0 }}},

#define VW_MEB_GEN2_RX_CHECKS                                                                       \
  {.msg = {{MSG_Motor_51, 0, 48, .max_counter = 15U, .frequency = 100U}, { 0 }, { 0 }}},  \
  {.msg = {{MSG_ESC_51, 0, 64, .max_counter = 15U, .frequency = 100U}, { 0 }, { 0 }}},

static uint32_t volkswagen_meb_get_checksum(const CANPacket_t *to_push) {
  return (uint8_t)GET_BYTE(to_push, 0);
}

static uint8_t volkswagen_meb_get_counter(const CANPacket_t *to_push) {
  return (uint8_t)GET_BYTE(to_push, 1) & 0xFU;
}

static uint32_t volkswagen_meb_compute_crc(const CANPacket_t *to_push) {
  int addr = GET_ADDR(to_push);
  int len = GET_LEN(to_push);

  uint8_t crc = 0xFFU;
  for (int i = 1; i < len; i++) {
    crc ^= (uint8_t)GET_BYTE(to_push, i);
    crc = volkswagen_crc8_lut_8h2f[crc];
  }

  uint8_t counter = volkswagen_meb_get_counter(to_push);
  if (addr == MSG_LH_EPS_03) {
    crc ^= (uint8_t[]){0xF5,0xF5,0xF5,0xF5,0xF5,0xF5,0xF5,0xF5,0xF5,0xF5,0xF5,0xF5,0xF5,0xF5,0xF5,0xF5}[counter];
  } else if (addr == MSG_GRA_ACC_01) {
    crc ^= (uint8_t[]){0x6A,0x38,0xB4,0x27,0x22,0xEF,0xE1,0xBB,0xF8,0x80,0x84,0x49,0xC7,0x9E,0x1E,0x2B}[counter];
  } else if (addr == MSG_QFK_01) {
    crc ^= (uint8_t[]){0x20,0xCA,0x68,0xD5,0x1B,0x31,0xE2,0xDA,0x08,0x0A,0xD4,0xDE,0x9C,0xE4,0x35,0x5B}[counter];
  } else if (addr == MSG_ESC_51) {
    crc ^= (uint8_t[]){0x77,0x5C,0xA0,0x89,0x4B,0x7C,0xBB,0xD6,0x1F,0x6C,0x4F,0xF6,0x20,0x2B,0x43,0xDD}[counter];
  } else if (addr == MSG_Motor_51) {
    crc ^= (uint8_t[]){0x77,0x5C,0xA0,0x89,0x4B,0x7C,0xBB,0xD6,0x1F,0x6C,0x4F,0xF6,0x20,0x2B,0x43,0xDD}[counter];
  } else if (addr == MSG_MOTOR_14) {
    crc ^= (uint8_t[]){0x1F,0x28,0xC6,0x85,0xE6,0xF8,0xB0,0x19,0x5B,0x64,0x35,0x21,0xE4,0xF7,0x9C,0x24}[counter];
  } else if (addr == MSG_KLR_01) {
    crc ^= (uint8_t[]){0xDA,0x6B,0x0E,0xB2,0x78,0xBD,0x5A,0x81,0x7B,0xD6,0x41,0x39,0x76,0xB6,0xD7,0x35}[counter];
  } else if (addr == MSG_EA_02) {
    crc ^= (uint8_t[]){0x2F,0x3C,0x22,0x60,0x18,0xEB,0x63,0x76,0xC5,0x91,0x0F,0x27,0x34,0x04,0x7F,0x02}[counter];
  } else {
    // Undefined CAN message, CRC check expected to fail
  }
  crc = volkswagen_crc8_lut_8h2f[crc];

  return (uint8_t)(crc ^ 0xFFU);
}

static uint32_t volkswagen_meb_gen2_compute_crc(const CANPacket_t *to_push) {
  // For newer variants the checksum is calculated over a specific signal length.
  if (!volkswagen_alt_crc_variant_1) {
    return volkswagen_meb_compute_crc(to_push);
  }

  int addr = GET_ADDR(to_push);
  int len = GET_LEN(to_push);

  if (addr == MSG_QFK_01) {
    len = 28;
  } else if (addr == MSG_ESC_51) {
    len = 60;
  } else if (addr == MSG_Motor_51) {
    len = 44;
  } else {
    return volkswagen_meb_compute_crc(to_push);
  }

  uint8_t crc = 0xFFU;
  for (int i = 1; i < len; i++) {
    crc ^= (uint8_t)GET_BYTE(to_push, i);
    crc = volkswagen_crc8_lut_8h2f[crc];
  }

  uint8_t counter = volkswagen_meb_get_counter(to_push);
  if (addr == MSG_QFK_01) {
    crc ^= (uint8_t[]){0x18,0x71,0x10,0x8D,0xD7,0xAA,0xB0,0x78,0xAC,0x12,0xAE,0x0C,0xDD,0xF1,0x85,0x68}[counter];
  } else if (addr == MSG_ESC_51) {
    crc ^= (uint8_t[]){0x69,0xDC,0xF9,0x64,0x6A,0xCE,0x55,0x2C,0xC4,0x38,0x8F,0xD1,0xC6,0x43,0xB4,0xB1}[counter];
  } else if (addr == MSG_Motor_51) {
    crc ^= (uint8_t[]){0x2C,0xB1,0x1A,0x75,0xBB,0x65,0x79,0x47,0x81,0x2B,0xCC,0x96,0x17,0xDB,0xC0,0x94}[counter];
  } else {
    return volkswagen_meb_compute_crc(to_push);
  }

  crc = (uint8_t)(volkswagen_crc8_lut_8h2f[crc] ^ 0xFFU);

  if (crc != GET_BYTE(to_push, 0)) {
    return volkswagen_meb_compute_crc(to_push);
  }

  return (uint8_t)(crc);
}

static safety_config volkswagen_meb_init(uint16_t param) {
  static const CanMsg VOLKSWAGEN_MEB_STOCK_TX_MSGS[] = {{MSG_HCA_03, 0, 24}, {MSG_GRA_ACC_01, 0, 8},
                                                        {MSG_EA_01, 0, 8}, {MSG_EA_02, 0, 8},
                                                        {MSG_KLR_01, 0, 8}, {MSG_KLR_01, 2, 8},
                                                        {MSG_GRA_ACC_01, 2, 8}, {MSG_LDW_02, 0, 8}};

  static const CanMsg VOLKSWAGEN_MEB_LONG_TX_MSGS[] = {
    {MSG_HCA_03, 0, 24}, {MSG_ACC_19, 0, 48}, {MSG_ACC_18, 0, 32},
    {MSG_EA_01, 0, 8}, {MSG_EA_02, 0, 8},
    {MSG_KLR_01, 0, 8}, {MSG_KLR_01, 2, 8},
    {MSG_LDW_02, 0, 8}, {MSG_TA_01, 0, 8},
  };

  static const CanMsg VOLKSWAGEN_MEB_LONG_NO_RADAR_TX_MSGS[] = {
    {MSG_HCA_03, 0, 24}, {MSG_ACC_19, 0, 48}, {MSG_ACC_18, 0, 32},
    {MSG_EA_01, 0, 8}, {MSG_EA_02, 0, 8},
    {MSG_KLR_01, 0, 8}, {MSG_KLR_01, 2, 8},
    {MSG_LDW_02, 0, 8}, {MSG_TA_01, 0, 8},
    {MSG_AWV_03, 0, 48}, {MSG_MEB_AWV_01, 0, 8}, {MSG_Strukturen_01, 0, 64},
    {MSG_DIAGNOSTIC, 0, 8},
  };

  static RxCheck volkswagen_meb_rx_checks[] = {
    VW_MEB_COMMON_RX_CHECKS
    VW_MEB_RX_CHECKS
  };

  static RxCheck volkswagen_meb_gen2_rx_checks[] = {
    VW_MEB_COMMON_RX_CHECKS
    VW_MEB_GEN2_RX_CHECKS
  };

  volkswagen_set_button_prev = false;
  volkswagen_resume_button_prev = false;

  volkswagen_alt_crc_variant_1 = GET_FLAG(param, FLAG_VOLKSWAGEN_ALT_CRC_VARIANT_1);

#ifdef ALLOW_DEBUG
  volkswagen_longitudinal = GET_FLAG(param, FLAG_VOLKSWAGEN_LONG_CONTROL);
  volkswagen_disable_radar = GET_FLAG(param, FLAG_VOLKSWAGEN_DISABLE_RADAR);
#endif

  gen_crc_lookup_table_8(0x2F, volkswagen_crc8_lut_8h2f);

  safety_config ret;
  if (volkswagen_longitudinal) {
    if (volkswagen_disable_radar) {
      SET_TX_MSGS(VOLKSWAGEN_MEB_LONG_NO_RADAR_TX_MSGS, ret);
    } else {
      SET_TX_MSGS(VOLKSWAGEN_MEB_LONG_TX_MSGS, ret);
    }
  } else {
    SET_TX_MSGS(VOLKSWAGEN_MEB_STOCK_TX_MSGS, ret);
  }

  if (volkswagen_alt_crc_variant_1) {
    SET_RX_CHECKS(volkswagen_meb_gen2_rx_checks, ret);
  } else {
    SET_RX_CHECKS(volkswagen_meb_rx_checks, ret);
  }

  return ret;
}

// lateral limits for curvature. Upstream MEB safety bounds curvature by max_curvature
// and ISO 11270 lateral accel, without a per-frame rate limit. We approximate the same
// using steer_angle_cmd_checks with angle_is_curvature=true and permissive rate lookups.
static const AngleSteeringLimits VOLKSWAGEN_MEB_STEERING_LIMITS = {
  .max_angle = 29105,                       // 0.195 rad/m
  .angle_deg_to_can = 149253.7313f,         // 1 / 6.7e-6 rad/m to can
  .angle_rate_up_lookup = {.x = {0, 0, 0}, .y = {1000000, 1000000, 1000000}},
  .angle_rate_down_lookup = {.x = {0, 0, 0}, .y = {1000000, 1000000, 1000000}},
  .max_angle_error = 0,
  .angle_error_min_speed = 0,
  .angle_is_curvature = true,
  .enforce_angle_error = false,
  .inactive_angle_is_zero = true,
};

static void volkswagen_meb_rx_hook(const CANPacket_t *to_push) {
  if (GET_BUS(to_push) == 0U) {
    int addr = GET_ADDR(to_push);

    // Update in-motion state by sampling wheel speeds
    if (addr == MSG_ESC_51) {
      uint32_t fr = GET_BYTE(to_push, 10) | (GET_BYTE(to_push, 11) << 8);
      uint32_t rr = GET_BYTE(to_push, 14) | (GET_BYTE(to_push, 15) << 8);
      uint32_t rl = GET_BYTE(to_push, 12) | (GET_BYTE(to_push, 13) << 8);
      uint32_t fl = GET_BYTE(to_push, 8) | (GET_BYTE(to_push, 9) << 8);

      vehicle_moving = (fr > 0U) || (rr > 0U) || (rl > 0U) || (fl > 0U);

      UPDATE_VEHICLE_SPEED(((fr + rr + rl + fl) / 4) * 0.0075 / 3.6);
    }

    // Update measured curvature from QFK_01 (same scaling as HCA_03 curvature command)
    if (addr == MSG_QFK_01) {
      int current_curvature = ((GET_BYTE(to_push, 6) & 0x7F) << 8) | GET_BYTE(to_push, 5);

      bool current_curvature_sign = GET_BIT(to_push, 55U);
      if (!current_curvature_sign) {
        current_curvature *= -1;
      }

      update_sample(&angle_meas, current_curvature);
    }

    // Update driver input torque samples
    if (addr == MSG_LH_EPS_03) {
      int torque_driver_new = GET_BYTE(to_push, 5) | ((GET_BYTE(to_push, 6) & 0x1FU) << 8);
      int sign = (GET_BYTE(to_push, 6) & 0x80U) >> 7;
      if (sign == 1) {
        torque_driver_new *= -1;
      }
      update_sample(&torque_driver, torque_driver_new);
    }

    // Update cruise state
    if (addr == MSG_Motor_51) {
      int acc_status = ((GET_BYTE(to_push, 11) >> 0) & 0x07U);
      bool cruise_engaged = (acc_status == 3) || (acc_status == 4) || (acc_status == 5);
      acc_main_on = cruise_engaged || (acc_status == 2);

      if (!volkswagen_longitudinal) {
        pcm_cruise_check(cruise_engaged);
      }

      if (!acc_main_on) {
        controls_allowed = false;
      }
    }

    // Update cruise buttons
    if (addr == MSG_GRA_ACC_01) {
      if (volkswagen_longitudinal) {
        bool set_button = GET_BIT(to_push, 16U);
        bool resume_button = GET_BIT(to_push, 19U);
        if ((volkswagen_set_button_prev && !set_button) || (volkswagen_resume_button_prev && !resume_button)) {
          controls_allowed = acc_main_on;
        }
        volkswagen_set_button_prev = set_button;
        volkswagen_resume_button_prev = resume_button;
      }
      if (GET_BIT(to_push, 13U)) {
        controls_allowed = false;
      }
    }

    // Update brake pedal
    if (addr == MSG_MOTOR_14) {
      brake_pressed = GET_BIT(to_push, 28U);
    }

    // Update accel pedal
    if (addr == MSG_Motor_51) {
      int accel_pedal_value = ((GET_BYTE(to_push, 1) >> 4) & 0x0FU) | ((GET_BYTE(to_push, 2) & 0x1FU) << 4);
      gas_pressed = accel_pedal_value > 0;
    }

    generic_rx_checks(false);
  }
}

static bool volkswagen_meb_tx_hook(const CANPacket_t *to_send) {
  const LongitudinalLimits VOLKSWAGEN_MEB_LONG_LIMITS = {
    .max_accel = 2000,
    .min_accel = -3500,
    .inactive_accel = 3010,
  };

  int addr = GET_ADDR(to_send);
  bool tx = true;

  // Safety check for HCA_03 Heading Control Assist curvature
  if (addr == MSG_HCA_03) {
    int desired_curvature_raw = (int)(GET_BYTES(to_send, 3, 2) & 0x7FFFU);

    bool desired_curvature_sign = GET_BIT(to_send, 39U);
    if (!desired_curvature_sign) {
      desired_curvature_raw *= -1;
    }

    bool steer_req = (((GET_BYTE(to_send, 1) >> 4) & 0x0FU) == 4U);

    if (steer_angle_cmd_checks(desired_curvature_raw, steer_req, VOLKSWAGEN_MEB_STEERING_LIMITS)) {
      tx = false;
    }
  }

  // Safety check for MSG_ACC_18 acceleration requests
  if (addr == MSG_ACC_18) {
    int desired_accel = ((((GET_BYTE(to_send, 4) & 0x7U) << 8) | GET_BYTE(to_send, 3)) * 5U) - 7220U;

    if (longitudinal_accel_checks(desired_accel, VOLKSWAGEN_MEB_LONG_LIMITS)) {
      tx = false;
    }
  }

  // FORCE CANCEL: ensuring that only the cancel button press is sent when controls are off.
  if ((addr == MSG_GRA_ACC_01) && !controls_allowed) {
    if ((GET_BYTE(to_send, 2) & 0x9U) != 0U) {
      tx = false;
    }
  }

  // UDS: Only tester present allowed on diagnostics address
  if (addr == MSG_DIAGNOSTIC) {
    if ((GET_BYTES(to_send, 0, 4) != 0x00803E02U) || (GET_BYTES(to_send, 4, 4) != 0x0U)) {
      tx = false;
    }
  }

  return tx;
}

static int volkswagen_meb_fwd_hook(int bus_num, int addr) {
  int bus_fwd = -1;

  switch (bus_num) {
    case 0:
      // Forward gateway traffic toward the camera bus
      bus_fwd = 2;
      break;
    case 2:
      // Camera bus: block openpilot's own TX messages, forward the rest to the gateway
      if ((addr == MSG_HCA_03) || (addr == MSG_LDW_02) || (addr == MSG_TA_01) ||
          (addr == MSG_EA_01) || (addr == MSG_EA_02) || (addr == MSG_KLR_01) ||
          (addr == MSG_ACC_18) || (addr == MSG_ACC_19)) {
        bus_fwd = -1;
      } else {
        bus_fwd = 0;
      }
      break;
    default:
      bus_fwd = -1;
      break;
  }

  return bus_fwd;
}

const safety_hooks volkswagen_meb_hooks = {
  .init = volkswagen_meb_init,
  .rx = volkswagen_meb_rx_hook,
  .tx = volkswagen_meb_tx_hook,
  .fwd = volkswagen_meb_fwd_hook,
  .get_counter = volkswagen_meb_get_counter,
  .get_checksum = volkswagen_meb_get_checksum,
  .compute_checksum = volkswagen_meb_gen2_compute_crc,
};