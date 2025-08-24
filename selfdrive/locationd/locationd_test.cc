#include "selfdrive/locationd/locationd.h"
#include <gtest/gtest.h>

TEST(LocalizerTest, Initialization) {
  Localizer localizer(LocalizerGnssSource::UBLOX);
  EXPECT_TRUE(localizer.get_state().size() > 0);
  EXPECT_TRUE(localizer.get_stdev().size() > 0);
}

TEST(LocalizerTest, HandleSensorData) {
  Localizer localizer(LocalizerGnssSource::UBLOX);
  cereal::SensorEventData::Builder sensor_data;
  sensor_data.setTimestamp(1e9);
  sensor_data.setSource(cereal::SensorEventData::SensorSource::BMX055);
  sensor_data.setSensor(SENSOR_GYRO_UNCALIBRATED);
  sensor_data.setType(SENSOR_TYPE_GYROSCOPE_UNCALIBRATED);
  auto gyro = sensor_data.initGyroUncalibrated();
  gyro.setV({0.1, 0.1, 0.1});
  localizer.handle_sensor(1.0, sensor_data.asReader());
  EXPECT_TRUE(localizer.are_inputs_ok());
}

TEST(LocalizerTest, HandleGpsData) {
  Localizer localizer(LocalizerGnssSource::UBLOX);
  cereal::GpsLocationData::Builder gps_data;
  gps_data.setLatitude(37.7749);
  gps_data.setLongitude(-122.4194);
  gps_data.setAltitude(0.0);
  gps_data.setHorizontalAccuracy(1.0);
  gps_data.setVerticalAccuracy(1.0);
  gps_data.setSpeedAccuracy(0.1);
  gps_data.setBearingAccuracyDeg(1.0);
  gps_data.setHasFix(true);
  gps_data.setUnixTimestampMillis(123456789);
  localizer.handle_gps(1.0, gps_data.asReader(), 0.0);
  EXPECT_TRUE(localizer.are_inputs_ok());
}

TEST(LocalizerTest, HandleGnssData) {
  Localizer localizer(LocalizerGnssSource::UBLOX);
  cereal::GnssMeasurements::Builder gnss_data;
  gnss_data.setMeasTime(1e9);
  auto pos = gnss_data.initPositionECEF();
  pos.setValid(true);
  pos.setValue({0.0, 0.0, 0.0});
  pos.setStd({1.0, 1.0, 1.0});
  auto vel = gnss_data.initVelocityECEF();
  vel.setValid(true);
  vel.setValue({0.0, 0.0, 0.0});
  vel.setStd({0.1, 0.1, 0.1});
  localizer.handle_gnss(1.0, gnss_data.asReader());
  EXPECT_TRUE(localizer.are_inputs_ok());
}

TEST(LocalizerTest, HandleCarState) {
  Localizer localizer(LocalizerGnssSource::UBLOX);
  cereal::CarState::Builder car_state;
  car_state.setVEgo(10.0);
  car_state.setStandstill(false);
  localizer.handle_car_state(1.0, car_state.asReader());
  EXPECT_FALSE(localizer.standstill);
}

TEST(LocalizerTest, HandleCamOdo) {
  Localizer localizer(LocalizerGnssSource::UBLOX);
  cereal::CameraOdometry::Builder cam_odo;
  cam_odo.setRot({0.1, 0.1, 0.1});
  cam_odo.setTrans({0.1, 0.1, 0.1});
  cam_odo.setRotStd({0.01, 0.01, 0.01});
  cam_odo.setTransStd({0.01, 0.01, 0.01});
  localizer.handle_cam_odo(1.0, cam_odo.asReader());
  EXPECT_TRUE(localizer.are_inputs_ok());
}

TEST(LocalizerTest, HandleLiveCalib) {
  Localizer localizer(LocalizerGnssSource::UBLOX);
  cereal::LiveCalibrationData::Builder live_calib;
  live_calib.setRpyCalib({0.1, 0.1, 0.1});
  live_calib.setCalStatus(cereal::LiveCalibrationData::Status::CALIBRATED);
  localizer.handle_live_calib(1.0, live_calib.asReader());
  EXPECT_TRUE(localizer.calibrated);
}

TEST(LocalizerTest, ResetKalman) {
  Localizer localizer(LocalizerGnssSource::UBLOX);
  localizer.reset_kalman(1.0);
  EXPECT_TRUE(localizer.get_state().size() > 0);
}

TEST(LocalizerTest, FiniteCheck) {
  Localizer localizer(LocalizerGnssSource::UBLOX);
  localizer.finite_check(1.0);
  EXPECT_TRUE(localizer.get_state().array().isFinite().all());
}

TEST(LocalizerTest, TimeCheck) {
  Localizer localizer(LocalizerGnssSource::UBLOX);
  localizer.time_check(1.0);
  EXPECT_FALSE(std::isnan(localizer.last_reset_time));
}

TEST(LocalizerTest, UpdateResetTracker) {
  Localizer localizer(LocalizerGnssSource::UBLOX);
  localizer.update_reset_tracker();
  EXPECT_GE(localizer.reset_tracker, 0.0);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}