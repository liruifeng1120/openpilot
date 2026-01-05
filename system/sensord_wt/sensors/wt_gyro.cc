#include "wt_gyro.h"
#include "common/swaglog.h"
#include "common/timing.h"
#include "REG.h"
#include "wit_c_sdk.h"
#include <cmath>
#include "constants.h"
#include "wt_data_manager.h"
WT_Gyro::WT_Gyro(const std::string& device, int baud) {
  LOGD("Creating WT gyroscope sensor for PC environment");
  // 确保数据管理器已初始化
  WTDataManager::getInstance(device, baud);
}

void WT_Gyro::update_gyro_data() {
  // 从 WT SDK 寄存器读取陀螺仪数据
  // 陀螺仪数据范围通常是 ±2000°/s，寄存器值范围是 ±32768
  // 转换为 rad/s
  const double DEG_TO_RAD = M_PI / 180.0;
  last_gyro_x = (double)sReg[GX] / 32768.0 * 2000.0 * DEG_TO_RAD;
  last_gyro_y = (double)sReg[GY] / 32768.0 * 2000.0 * DEG_TO_RAD;
  last_gyro_z = (double)sReg[GZ] / 32768.0 * 2000.0 * DEG_TO_RAD;
  last_update_ts = nanos_since_boot();

  // 添加详细的数据日志
  LOGD("WT Gyro: raw[%d,%d,%d] -> scaled[%.6f,%.6f,%.6f] rad/s (ts=%lu)",
       sReg[GX], sReg[GY], sReg[GZ],
       last_gyro_x, last_gyro_y, last_gyro_z, last_update_ts);
}

bool WT_Gyro::get_event(MessageBuilder &msg, uint64_t ts) {
  LOGD("WT_Gyro::get_event called, enabled=%s", enabled ? "true" : "false");

  if (!enabled) {
    return false;
  }

  // 从统一数据管理器获取数据
  WTDataManager* data_manager = WTDataManager::getInstance();
  if (!data_manager->updateData() || !data_manager->isDataValid()) {
    return false;
  }

  // 更新陀螺仪数据
  update_gyro_data();

  LOGD("WT_Gyro: Building gyroscope message, enabled=%s", enabled ? "true" : "false");

  auto event = msg.initEvent().initGyroscope();
  event.setSource(cereal::SensorEventData::SensorSource::WT_SDK);
  event.setVersion(1);
  event.setSensor(SENSOR_GYRO_UNCALIBRATED);
  event.setType(SENSOR_TYPE_GYROSCOPE_UNCALIBRATED);
  event.setTimestamp(ts == 0 ? nanos_since_boot() : ts);

  auto gyro = event.initGyroUncalibrated();
  // WT传感器安装方向映射到标准车辆坐标系
  // locationd转换: meas = [-v[2], -v[1], -v[0]]
  // WT轴向: Y轴(前进-俯仰), X轴(左右-横滚), Z轴(上下-偏航)
  // 目标车辆坐标系: meas[0]=roll(横滚), meas[1]=pitch(俯仰), meas[2]=yaw(偏航)
  //
  // 修复：确保陀螺仪轴映射正确
  // v[0] → meas[2] = yaw(偏航) → 使用WT的Z轴
  // v[1] → meas[1] = pitch(俯仰) → 使用WT的Y轴
  // v[2] → meas[0] = roll(横滚) → 使用WT的X轴
  gyro.setV({{(float)last_gyro_z, (float)(-last_gyro_y), (float)(-last_gyro_x)}});
  gyro.setStatus(true);

  LOGD("WT_Gyro: Message built successfully");
  return true;
}