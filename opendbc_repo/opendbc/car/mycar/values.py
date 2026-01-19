
from dataclasses import dataclass, field
from enum import IntFlag
from opendbc.car import Bus, DbcDict, PlatformConfig, Platforms, CarSpecs
from opendbc.car.docs_definitions import CarHarness, CarDocs, CarParts
from opendbc.car.fw_query_definitions import FwQueryConfig, Request, StdQueries
# 车辆静态参数配置
class CarControllerParams:
  DEBUG=False
  YAW=20
  STATE = 5
  END = 5
  STEER_MAX = 150
  STEER_DELTA_UP = 2
  STEER_DELTA_DOWN = 6

  SPEED_MAX_LIMIT =[2.5 , 7.5,16.7,27.7]  #(9-27-60-100)
  STEER_MAX_LIMIT =  [28 , 42 , 58 , 48]


  #允许的转向角度偏移范围不超过预设值
  STEER_DRIVER_ALLOWANCE = 65
  # 车辆线控转向（Steer-by-Wire）系统中，该参数用于调节 ‌驾驶员输入信号的增益系数‌
  STEER_DRIVER_MULTIPLIER = 1.6
  #驾驶员转向输入的动态调节系
  STEER_DRIVER_FACTOR = 1.1
  #车辆实际航向与规划路径之间的最大允许横向偏差
  STEER_ERROR_MAX = 55
  #仿真帧率（如 FPS=50）绑定，通过 1.0/FPS 实现毫秒级步进控制
  STEER_STEP = 2  #100/2=50hz
  #转向软启动步长控制‌
  STEER_SOFTSTART_STEP = 1 # 20ms(50Hz) * 150 / 1 = 2000ms. This means the clip ceiling will be increased to 150 in 1500ms

  ACC_STEP = 2    #50hz
  ACCEL_MAX = 1.9
  ACCEL_MIN = -4.2
  K_DASHSPEED = 0.0719088 #convert pulse to kph
  USE_STEERING_SPEED_LIMITER = True

  # op long control
  K_accel_jerk_upper = 0.07
  K_accel_jerk_lower = 0.6
  K_jerk_xp =            [   9,   8,   15,   30,   60]  # meters
  K_jerk_base_lower_fp = [-2.6, -2.0, -1.5, -0.9, -0.3]
  K_jerk_base_upper_fp = [ 0.5,  0.4,  0.3,  0.2,  0.1]
  DCT_SHIFT_COMPENSATION = 0.9
  SPORT_MODE_GAIN = 1.15


#FD to be added later
class GacSafetyFlags(IntFlag):
  MY_CAR = 0x1

@dataclass
class GacCarDocs(CarDocs):
  package: str = "All"
  car_parts: CarParts = field(default_factory=CarParts.common([CarHarness.custom]))
  #todo add docs and harness info

@dataclass
class GacPlatformConfig(PlatformConfig):
  dbc_dict: DbcDict = field(default_factory=lambda: {Bus.pt: "my_car"})
  #todo add dbc for other models

class CAR(Platforms):
  MY_CAR = GacPlatformConfig(
    [GacCarDocs("MY CAR")],
    CarSpecs(mass=1380.0, wheelbase=2.700, steerRatio=14.2, centerToFrontRatio=0.48, tireStiffnessFactor=1.05),
  )

#汽车CAN总线通信的基础类
class CanBus:
  ESC = 0 # ESC总线  电子稳定控制系统总线（Electronic Stability Control）
  MRR = 1 # 雷达总线  毫米波雷达总线（Medium Range Radar）
  MPC = 2 # MPC总线  模型预测控制器总线（gyroscope Predictive Controller）
#CAN总线固件查询配置
FW_QUERY_CONFIG = FwQueryConfig(
  requests=[
    Request(
      [StdQueries.MANUFACTURER_SOFTWARE_VERSION_REQUEST], #查询制造商软件版本的标准请求指令
      [StdQueries.MANUFACTURER_SOFTWARE_VERSION_RESPONSE],#接收ECU返回的软件版本信息
      bus=CanBus.ESC,
    ),
  ],
)

DBC = CAR.create_dbc_map()

if __name__ == "__main__":
  cars = []
  for platform in CAR:
    for doc in platform.config.car_docs:
      cars.append(doc.name)
  cars.sort()
  for c in cars:
    print(c)
