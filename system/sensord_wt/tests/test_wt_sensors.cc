#include <gtest/gtest.h>
#include <memory>
#include "system/sensord_wt/sensors/wt_accel.h"
#include "system/sensord_wt/sensors/wt_gyro.h"
#include "common/timing.h"

class WTSensorTest : public ::testing::Test {
protected:
  void SetUp() override {
    // 测试设置
  }

  void TearDown() override {
    // 测试清理
  }
};

TEST_F(WTSensorTest, AccelSensorCreation) {
  // 测试加速度计传感器创建
  auto accel = std::make_unique<WT_Accel>("/dev/null", 115200, WIT_PROTOCOL_JY61);
  EXPECT_NE(accel, nullptr);
  EXPECT_FALSE(accel->enabled);
}

TEST_F(WTSensorTest, GyroSensorCreation) {
  // 测试陀螺仪传感器创建
  auto gyro = std::make_unique<WT_Gyro>("/dev/null", 115200, WIT_PROTOCOL_JY61);
  EXPECT_NE(gyro, nullptr);
  EXPECT_FALSE(gyro->enabled);
}

TEST_F(WTSensorTest, DataValidation) {
  auto accel = std::make_unique<WT_Accel>();

  // 测试数据有效性检查
  uint64_t current_ts = nanos_since_boot();

  // 初始状态下应该无效
  EXPECT_FALSE(accel->is_data_valid(current_ts));

  // 模拟初始化后的状态
  accel->start_ts = current_ts - 600e6; // 600ms 前
  EXPECT_TRUE(accel->is_data_valid(current_ts));
}