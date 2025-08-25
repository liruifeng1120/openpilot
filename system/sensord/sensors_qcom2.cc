#include <sys/resource.h>
#include "system/hardware/hw.h"
#include <chrono>
#include <thread>
#include <vector>
#include <map>
#include <poll.h>
#include <linux/gpio.h>
#include <signal.h>
#include <unistd.h>
#include "cereal/services.h"
#include "cereal/messaging/messaging.h"
#include "common/i2c.h"
#include "common/ratekeeper.h"
#include "common/swaglog.h"
#include "common/timing.h"
#include "common/util.h"
#include "system/sensord/sensors/bmx055_accel.h"
#include "system/sensord/sensors/bmx055_gyro.h"
#include "system/sensord/sensors/bmx055_magn.h"
#include "system/sensord/sensors/bmx055_temp.h"
#include "system/sensord/sensors/constants.h"
#include "system/sensord/sensors/lsm6ds3_accel.h"
#include "system/sensord/sensors/lsm6ds3_gyro.h"
#include "system/sensord/sensors/lsm6ds3_temp.h"
#include "system/sensord/sensors/mmc5603nj_magn.h"

// 添加串口传感器相关头文件
#include "system/sensord/sensors/serial.h"
#include "system/sensord/sensors/wit_c_sdk.h"
#include "system/sensord/sensors/REG.h"

#include <stdint.h>

#define I2C_BUS_IMU 1

// WIT Motion 传感器相关定义
#define ACC_UPDATE      0x01
#define GYRO_UPDATE     0x02
#define ANGLE_UPDATE    0x04
#define MAG_UPDATE      0x08
#define READ_UPDATE     0x80

// 全局变量
static int serial_fd = -1;
static int s_iCurBaud = 9600;
static volatile char s_cDataUpdate = 0;
const int c_uiBaud[] = {2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600};

ExitHandler do_exit;

// 函数声明
static void AutoScanSensor(const char* dev);
static void SensorDataUpdata(uint32_t uiReg, uint32_t uiRegNum);
static void Delayms(uint16_t ucMs);

// 创建陀螺仪事件消息
bool get_gyro_event(MessageBuilder &msg, uint64_t ts, float x, float y, float z) {
  if (ts ==0) {
    ts = nanos_since_boot();
  }
  auto event = msg.initEvent().initGyroscope();
  event.setSource(cereal::SensorEventData::SensorSource::LSM6DS3TRC);
  event.setVersion(2);
  event.setSensor(SENSOR_GYRO_UNCALIBRATED);
  event.setType(SENSOR_TYPE_GYROSCOPE_UNCALIBRATED);
  event.setTimestamp(ts);

  float xyz[] = {y, -x, z};
  auto svec = event.initGyroUncalibrated();
  svec.setV(xyz);
  svec.setStatus(true);

  return true;
}

// 创建加速度计事件消息
bool get_accel_event(MessageBuilder &msg, uint64_t ts, float x, float y, float z) {
  if (ts == 0) {
    ts = nanos_since_boot();
  }
  auto event = msg.initEvent().initAccelerometer();
  event.setSource(cereal::SensorEventData::SensorSource::LSM6DS3TRC);
  event.setVersion(1);
  event.setSensor(SENSOR_ACCELEROMETER);
  event.setType(SENSOR_TYPE_ACCELEROMETER);
  event.setTimestamp(ts);

  float xyz[] = {y, -x, z};
  auto svec = event.initAcceleration();
  svec.setV(xyz);
  svec.setStatus(true);

  return true;
}

// 初始化串口传感器
int open_sensor(const char *com) {
  if ((serial_fd = serial_open(com, 9600)) < 0) {
    LOGE("Failed to open %s", com);
    return -1;
  } else {
    LOGD("Successfully opened %s", com);
  }

  WitInit(WIT_PROTOCOL_NORMAL, 0x50);
  WitRegisterCallBack(SensorDataUpdata);
  AutoScanSensor(com);

  return 0;
}

// 传感器数据更新回调
static void SensorDataUpdata(uint32_t uiReg, uint32_t uiRegNum) {
  int i;
  for (i = 0; i < uiRegNum; i++) {
    switch (uiReg) {
      case AZ:
        s_cDataUpdate |= ACC_UPDATE;
        break;
      case GZ:
        s_cDataUpdate |= GYRO_UPDATE;
        break;
      case HZ:
        s_cDataUpdate |= MAG_UPDATE;
        break;
      case Yaw:
        s_cDataUpdate |= ANGLE_UPDATE;
        break;
      default:
        s_cDataUpdate |= READ_UPDATE;
        break;
    }
    uiReg++;
  }
}

// 延时函数
static void Delayms(uint16_t ucMs) {
  usleep(ucMs * 1000);
}

