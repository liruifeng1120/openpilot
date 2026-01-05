#include "wt_accel.h"
#include "common/swaglog.h"
#include "common/timing.h"
#include "REG.h"
#include "wt_data_manager.h"
#include "wit_c_sdk.h"
#include "constants.h"

WT_Accel::WT_Accel(const std::string& device, int baud) {  LOGD("Creating WT accelerometer sensor for PC environment");
  // 确保数据管理器已初始化
  WTDataManager::getInstance(device, baud);
}

void WT_Accel::update_accel_data() {
  // 从 WT SDK 寄存器读取加速度数据
  // 加速度数据范围通常是 ±16g，寄存器值范围是 ±32768
  // 转换为 m/s²
  last_accel_x = (double)sReg[AX] / 32768.0 * 16.0 * 9.8;
  last_accel_y = (double)sReg[AY] / 32768.0 * 16.0 * 9.8;
  last_accel_z = (double)sReg[AZ] / 32768.0 * 16.0 * 9.8;
  last_update_ts = nanos_since_boot();

  // 添加详细的数据日志
  LOGD("WT Accel: raw[%d,%d,%d] -> scaled[%.6f,%.6f,%.6f] m/s² (ts=%lu)",
       sReg[AX], sReg[AY], sReg[AZ],
       last_accel_x, last_accel_y, last_accel_z, last_update_ts);
}

bool WT_Accel::get_event(MessageBuilder &msg, uint64_t ts) {
  if (!enabled) {
    return false;
  }

  // 从统一数据管理器获取数据
  WTDataManager* data_manager = WTDataManager::getInstance();
  if (!data_manager->updateData() || !data_manager->isDataValid()) {
    return false;
  }

  // 更新加速度数据
  update_accel_data();

  // 构建 accelerometer 消息
  auto event = msg.initEvent();
  event.setLogMonoTime(ts == 0 ? nanos_since_boot() : ts);

  auto accel = event.initAccelerometer();
  accel.setVersion(1);
  accel.setSensor(1);
  accel.setType(1);
  accel.setTimestamp(last_update_ts);
  accel.setSource(cereal::SensorEventData::SensorSource::WT_SDK);

  // 设置加速度数据
  auto acceleration = accel.initAcceleration();
  auto v = acceleration.initV(3);
  // WT传感器安装方向映射到标准车辆坐标系
  // locationd转换: meas = [-v[2], -v[1], -v[0]]
  // WT轴向: Y轴(前进), X轴(左右), Z轴(上下)
  // 目标: meas[0]=X轴(前进), meas[1]=Y轴(左右), meas[2]=Z轴(上下)
  //
  // 修复：根据WT传感器安装方向正确映射轴向
  // WT安装：Y轴(前进), X轴(左右), Z轴(上下)
  // locationd转换: meas = [-v[2], -v[1], -v[0]]
  // 目标：meas[0]=前进, meas[1]=左右, meas[2]=上下(重力+9.8)
  //
  // 正确映射：
  // v[0] → meas[2] = 上下轴 → 使用WT的Z轴，静止时应为+9.8
  // v[1] → meas[1] = 左右轴 → 使用WT的X轴
  // v[2] → meas[0] = 前进轴 → 使用WT的Y轴
  v.set(0, last_accel_z);      // WT Z轴 → v[0] → meas[2] = 车辆Z轴(垂直)
  v.set(1, -last_accel_x);     // WT X轴 → v[1] → meas[1] = 车辆Y轴(横向)
  v.set(2, -last_accel_y);     // WT Y轴 → v[2] → meas[0] = 车辆X轴(纵向)
  acceleration.setStatus(0);

  return true;
}