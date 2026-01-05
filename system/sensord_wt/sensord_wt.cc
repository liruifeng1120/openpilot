#include <sys/resource.h>
#include <chrono>
#include <thread>
#include <vector>
#include <map>
#include <poll.h>
#include <getopt.h>

#include "cereal/services.h"
#include "cereal/messaging/messaging.h"
#include "common/ratekeeper.h"
#include "common/swaglog.h"
#include "common/timing.h"
#include "common/util.h"
#include "system/sensord_wt/sensors/wt_accel.h"
#include "system/sensord_wt/sensors/wt_gyro.h"
#include "system/sensord_wt/sensors/wt_magn.h"
#include "system/sensord_wt/sensors/wt_temp.h"
#include "system/sensord_wt/sensors/wit_c_sdk.h"

ExitHandler do_exit;

// PC环境简化配置参数结构 - 只支持串口协议
struct SensordWTConfig {
  std::string device_path = "/dev/ttyUSB0";
  int baud_rate = 115200;
  bool enable_accel = true;
  bool enable_gyro = true;
  bool enable_magn = false;
  bool enable_temp = false;
  bool verbose = false;
};

void print_usage(const char* prog_name) {
  printf("Usage: %s [OPTIONS]\n", prog_name);
  printf("WT Motion Sensor Daemon for PC Environment\n");
  printf("Options:\n");
  printf("  -d, --device PATH     Serial device path (default: /dev/ttyUSB0)\n");
  printf("  -b, --baud RATE       Baud rate (default: 115200)\n");
  printf("  -a, --no-accel        Disable accelerometer\n");
  printf("  -g, --no-gyro         Disable gyroscope\n");
  printf("  -m, --enable-magn     Enable magnetometer\n");
  printf("  -t, --enable-temp     Enable temperature sensor\n");
  printf("  -v, --verbose         Verbose output\n");
  printf("  -h, --help            Show this help\n");
  printf("\nNote: PC environment only supports serial protocol\n");
}

SensordWTConfig parse_args(int argc, char* argv[]) {
  SensordWTConfig config;

  static struct option long_options[] = {
    {"device",      required_argument, 0, 'd'},
    {"baud",        required_argument, 0, 'b'},
    {"no-accel",    no_argument,       0, 'a'},
    {"no-gyro",     no_argument,       0, 'g'},
    {"enable-magn", no_argument,       0, 'm'},
    {"enable-temp", no_argument,       0, 't'},
    {"verbose",     no_argument,       0, 'v'},
    {"help",        no_argument,       0, 'h'},
    {0, 0, 0, 0}
  };

  int c;
  while ((c = getopt_long(argc, argv, "d:b:agmtvh", long_options, nullptr)) != -1) {
    switch (c) {
      case 'd':
        config.device_path = optarg;
        break;
      case 'b':
        config.baud_rate = atoi(optarg);
        break;
      case 'a':
        config.enable_accel = false;
        break;
      case 'g':
        config.enable_gyro = false;
        break;
      case 'm':
        config.enable_magn = true;
        break;
      case 't':
        config.enable_temp = true;
        break;
      case 'v':
        config.verbose = true;
        break;
      case 'h':
        print_usage(argv[0]);
        exit(0);
      default:
        print_usage(argv[0]);
        exit(1);
    }
  }

  return config;
}

template<typename T>
void polling_loop(T *sensor, const std::string& msg_name) {
  PubMaster pm({msg_name.c_str()});
  RateKeeper rk(msg_name, services.at(msg_name).frequency);

  LOGD("Starting polling loop for %s at %dHz", msg_name.c_str(), services.at(msg_name).frequency);

  while (!do_exit) {
    MessageBuilder msg;
    if (sensor->get_event(msg) && sensor->is_data_valid(nanos_since_boot())) {
      pm.send(msg_name.c_str(), msg);
    }
    rk.keepTime();
  }

  LOGD("Stopped polling loop for %s", msg_name.c_str());
}