// 自动扫描传感器波特率
static void AutoScanSensor(const char* dev) {
  int i, iRetry;
  char cBuff[1];

  for (i = 1; i < 10; i++) {
    serial_close(serial_fd);
    s_iCurBaud = c_uiBaud[i];
    serial_fd = serial_open(dev, c_uiBaud[i]);
	if (serial_fd < 0) {
	  continue; // 跳过这个波特率，尝试下一个
	}

    iRetry = 2;
    do {
      s_cDataUpdate = 0;
      WitReadReg(AX, 3);
      Delayms(1);
      while (serial_read_data(serial_fd, (unsigned char*)cBuff, 1)) {
        WitSerialDataIn(cBuff[0]);
      }
      if (s_cDataUpdate != 0) {
        LOGD("Found sensor at %d baud", c_uiBaud[i]);
        return;
      }
      iRetry--;
    } while (iRetry);
  }
  LOGE("Cannot find sensor, please check your connection");
}

// PC环境下的串口传感器循环
void serial_sensor_loop() {
  PubMaster pm({"gyroscope", "accelerometer"});
  RateKeeper rk("gyroscope", services.at("gyroscope").frequency);

  float fAcc[3], fGyro[3], fAngle[3];
  int i;
  char cBuff[1];

  LOGD("Starting serial sensor loop");

  while (!do_exit) {
    // 读取串口数据
    while (serial_read_data(serial_fd, (unsigned char*)cBuff, 1)) {
      WitSerialDataIn(cBuff[0]);
    }

    if (s_cDataUpdate) {
      uint64_t ts = nanos_since_boot();

      // 转换传感器数据
      for (i = 0; i < 3; i++) {
        fAcc[i] = sReg[AX + i] / 32768.0f * 16.0f;
        fGyro[i] = sReg[GX + i] / 32768.0f * 2000.0f;
        fAngle[i] = sReg[Roll + i] / 32768.0f * 180.0f;
      }

      // 发送加速度计数据
      if (s_cDataUpdate & ACC_UPDATE) {
        MessageBuilder msg;
        if (get_accel_event(msg, ts, fAcc[0], fAcc[1], fAcc[2])) {
          pm.send("accelerometer", msg);
		  LOGD("Sent accel: %.3f %.3f %.3f", fAcc[0], fAcc[1], fAcc[2]); // 调试输出
        }
        s_cDataUpdate &= ~ACC_UPDATE;
      }

      // 发送陀螺仪数据
      if (s_cDataUpdate & GYRO_UPDATE) {
        MessageBuilder msg;
        if (get_gyro_event(msg, ts, fGyro[0], fGyro[1], fGyro[2])) {
          pm.send("gyroscope", msg);
          LOGD("Sent gyro: %.3f %.3f %.3f", fGyro[0], fGyro[1], fGyro[2]); // 调试输出
        }
        s_cDataUpdate &= ~GYRO_UPDATE;
      }

      // 处理其他数据类型
      if (s_cDataUpdate & ANGLE_UPDATE) {
        s_cDataUpdate &= ~ANGLE_UPDATE;
      }

      if (s_cDataUpdate & MAG_UPDATE) {
        s_cDataUpdate &= ~MAG_UPDATE;
      }
    }

    rk.keepTime();
  }
}
// 原有的中断循环（用于I2C传感器）
void interrupt_loop(std::vector<std::tuple<Sensor *, std::string>> sensors) {
  PubMaster pm({"gyroscope", "accelerometer"});

  int fd = -1;
  for (auto &[sensor, msg_name] : sensors) {
    if (sensor->has_interrupt_enabled()) {
      fd = sensor->gpio_fd;
      break;
    }
  }

  uint64_t offset = nanos_since_epoch() - nanos_since_boot();
  struct pollfd fd_list[1] = {0};
  fd_list[0].fd = fd;
  fd_list[0].events = POLLIN | POLLPRI;

  while (!do_exit) {
    int err = poll(fd_list, 1, 100);
    if (err == -1) {
      if (errno == EINTR) {
        continue;
      }
      return;
    } else if (err == 0) {
      LOGE("poll timed out");
      continue;
    }

    if ((fd_list[0].revents & (POLLIN | POLLPRI)) == 0) {
      LOGE("no poll events set");
      continue;
    }

    // Read all events
    struct gpioevent_data evdata[16];
    err = HANDLE_EINTR(read(fd, evdata, sizeof(evdata)));
    if (err < 0 || err % sizeof(*evdata) != 0) {
      LOGE("error reading event data %d", err);
      continue;
    }

    uint64_t cur_offset = nanos_since_epoch() - nanos_since_boot();
    uint64_t diff = cur_offset > offset ? cur_offset - offset : offset - cur_offset;
    if (diff > 10*1e6) { // 10ms
      LOGW("time jumped: %lu %lu", cur_offset, offset);
      offset = cur_offset;
      continue;
    }

    int num_events = err / sizeof(*evdata);
    uint64_t ts = evdata[num_events - 1].timestamp - cur_offset;

    for (auto &[sensor, msg_name] : sensors) {
      if (!sensor->has_interrupt_enabled()) {
        continue;
      }

      MessageBuilder msg;
      if (!sensor->get_event(msg, ts)) {
        continue;
      }

      if (!sensor->is_data_valid(ts)) {
        continue;
      }

      pm.send(msg_name.c_str(), msg);
    }
  }
}

// 轮询循环（用于无中断的I2C传感器）
void polling_loop(Sensor *sensor, std::string msg_name) {
  PubMaster pm({msg_name.c_str()});
  RateKeeper rk(msg_name, services.at(msg_name).frequency);
  while (!do_exit) {
    MessageBuilder msg;
    if (sensor->get_event(msg) && sensor->is_data_valid(nanos_since_boot())) {
      pm.send(msg_name.c_str(), msg);
    }
    rk.keepTime();
  }
}

// I2C传感器循环
int sensor_loop(I2CBus *i2c_bus_imu) {
  // Sensor init
  std::vector<std::tuple<Sensor *, std::string>> sensors_init = {
    {new BMX055_Accel(i2c_bus_imu), "accelerometer2"},
    {new BMX055_Gyro(i2c_bus_imu), "gyroscope2"},
    {new BMX055_Magn(i2c_bus_imu), "magnetometer"},
    {new BMX055_Temp(i2c_bus_imu), "temperatureSensor2"},

    {new LSM6DS3_Accel(i2c_bus_imu, GPIO_LSM_INT), "accelerometer"},
    {new LSM6DS3_Gyro(i2c_bus_imu, GPIO_LSM_INT, true), "gyroscope"},
    {new LSM6DS3_Temp(i2c_bus_imu), "temperatureSensor"},

    {new MMC5603NJ_Magn(i2c_bus_imu), "magnetometer"},
  };

  // Initialize sensors
  std::vector<std::thread> threads;
  for (auto &[sensor, msg_name] : sensors_init) {
    int err = sensor->init();
    if (err < 0) {
      continue;
    }

    if (!sensor->has_interrupt_enabled()) {
      threads.emplace_back(polling_loop, sensor, msg_name);
    }
  }

  // increase interrupt quality by pinning interrupt and process to core 1
  setpriority(PRIO_PROCESS, 0, -18);
  util::set_core_affinity({1});

  // TODO: get the IRQ number from gpiochip
  std::string irq_path = "/proc/irq/336/smp_affinity_list";
  if (!util::file_exists(irq_path)) {
    irq_path = "/proc/irq/335/smp_affinity_list";
  }
  std::system(util::string_format("sudo su -c 'echo 1 > %s'", irq_path.c_str()).c_str());

  // thread for reading events via interrupts
  threads.emplace_back(&interrupt_loop, std::ref(sensors_init));

  // wait for all threads to finish
  for (auto &t : threads) {
    t.join();
  }

  for (auto &[sensor, msg_name] : sensors_init) {
    sensor->shutdown();
    delete sensor;
  }
  return 0;
}
// 主函数
int main(int argc, char *argv[]) {
  try {
    // 检测是否为PC环境
    if (Hardware::PC()) {
      LOGD("Running in PC environment, using serial sensors");

      // 初始化串口传感器
      const char* device_path = "/dev/ttyUSB0"; // 默认值
      if (argc > 1) {
        device_path = argv[1]; // 从命令行参数获取
      } else if (const char* env_device = getenv("SERIAL_DEVICE")) {
        device_path = env_device; // 从环境变量获取
      }

if (open_sensor(device_path) < 0) {
  LOGE("Failed to open serial sensor at %s", device_path);
  return -1;
}

      // 在PC环境下直接运行串口传感器循环
      serial_sensor_loop();

      // 清理资源
      if (serial_fd >= 0) {
        serial_close(serial_fd);
      }

      return 0;
    } else {
      LOGD("Running in embedded environment, using I2C sensors");

      // 原有的I2C传感器逻辑
      auto i2c_bus_imu = std::make_unique<I2CBus>(I2C_BUS_IMU);
      return sensor_loop(i2c_bus_imu.get());
    }
  } catch (std::exception &e) {
    LOGE("Sensor initialization failed: %s", e.what());

    // 清理串口资源
    if (serial_fd >= 0) {
      serial_close(serial_fd);
    }

    return -1;
  }
}