int sensor_loop(const SensordWTConfig& config) {
  LOGD("Initializing WT sensors for PC environment:");
  LOGD("  Device: %s", config.device_path.c_str());
  LOGD("  Baud rate: %d", config.baud_rate);
  LOGD("  Protocol: JY61 (PC Serial Only)");

  // 使用具体的传感器类型容器
  std::vector<std::pair<WT_Accel*, std::string>> accel_sensors;
  std::vector<std::pair<WT_Gyro*, std::string>> gyro_sensors;
  std::vector<std::pair<WT_Magn*, std::string>> magn_sensors;
  std::vector<std::pair<WT_Temp*, std::string>> temp_sensors;
  std::vector<std::thread> threads;

  int initialized_count = 0;

  // 初始化加速度计
  if (config.enable_accel) {
    auto* accel_sensor = new WT_Accel(config.device_path, config.baud_rate);
    if (accel_sensor->init() >= 0) {
      accel_sensors.emplace_back(accel_sensor, "accelerometer");
      threads.emplace_back(polling_loop<WT_Accel>, accel_sensor, std::string("accelerometer"));
      LOGD("Successfully initialized accelerometer sensor");
      initialized_count++;
    } else {
      LOGE("Failed to initialize accelerometer sensor");
      delete accel_sensor;
    }
  }

  // 初始化陀螺仪
  if (config.enable_gyro) {
    auto* gyro_sensor = new WT_Gyro(config.device_path, config.baud_rate);
    if (gyro_sensor->init() >= 0) {
      gyro_sensors.emplace_back(gyro_sensor, "gyroscope");
      threads.emplace_back(polling_loop<WT_Gyro>, gyro_sensor, std::string("gyroscope"));
      LOGD("Successfully initialized gyroscope sensor");
      initialized_count++;
    } else {
      LOGE("Failed to initialize gyroscope sensor");
      delete gyro_sensor;
    }
  }

  // 初始化磁力计
  if (config.enable_magn) {
    auto* magn_sensor = new WT_Magn(config.device_path, config.baud_rate);
    if (magn_sensor->init() >= 0) {
      magn_sensors.emplace_back(magn_sensor, "magnetometer");
      threads.emplace_back(polling_loop<WT_Magn>, magn_sensor, std::string("magnetometer"));
      LOGD("Successfully initialized magnetometer sensor");
      initialized_count++;
    } else {
      LOGE("Failed to initialize magnetometer sensor");
      delete magn_sensor;
    }
  }

  // 初始化温度传感器
  if (config.enable_temp) {
    auto* temp_sensor = new WT_Temp(config.device_path, config.baud_rate);
    if (temp_sensor->init() >= 0) {
      temp_sensors.emplace_back(temp_sensor, "temperatureSensor");
      threads.emplace_back(polling_loop<WT_Temp>, temp_sensor, std::string("temperatureSensor"));
      LOGD("Successfully initialized temperature sensor");
      initialized_count++;
    } else {
      LOGE("Failed to initialize temperature sensor");
      delete temp_sensor;
    }
  }

  // 检查是否有传感器被成功初始化
  if (initialized_count == 0) {
    LOGE("No sensors enabled or initialized successfully");
    return -1;
  }

  LOGD("Successfully initialized %d sensors", initialized_count);

  // 提高进程优先级
  setpriority(PRIO_PROCESS, 0, -18);

  LOGD("WT sensor daemon is running... Press Ctrl+C to stop");

  // 等待所有线程结束
  for (auto &t : threads) {
    t.join();
  }

  // 清理资源
  for (auto &[sensor, msg_name] : accel_sensors) {
    sensor->shutdown();
    delete sensor;
  }
  for (auto &[sensor, msg_name] : gyro_sensors) {
    sensor->shutdown();
    delete sensor;
  }
  for (auto &[sensor, msg_name] : magn_sensors) {
    sensor->shutdown();
    delete sensor;
  }
  for (auto &[sensor, msg_name] : temp_sensors) {
    sensor->shutdown();
    delete sensor;
  }

  LOGD("WT sensor daemon stopped");
  return 0;
}

int main(int argc, char *argv[]) {
  // 解析命令行参数
  SensordWTConfig config = parse_args(argc, argv);

  // 设置日志级别
  if (config.verbose) {
    setenv("LOGPRINT", "debug", 1);
  }

  LOGD("Starting WT Motion Sensor Daemon");

  try {
    return sensor_loop(config);
  } catch (std::exception &e) {
    LOGE("Exception in sensor loop: %s", e.what());
    return -1;
  }
}