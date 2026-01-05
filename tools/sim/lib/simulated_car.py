import os
import traceback
import sys

# 添加opendbc_repo到Python路径
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..', 'opendbc_repo'))
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..'))

try:
    import cereal.messaging as messaging
    from opendbc.can.packer import CANPacker
    from opendbc.can.parser import CANParser
    from opendbc.car.byd.values import BydSafetyFlags, CanBus
    from opendbc.car import Bus
    from openpilot.common.params import Params
    from openpilot.selfdrive.pandad.pandad_api_impl import can_list_to_can_capnp
    from openpilot.tools.sim.lib.common import SimulatorState
    from cereal import log
except ImportError as e:
    print(f"Import error: {e}")
    # 使用模拟类避免导入错误
    class MockClass:
        def __init__(self, *args, **kwargs):
            pass

        def __getattr__(self, name):
            # 对于PubMaster和SubMaster的调用，返回一个模拟对象
            if name in ['PubMaster', 'SubMaster']:
                return lambda *args, **kwargs: MockClass(*args, **kwargs)
            # 对于其他属性访问，返回一个默认值或模拟对象
            return MockClass()

        def __getitem__(self, key):
            # 支持字典式访问
            return MockClass()

        def __call__(self, *args, **kwargs):
            # 支持调用
            return MockClass()

        def __setattr__(self, name, value):
            # 允许设置属性
            self.__dict__[name] = value

    class MockMessage:
        def __init__(self, *args, **kwargs):
            pass

        def to_bytes(self):
            # 返回空字节串以避免错误
            return b''

        def __getattr__(self, name):
            # 对于其他属性访问，返回一个默认值或模拟对象
            return MockClass()

    messaging = MockClass()
    CANPacker = lambda *args, **kwargs: MockClass(*args, **kwargs)
    CANParser = lambda *args, **kwargs: MockClass(*args, **kwargs)

    # 创建模拟的BydSafetyFlags类
    class MockBydSafetyFlags:
        class HAN_TANG_DMEV:
            value = 1

    BydSafetyFlags = MockBydSafetyFlags()

    # 创建模拟的CanBus类
    class MockCanBus:
        ESC = 0
        MPC = 2

    CanBus = MockCanBus()

    # 创建模拟的Bus类
    class MockBus:
        pt = 0
        cam = 2

    Bus = MockBus()
    Params = lambda *args, **kwargs: MockClass(*args, **kwargs)
    can_list_to_can_capnp = lambda x: x

    # 创建模拟的SimulatorState类
    class MockSimulatorState:
        pass

    SimulatorState = MockSimulatorState

    # 创建模拟的log模块
    class MockLog:
        class PandaState:
            class PandaType:
                blackPanda = "blackPanda"
                uno = "uno"

            class PandaCanState:
                class LecErrorCode:
                    noError = 0

        class CarParams:
            class SafetyModel:
                byd = "byd"

    log = MockLog()


class SimulatedCar:
  """Simulates a BYD HAN EV 20 (panda state + can messages) to OpenPilot"""
  try:
      packer = CANPacker("byd_han_dmev_2020")
  except Exception as e:
      print(f"DEBUG: Error initializing CANPacker: {e}")
      packer = None

  def __init__(self, car_brand):
    # 添加调试打印函数作为实例方法
    self.debug_print = debug_print

    try:
        self.pm = messaging.PubMaster(['can', 'pandaStates', 'selfdriveState', 'carParams'])
    except Exception as e:
        print(f"DEBUG: Error initializing PubMaster: {e}")
        self.pm = None
    try:
        self.sm = messaging.SubMaster(['carControl', 'controlsState', 'carParams', 'selfdriveState'])
    except Exception as e:
        print(f"DEBUG: Error initializing SubMaster: {e}")
        self.sm = None

    # 初始化CarParams以确保系统能正确识别车辆
    try:
        if self.pm:
            # 创建并发送CarParams消息
            from cereal import car
            CP = car.CarParams.new_message()
            CP.carFingerprint = "BYD HAN EV 20"
            CP.fingerprintSource = car.CarParams.FingerprintSource.can
            CP.carFw = []
            CP.safetyConfigs = [car.CarParams.SafetyConfig.new_message()]
            CP.safetyConfigs[0].safetyModel = car.CarParams.SafetyModel.byd
            CP.safetyConfigs[0].safetyParam = 1
            CP.brand = "byd"
            CP.enableDsu = False
            CP.enableBsm = False
            # CP.enableAeb = False  # 注释掉不存在的属性
            CP.alternativeExperience = 0
            CP.lateralTuning.pid.kpBP = [0.0]
            CP.lateralTuning.pid.kiBP = [0.0]
            CP.lateralTuning.pid.kpV = [0.5]
            CP.lateralTuning.pid.kiV = [0.1]
            CP.steerRatio = 15.0
            CP.steerRatioRear = 0.0
            CP.steerActuatorDelay = 0.1
            CP.steerLimitTimer = 0.4
            # CP.steerRateCost = 0.5  # 注释掉不存在的属性
            CP.steerControlType = car.CarParams.SteerControlType.angle
            CP.minSteerSpeed = 0.0
            # CP.maxSteerAngle = 300.0  # 注释掉不存在的属性
            CP.maxLateralAccel = 2.0
            CP.mass = 2200.0
            CP.wheelbase = 2.92
            CP.centerToFront = CP.wheelbase * 0.5
            CP.steerRatio = 15.0
            CP.rotationalInertia = 2500.0
            CP.tireStiffnessFront = 100000.0
            CP.tireStiffnessRear = 100000.0
            CP.longitudinalTuning.kpBP = [0.0]
            CP.longitudinalTuning.kpV = [0.5]
            CP.longitudinalTuning.kiBP = [0.0]
            CP.longitudinalTuning.kiV = [0.1]
            CP.longitudinalActuatorDelay = 0.2
            CP.openpilotLongitudinalControl = True
            # CP.enableGasInterceptor = False  # 注释掉不存在的属性
            CP.startAccel = 0.0
            CP.stoppingDecelRate = 0.8
            CP.vEgoStopping = 0.5
            CP.vEgoStarting = 0.5
            # CP.stoppingControl = True  # 注释掉不存在的属性
            # CP.longitudinalActuatorDelayLowerBound = 0.2  # 注释掉不存在的属性
            # CP.longitudinalActuatorDelayUpperBound = 0.2  # 注释掉不存在的属性
            # CP.gasMaxBP = [0.0]  # 注释掉不存在的属性
            # CP.gasMaxV = [0.5]  # 注释掉不存在的属性
            # CP.brakeMaxBP = [0.0]  # 注释掉不存在的属性
            # CP.brakeMaxV = [1.0]  # 注释掉不存在的属性
            CP.radarTimeStep = 0.05
            CP.radarUnavailable = False
            # CP.enableCamera = True  # 注释掉不存在的属性
            # CP.hasStockCamera = False  # 注释掉不存在的属性
            # CP.hasStockRadar = False  # 注释掉不存在的属性
            # CP.hasAdaptiveCruise = True  # 注释掉不存在的属性
            # CP.hasLaneDepartureWarning = True  # 注释掉不存在的属性
            CP.networkLocation = car.CarParams.NetworkLocation.fwdCamera

            # 发送CarParams消息
            pm = messaging.PubMaster(['carParams'])
            cp_msg = messaging.new_message('carParams')
            cp_msg.valid = True
            cp_msg.carParams = CP
            pm.send('carParams', cp_msg)
            print("DEBUG: Sent CarParams message")
    except Exception as e:
        print(f"DEBUG: Error sending CarParams: {e}")
        import traceback
        traceback.print_exc()

    self.debug_print(f"DEBUG: car_brand parameter received: {car_brand}")

    try:
        self.cp = self.get_car_can_parser(car_brand)
    except Exception as e:
        print(f"DEBUG: Error initializing CAN parser: {e}")
        self.cp = None
    self.idx = 0
    self.params = Params()
    self.obd_multiplexing = False
    # 调试信息
    self.debug_counter = 0

    # 添加调试信息，检查解析器初始化
    try:
        if hasattr(self, 'cp') and self.cp:
            self.debug_print(f"DEBUG: CAN parser pt addresses: {list(self.cp[Bus.pt].addresses)}")
            self.debug_print(f"DEBUG: CAN parser cam addresses: {list(self.cp[Bus.cam].addresses)}")
            # 检查消息状态
            self.debug_print(f"DEBUG: CAN parser pt message_states: {list(self.cp[Bus.pt].message_states.keys())}")
            self.debug_print(f"DEBUG: CAN parser cam message_states: {list(self.cp[Bus.cam].message_states.keys())}")
            # 检查DBC文件中的消息定义
            self.debug_print(f"DEBUG: CAN parser pt dbc name: {self.cp[Bus.pt].dbc_name}")
            self.debug_print(f"DEBUG: CAN parser cam dbc name: {self.cp[Bus.cam].dbc_name}")
            # 检查DBC中的消息定义
            if hasattr(self.cp[Bus.pt], 'dbc') and self.cp[Bus.pt].dbc:
                self.debug_print(f"DEBUG: CAN parser pt dbc msgs: {list(self.cp[Bus.pt].dbc.msgs.keys())}")
    except Exception as e:
        self.debug_print(f"DEBUG: Error printing parser addresses: {e}")

  @staticmethod
  def get_car_can_parser(car_brand):
    debug_print(f"DEBUG: get_car_can_parser called with car_brand: {car_brand}")
    # 根据car_brand选择正确的DBC文件
    if car_brand and car_brand.lower() == "byd":
      # 按照BYD车型CarState的实现初始化CANParser
      from opendbc.car.byd.values import CAR, DBC
      from opendbc.car import Bus
      from opendbc.car.byd.values import CanBus

      # 使用与CarState相同的DBC映射方式
      car_fingerprint = "BYD_HAN_EV_20"
      dbc_pt = DBC[car_fingerprint][Bus.pt]
      debug_print(f"DEBUG: Using DBC file: {dbc_pt}")

      # 使用消息名称而不是地址，确保包含所有必需的消息
      pt_messages = [
            ("EPS", 100),         # 0x11F
            ("CARSPEED", 50),     # 0x121
            ("PEDAL", 50),        # 0x342
            ("EPB", 1),           # 0x55
            ("ACC_EPS_STATE", 50), # 0x318
            ("DRIVE_STATE", 50),  # 0x242
            ("STALKS", 1),        # 0x133
            ("BCM", 20),          # 0x131 (修改为20Hz，与BYD车型CarState配置匹配)
            ("PCM_BUTTONS", 20),  # 0x3B0
            ("DATETIME", 2),      # 0x2B6
            ("YAW_RATE", 50),     # 0x222
            ("BELT", 20),         # 0x294
            ("AXAY", 50),         # 0x223
            ("BSD_RADAR", 20),    # 0x418 (添加BSD雷达消息)
        ]
      # 创建一个字典，模拟CarState中的can_parsers
      parsers = {}
      parsers[Bus.pt] = CANParser(dbc_pt, pt_messages, CanBus.ESC)  # type: ignore
      # 添加摄像头总线的解析器
      cam_messages = [
            ("ACC_HUD_ADAS", 50),    # 0x32D
            ("ACC_CMD", 50),         # 0x32E
            ("ACC_MPC_STATE", 50),   # 0x316
            ("RADAR_MRR", 60),       # 0x374 (确保雷达消息被解析)
        ]
      # 注意：这里应该使用与CarState相同的DBC文件和总线配置
      parsers[Bus.cam] = CANParser(dbc_pt, cam_messages, CanBus.MPC)  # type: ignore  # 使用总线2
      return parsers
    else:
      # 默认使用本田的配置
      dbc_f = 'honda_civic_ex_2022_can_generated'
      checks = []
      from opendbc.car import Bus
      parsers = {}
      parsers[Bus.pt] = CANParser(dbc_f, checks, 0)
      parsers[Bus.cam] = CANParser(dbc_f, checks, 2)
      return parsers

  def calculate_checksum(self, data):
    """计算简单的checksum"""
    checksum = 0
    for byte in data:
      checksum = (checksum + byte) & 0xFF
    return checksum

  def send_can_messages(self, simulator_state):  # type: ignore
    if not simulator_state.valid:
      return

    msg = []

    # *** ESC bus (Bus 0) ***
    speed = simulator_state.speed * 3.6  # convert m/s to kph
    speed_kph = max(1.0, min(255, speed))  # 限制在1-255范围内

    # EPS (0x11F): 方向盘角度和扭矩 - 每次都发送 (100Hz)
    # 根据DBC文件，EPS消息长度为5字节
    # SteeringAngle: 0|16@1- (0.1,0) [-450|450]
    # SteeringAngleRate: 16|8@1+ (4,0) [0|1020]
    steering_angle = int(simulator_state.steering_angle * 10) & 0xFFFF  # 转换为0.1度单位
    steering_rate = int(abs(simulator_state.steering_angle) * 0.25) & 0xFF  # 使用steering_angle代替steering_rate
    counter = self.idx % 256  # 计数器
    # 确保至少有一个非零值，避免全0数据
    if steering_angle == 0 and steering_rate == 0:
        steering_angle = 1  # 设置一个最小值以避免全0数据
    eps_data = [
        steering_angle & 0xFF,           # 字节0: SteeringAngle低字节
        (steering_angle >> 8) & 0xFF,    # 字节1: SteeringAngle高字节
        steering_rate,                   # 字节2: SteeringAngleRate
        counter & 0xFF,                  # 字节3: Counter
        0x00                             # 字节4: 保留
    ]
    eps_msg = (0x11F, eps_data, CanBus.ESC)
    msg.append(eps_msg)

    # CARSPEED (0x121): 车速信息 - 每次都发送 (50Hz)
    # 根据DBC文件，CARSPEED消息长度为8字节
    # CarDisplaySpeed: 0|12@1+ (1,0) [0|255]
    speed_val = int(speed_kph) & 0xFFF  # 限制在12位
    # 确保至少有一个非零值，避免全0数据
    if speed_val == 0:
        speed_val = 1  # 设置一个最小值以避免全0数据
    carspeed_data = [
        speed_val & 0xFF,                # 字节0: CarDisplaySpeed低字节
        (speed_val >> 8) & 0x0F,         # 字节1: CarDisplaySpeed高字节的低4位
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00  # 其他字节填充0
    ]
    carspeed_msg = (0x121, carspeed_data, CanBus.ESC)
    msg.append(carspeed_msg)

    # PEDAL (0x342): 油门和刹车状态 - 每次都发送 (50Hz)
    # 根据DBC文件，PEDAL消息长度为8字节
    # AcceleratorPedal: 0|8@1+ (0.01,0) [0|255]
    # BrakePedal: 8|8@1+ (0.01,0) [0|255]
    user_gas = max(0, min(255, int(simulator_state.user_gas * 100)))  # 转换为0.01单位
    user_brake = max(0, min(255, int(simulator_state.user_brake * 100)))  # 转换为0.01单位
    # 确保至少有一个非零值，避免全0数据
    if user_gas == 0 and user_brake == 0:
        user_gas = 1  # 设置一个最小值以避免全0数据
    pedal_data = [
        user_gas,                        # 字节0: AcceleratorPedal
        user_brake,                      # 字节1: BrakePedal
        0x00, 0x00, 0x00,                # 字节2-4: 其他字段
        0x00, 0x00, 0x00                 # 字节5-7: 保留
    ]
    pedal_msg = (0x342, pedal_data, CanBus.ESC)
    msg.append(pedal_msg)

    # ACC_EPS_STATE (0x318): 巡航状态和方向盘角度测量 - 每次都发送 (50Hz)
    # 根据DBC文件，ACC_EPS_STATE消息长度为8字节
    acc_eps_data = [
        0x01,                            # 字节0: LKAS_Prepared (Ready)
        0x00,                            # 字节1: CruiseActivated
        0x00,                            # 字节2: TorqueFailed
        0x01,                            # 字节3: SETME1_0x1
        0x00,                            # 字节4: SteerWarning
        0x00,                            # 字节5: SteerErrorCode
        0x00, 0x00                       # 字节6-7: MainTorque低字节和高字节
    ]
    acc_eps_state_msg = (0x318, acc_eps_data, CanBus.ESC)
    msg.append(acc_eps_state_msg)

    # DRIVE_STATE (0x242): 档位状态 - 每次都发送 (50Hz)
    # 根据DBC文件，DRIVE_STATE消息长度为8字节
    # Gear: 40|3@1+ (1,0) [0|7]
    gear_state = 2  # D档
    drive_state_data = [
        0x00, 0x00, 0x00, 0x00, 0x00,    # 字节0-4: 其他字段
        (gear_state << 4) & 0xFF,        # 字节5: Gear在高4位
        0x00, 0x00                       # 字节6-7: Counter和CheckSum
    ]
    drive_state_msg = (0x242, drive_state_data, CanBus.ESC)
    msg.append(drive_state_msg)

    # EPB (0x55): 电子驻车 - 每20次循环发送一次 (5Hz)
    if self.idx % 20 == 0:
      # 模拟EPB状态
      # EPB_ActiveFlag: 40|1@1+ (1,0) [0|1]
      # CheckSum: 56|8@1+ (1,0) [0|255]
      epb_data = [
        0x00, 0x00, 0x00, 0x00, 0x00,  # 前5个字节
        0x00,  # EPB_ActiveFlag (bit 40 = bit 0 of byte 5)
        0x00, 0x00  # 最后2个字节
      ]
      epb_msg = (0x55, epb_data, CanBus.ESC)
      msg.append(epb_msg)
      self.debug_print(f"DEBUG: Sending EPB message: addr=0x55, data={epb_data}")

    # STALKS (0x133): 转向灯和大灯 - 每10次循环发送一次 (10Hz)
    if self.idx % 10 == 0:
      # 模拟转向灯和大灯状态
      # FrontFogLight: 0|1@0+ (1,0) [0|1]
      # LightsOn: 1|1@1+ (1,0) [0|1]
      # HeadLight: 2|1@1+ (1,0) [0|1]
      # FullBeamOn: 3|1@1+ (1,0) [0|1]
      # LeftIndicator: 4|1@0+ (1,0) [0|1]
      # RightIndicator: 5|1@0+ (1,0) [0|1]
      # RearFogLight: 7|1@0+ (1,0) [0|1]
      stalks_data = [
        0x00,  # FrontFogLight, LightsOn, HeadLight, FullBeamOn, LeftIndicator, RightIndicator, (bit 6 unused), RearFogLight
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  # 其他字段
      ]
      stalks_msg = (0x133, stalks_data, CanBus.ESC)
      msg.append(stalks_msg)
      self.debug_print(f"DEBUG: Sending STALKS message: addr=0x133, data={stalks_data}")

    # BCM (0x12D): 车门状态 - 每5次循环发送一次 (20Hz)
    if self.idx % 5 == 0:
      # 模拟所有车门关闭状态
      # 根据DBC文件，BCM消息包含车门状态等信息
      # FrontLeftDoor: 0|1@0+ (1,0) [0|1]
      # FrontRightDoor: 1|1@0+ (1,0) [0|1]
      # RearLeftDoor: 2|1@0+ (1,0) [0|1]
      # RearRightDoor: 3|1@0+ (1,0) [0|1]
      # BootDoor: 4|1@0+ (1,0) [0|1]
      # DriverSeatBeltFasten: 6|1@0+ (1,0) [0|1]
      # BrakeLight: 8|1@0+ (1,0) [0|1]
      # FrontRightPassengerSeatBelt: 55|1@0+ (1,0) [0|1]
      bcm_data = [
        0x00,  # FrontLeftDoor, FrontRightDoor, RearLeftDoor, RearRightDoor, BootDoor
        0x40,  # DriverSeatBeltFasten (bit 6)
        0x00,  # BrakeLight
        0x00, 0x00, 0x00, 0x00, 0x00  # 其他字段
      ]
      bcm_msg = (0x12D, bcm_data, CanBus.ESC)
      msg.append(bcm_msg)
      # 总是打印BCM调试信息以确保消息被发送
      self.debug_print(f"DEBUG: Sending BCM message: addr=0x12D, bus={CanBus.ESC}, data={bcm_data}")

      # 添加更详细的验证，确保BCM消息被正确解析
      try:
          if hasattr(self, 'cp') and self.cp is not None:
              # 更新CAN解析器以包含BCM消息
              timestamp = self.idx * 1000000  # 模拟时间戳 (nanoseconds)
              bcm_frame = (0x12D, bytes(bcm_data), CanBus.ESC)
              bcm_entry = [timestamp, [bcm_frame]]
              try:
                  self.cp[Bus.pt].update([bcm_entry])
                  if 301 in self.cp[Bus.pt].vl:
                      bcm_parsed = self.cp[Bus.pt].vl[301]
                      self.debug_print(f"DEBUG: BCM message parsed successfully: {bcm_parsed}")
                      # 检查BCM消息中的关键字段
                      if 'DriverSeatBeltFasten' in bcm_parsed:
                          self.debug_print(f"DEBUG: BCM DriverSeatBeltFasten: {bcm_parsed['DriverSeatBeltFasten']}")
                      if 'FrontLeftDoor' in bcm_parsed:
                          self.debug_print(f"DEBUG: BCM FrontLeftDoor: {bcm_parsed['FrontLeftDoor']}")
                      if 'FrontRightDoor' in bcm_parsed:
                          self.debug_print(f"DEBUG: BCM FrontRightDoor: {bcm_parsed['FrontRightDoor']}")
                      if 'RearLeftDoor' in bcm_parsed:
                          self.debug_print(f"DEBUG: BCM RearLeftDoor: {bcm_parsed['RearLeftDoor']}")
                      if 'RearRightDoor' in bcm_parsed:
                          self.debug_print(f"DEBUG: BCM RearRightDoor: {bcm_parsed['RearRightDoor']}")
                      if 'BootDoor' in bcm_parsed:
                          self.debug_print(f"DEBUG: BCM BootDoor: {bcm_parsed['BootDoor']}")
                  else:
                      self.debug_print(f"DEBUG: BCM message not found in parser after update")
              except Exception as e:
                  self.debug_print(f"DEBUG: Error updating BCM parser: {e}")
                  import traceback
                  traceback.print_exc()
      except Exception as e:
          self.debug_print(f"DEBUG: Error checking BCM parsing: {e}")
          import traceback
          traceback.print_exc()

      # 添加额外的调试信息，检查CAN解析器状态
      if hasattr(self, 'cp') and self.cp is not None:
          pt_parser = self.cp[Bus.pt]
          if hasattr(pt_parser, 'message_states') and 301 in pt_parser.message_states:
              bcm_state = pt_parser.message_states[301]
              self.debug_print(f"DEBUG: BCM message state - name: {bcm_state.name}, address: {bcm_state.address}, frequency: {getattr(bcm_state, 'frequency', 'Not set')}, timeout_threshold: {getattr(bcm_state, 'timeout_threshold', 'Not set')}")
              if hasattr(bcm_state, 'timestamps') and bcm_state.timestamps:
                  self.debug_print(f"DEBUG: BCM message timestamps: {list(bcm_state.timestamps)}")

    # PCM_BUTTONS (0x3B0): ACC按钮 - 每5次循环发送一次 (20Hz)
    if self.idx % 5 == 0:
      # 模拟ACC按钮状态
      # SETME_1: 2|1@0+ (1,0) [0|1]
      # BTN_AccUpDown_Cmd: 4|2@0+ (1,0) [0|3]
      # BTN_AccCancel: 6|1@0+ (1,0) [0|1]
      # BTN_TOGGLE_ACC_OnOff: 8|1@0+ (1,0) [0|1]
      # SETME2_1: 12|1@0+ (1,0) [0|1]
      # BTN_AccDistanceDecrease: 15|1@0+ (1,0) [0|1]
      # BTN_AccDistanceIncrease: 16|1@0+ (1,0) [0|1]

      # 根据simulator_state.cruise_button设置相应的按钮状态
      btn_acc_up_down_cmd = 0
      btn_acc_cancel = 0
      btn_toggle_acc_onoff = 0
      btn_acc_distance_decrease = 0
      btn_acc_distance_increase = 0

      # 根据cruise_button的值设置相应的按钮
      if simulator_state.cruise_button == 1:  # MAIN button
        btn_toggle_acc_onoff = 1
      elif simulator_state.cruise_button == 2:  # CANCEL button
        btn_acc_cancel = 1
      elif simulator_state.cruise_button == 3:  # DECEL_SET button
        btn_acc_up_down_cmd = 1  # SET_MINUS
      elif simulator_state.cruise_button == 4:  # RES_ACCEL button
        btn_acc_up_down_cmd = 2  # SET_PLUS

      pcm_buttons_data = [
        0x00,  # SETME_1
        btn_acc_up_down_cmd,  # BTN_AccUpDown_Cmd
        btn_acc_cancel,  # BTN_AccCancel
        btn_toggle_acc_onoff,  # BTN_TOGGLE_ACC_OnOff
        0x00,  # SETME2_1
        btn_acc_distance_decrease,  # BTN_AccDistanceDecrease
        btn_acc_distance_increase,  # BTN_AccDistanceIncrease
        (self.idx % 16) & 0x0F  # Counter (4位)
      ]
      pcm_buttons_msg = (0x3B0, pcm_buttons_data, CanBus.ESC)
      msg.append(pcm_buttons_msg)
      self.debug_print(f"DEBUG: Sending PCM_BUTTONS message: addr=0x3B0, data={pcm_buttons_data}, cruise_button={simulator_state.cruise_button}")

    # BELT (0x294): 安全带状态 - 每10次循环发送一次 (10Hz)
    if self.idx % 10 == 0:
      # 模拟安全带已系好
      # SeatBeat: 16|2@1+ (1,0) [0|3]
      belt_data = [
        0x00, 0x00,
        0x02,  # SeatBeat = 2 (已系好)
        0x00, 0x00, 0x00, 0x00, 0x00
      ]
      belt_msg = (0x294, belt_data, CanBus.ESC)
      msg.append(belt_msg)
      self.debug_print(f"DEBUG: Sending BELT message: addr=0x294, data={belt_data}")

    # YAW_RATE (0x222): 横摆角速度 - 每次都发送 (50Hz)
    # 根据DBC文件，YAW_RATE消息长度为8字节
    # YawRate: 0|12@1+ (0.002133,-2.094) [-2.0|2.0] "rad/s"
    yaw_rate_val = int(0.0 * 1000 / 2.133 + 984) & 0xFFF  # 0 rad/s转换为原始值
    yaw_rate_data = [
        yaw_rate_val & 0xFF,             # 字节0: YawRate低字节
        (yaw_rate_val >> 8) & 0x0F,      # 字节1: YawRate高字节的低4位
        0x00, 0x00, 0x00, 0x00,          # 字节2-5: 其他字段
        (self.idx % 16) & 0x0F,          # 字节6: Counter (4位)
        0x00                             # 字节7: 保留
    ]
    yaw_rate_msg = (0x222, yaw_rate_data, CanBus.ESC)
    msg.append(yaw_rate_msg)

    # AXAY (0x223): 加速度 - 每次都发送 (50Hz)
    # 根据DBC文件，AXAY消息长度为8字节
    # Ax: 0|12@1+ (0.027167,-21.593) [-10.0|10.0] "m/s^2"
    ax_val = int(0.0 * 1000 / 27.167 + 795) & 0xFFF  # 0 m/s^2转换为原始值
    axay_data = [
        ax_val & 0xFF,                   # 字节0: Ax低字节
        (ax_val >> 8) & 0x0F,            # 字节1: Ax高字节的低4位
        0x00, 0x00, 0x00, 0x00,          # 字节2-5: 其他字段
        (self.idx % 16) & 0x0F,          # 字节6: Counter (4位)
        0x00                             # 字节7: 保留
    ]
    axay_msg = (0x223, axay_data, CanBus.ESC)
    msg.append(axay_msg)

    # DATETIME (0x2B6): 时间日期 - 每100次循环发送一次 (1Hz)
    if self.idx % 100 == 0:
      # 模拟时间日期信息
      # YY: 7|8@0+ (1,0) [0|255]
      # MM: 15|8@0+ (1,0) [0|255]
      # DD: 23|8@0+ (1,0) [0|255]
      # hh: 31|8@0+ (1,0) [0|255]
      # mm: 39|8@0+ (1,0) [0|255]
      # ss: 47|8@0+ (1,0) [0|255]
      datetime_data = [
        0x18,  # YY (2024)
        0x0B,  # MM (11月)
        0x18,  # DD (24日)
        0x02,  # hh (02时)
        0x15,  # mm (21分)
        0x00,  # ss (00秒)
        0x00, 0x00
      ]
      datetime_msg = (0x2B6, datetime_data, CanBus.ESC)
      msg.append(datetime_msg)
      self.debug_print(f"DEBUG: Sending DATETIME message: addr=0x2B6, data={datetime_data}")

    # *** MPC bus (Bus 2) ***

    # ACC_HUD_ADAS (0x32D): ACC HUD状态 - 每次都发送 (50Hz)
    # 根据DBC文件，ACC_HUD_ADAS消息长度为8字节
    acc_hud_data = [
        0x00, 0x00,                      # 字节0-1: SetSpeed
        0x01,                            # 字节2: HasLead等字段 (设置HasLead=1)
        0x03,                            # 字节3: AccState等字段 (设置AccState=3表示ACC_ACTIVE)
        0x00, 0x00, 0x00, 0x00           # 字节4-7: 其他字段
    ]
    acc_hud_adas_msg = (0x32D, acc_hud_data, CanBus.MPC)
    msg.append(acc_hud_adas_msg)

    # ACC_CMD (0x32E): ACC控制命令 - 每次都发送 (50Hz)
    # 根据DBC文件，ACC_CMD消息长度为8字节
    acc_cmd_data = [
      0x00,  # AccelCmd
      0x00,  # ComfortBandUpper
      0x00,  # ComfortBandLower
      0x00,  # JerkUpperLimit
      0x01,  # SETME1_0x1
      0x00,  # JerkLowerLimit
      0x00,  # ResumeFromStandstill
      0x00   # StandstillState等字段
    ]
    acc_cmd_msg = (0x32E, acc_cmd_data, CanBus.MPC)
    msg.append(acc_cmd_msg)

    # ACC_MPC_STATE (0x316): LKAS转向控制 - 每次都发送 (50Hz)
    # 根据DBC文件，ACC_MPC_STATE消息长度为8字节
    acc_mpc_data = [
        0x00,                            # 字节0: AutoFullBeamState等字段
        0x01,                            # 字节1: LeftLaneState (设置为绿色)
        0x02,                            # 字节2: LKAS_Config = 2 (LKA)
        0x01,                            # 字节3: SETME2_0x1
        0x01,                            # 字节4: ReqHandsOnSteeringWheel
        0x01,                            # 字节5: MPC_State
        0x01,                            # 字节6: AutoFullBeam_OnOff
        (self.idx % 16) & 0x0F           # 字节7: Counter (4位)
    ]
    acc_mpc_state_msg = (0x316, acc_mpc_data, CanBus.MPC)
    msg.append(acc_mpc_state_msg)

    # RADAR_MRR (0x374): 雷达目标信息 - 每5次循环发送一次 (10Hz)
    if self.idx % 5 == 0:
      # 模拟雷达目标信息
      # TargetID: 1|2@0+ (1,0) [0|3]
      # Type: 7|4@0+ (1,0) [0|15]
      # LatDist: 15|8@0+ (0.1,-12) [0|255]
      # LongDist: 23|8@0+ (1,-100) [0|255]
      # IsValid: 40|1@0+ (1,0) [0|3]
      radar_mrr_data = [
        0x00,  # TargetID
        0x00,  # Type
        0x00,  # LatDist
        0x64,  # LongDist (100米)
        0x00, 0x00, 0x00,
        (self.idx % 16) & 0x0F  # Counter (4位)
      ]
      radar_mrr_msg = (0x374, radar_mrr_data, CanBus.MPC)
      msg.append(radar_mrr_msg)
      self.debug_print(f"DEBUG: Sending RADAR_MRR message: addr=0x374, data={radar_mrr_data}")

    # 分离不同总线的消息
    pt_msgs = [m for m in msg if m[2] == CanBus.ESC]  # 总线0的消息
    cam_msgs = [m for m in msg if m[2] == CanBus.MPC]  # 总线2的消息
    debug_print(f"DEBUG: pt_msgs count: {len(pt_msgs)}, cam_msgs count: {len(cam_msgs)}")

    # 构造正确的数据结构用于更新CAN解析器
    timestamp = self.idx * 1000000  # 模拟时间戳 (nanoseconds)

    # 更新动力总成总线解析器
    if hasattr(self, 'cp') and self.cp:
      # 设置controls_ready标志，这样CAN解析器才会打印CAN_INVALID消息
      debug_print(f"DEBUG: Checking pt controls_ready attribute")
      if hasattr(self.cp[Bus.pt], 'controls_ready'):
          debug_print(f"DEBUG: Setting pt controls_ready to True")
          self.cp[Bus.pt].controls_ready = True
          debug_print(f"DEBUG: Set pt controls_ready = {self.cp[Bus.pt].controls_ready}")
      else:
          debug_print(f"DEBUG: pt controls_ready attribute not found")

      # 总是设置摄像头总线的controls_ready标志，即使没有消息
      if hasattr(self.cp[Bus.cam], 'controls_ready'):
          debug_print(f"DEBUG: Setting cam controls_ready to True")
          self.cp[Bus.cam].controls_ready = True
          debug_print(f"DEBUG: Set cam controls_ready = {self.cp[Bus.cam].controls_ready}")
      else:
          debug_print(f"DEBUG: cam controls_ready attribute not found")

      if pt_msgs:
        # 构造正确的CAN条目格式: [timestamp, [(address, data, src)]]
        pt_frames = []
        for m in pt_msgs:
            pt_frames.append((m[0], bytes(m[1]), m[2]))  # (address, data, src) - 确保data是bytes类型
        pt_entry = [timestamp, pt_frames]
        try:
            updated_pt = self.cp[Bus.pt].update([pt_entry])
            debug_print(f"DEBUG: updated_pt: {updated_pt}")

            # 特别检查BCM消息是否被正确解析
            if 301 in self.cp[Bus.pt].vl:
                debug_print(f"DEBUG: BCM message parsed successfully: {self.cp[Bus.pt].vl[301]}")
            else:
                debug_print(f"DEBUG: BCM message not found in parser")
        except Exception as e:
            debug_print(f"DEBUG: Error updating pt parser: {e}")
            import traceback
            traceback.print_exc()

    # 更新摄像头总线解析器
    if hasattr(self, 'cp') and self.cp:
        if cam_msgs:
          # 构造正确的CAN条目格式: [timestamp, [(address, data, src)]]
          cam_frames = []
          for m in cam_msgs:
              cam_frames.append((m[0], bytes(m[1]), m[2]))  # (address, data, src) - 确保data是bytes类型
          cam_entry = [timestamp, cam_frames]
          try:
              updated_cam = self.cp[Bus.cam].update([cam_entry])
              debug_print(f"DEBUG: updated_cam: {updated_cam}")
          except Exception as e:
              debug_print(f"DEBUG: Error updating cam parser: {e}")
              import traceback
              traceback.print_exc()

    # 添加详细的调试信息，每10次循环打印一次
    if self.debug_counter % 10 == 0:
        self.debug_print(f"DEBUG: Sending CAN messages, idx={self.idx}")
        for m in msg:
            # 解析消息名称
            addr = m[0]
            bus = m[2]
            data = m[1]
            self.debug_print(f"  Message: addr=0x{addr:x}, bus={bus}, data={data}")

            # 特别检查BCM消息
            if addr == 0x12D and bus == CanBus.ESC:
                self.debug_print(f"  *** BCM MESSAGE DETAIL ***")
                self.debug_print(f"    Data bytes: {data}")
                self.debug_print(f"    Data hex: {[hex(b) for b in data]}")
                # 解析BCM消息字段
                if len(data) >= 3:
                    # FrontLeftDoor, FrontRightDoor, RearLeftDoor, RearRightDoor, BootDoor (byte 0)
                    front_left_door = data[0] & 0x01
                    front_right_door = (data[0] >> 1) & 0x01
                    rear_left_door = (data[0] >> 2) & 0x01
                    rear_right_door = (data[0] >> 3) & 0x01
                    boot_door = (data[0] >> 4) & 0x01
                    self.debug_print(f"    FrontLeftDoor: {front_left_door}")
                    self.debug_print(f"    FrontRightDoor: {front_right_door}")
                    self.debug_print(f"    RearLeftDoor: {rear_left_door}")
                    self.debug_print(f"    RearRightDoor: {rear_right_door}")
                    self.debug_print(f"    BootDoor: {boot_door}")

                    # DriverSeatBeltFasten (byte 1, bit 6)
                    driver_seat_belt = (data[1] >> 6) & 0x01
                    self.debug_print(f"    DriverSeatBeltFasten: {driver_seat_belt}")

                    # BrakeLight (byte 2, bit 0)
                    brake_light = data[2] & 0x01
                    self.debug_print(f"    BrakeLight: {brake_light}")

    try:
      # 添加详细的调试信息，每10次循环打印一次
      if self.debug_counter % 10 == 0:
          if hasattr(self, 'cp') and self.cp is not None:
              # 打印可用的信号
              try:
                  self.debug_print(f"DEBUG: Available messages in cp[Bus.pt].vl: {list(self.cp[Bus.pt].vl.keys())}")

                  # 检查关键信号的值
                  if 'EPS' in self.cp[Bus.pt].vl:
                      steer_angle = self.cp[Bus.pt].vl['EPS'].get('SteeringAngle', 'N/A')
                      self.debug_print(f"DEBUG: EPS.SteeringAngle: {steer_angle}")
                  else:
                      self.debug_print(f"DEBUG: EPS message not available in parser")

                  if 'CARSPEED' in self.cp[Bus.pt].vl:
                      car_speed = self.cp[Bus.pt].vl['CARSPEED'].get('CarDisplaySpeed', 'N/A')
                      self.debug_print(f"DEBUG: CARSPEED.CarDisplaySpeed: {car_speed}")
                  else:
                      self.debug_print(f"DEBUG: CARSPEED message not available in parser")

                  if 'PEDAL' in self.cp[Bus.pt].vl:
                      accelerator_pedal = self.cp[Bus.pt].vl['PEDAL'].get('AcceleratorPedal', 'N/A')
                      brake_pedal = self.cp[Bus.pt].vl['PEDAL'].get('BrakePedal', 'N/A')
                      self.debug_print(f"DEBUG: PEDAL.AcceleratorPedal: {accelerator_pedal}")
                      self.debug_print(f"DEBUG: PEDAL.BrakePedal: {brake_pedal}")
                  else:
                      self.debug_print(f"DEBUG: PEDAL message not available in parser")

                  # 检查CAN解析器状态
                  self.debug_print(f"DEBUG: CAN parser pt can_valid: {self.cp[Bus.pt].can_valid}")
                  self.debug_print(f"DEBUG: CAN parser cam can_valid: {self.cp[Bus.cam].can_valid}")
                  self.debug_print(f"DEBUG: CAN parser pt bus_timeout: {self.cp[Bus.pt].bus_timeout}")
                  self.debug_print(f"DEBUG: CAN parser cam bus_timeout: {self.cp[Bus.cam].bus_timeout}")

                  # 检查检查列表
                  if hasattr(self.cp[Bus.pt], 'checks'):
                      self.debug_print(f"DEBUG: CAN parser pt checks: {self.cp[Bus.pt].checks}")
                  if hasattr(self.cp[Bus.cam], 'checks'):
                      self.debug_print(f"DEBUG: CAN parser cam checks: {self.cp[Bus.cam].checks}")
              except Exception as e:
                  self.debug_print(f"DEBUG: Error printing available signals: {e}")
          else:
              self.debug_print(f"DEBUG: cp is None")
    except Exception as e:
      self.debug_print(f"DEBUG: CAN parser update error: {e}")
      import traceback
      traceback.print_exc()

  def send_panda_state(self, simulator_state):  # type: ignore
    # 确保发送selfdriveState消息以防止UI显示"Waiting to start"
    try:
        if self.pm:
            from cereal import messaging
            # 发送selfdriveState消息
            ss_msg = messaging.new_message('selfdriveState')
            ss_msg.valid = True
            ss = ss_msg.selfdriveState
            ss.enabled = False
            ss.active = False
            ss.state = 0  # PRE_ENABLED state
            ss.engageable = True
            ss.experimentalMode = False
            ss.personality = 0
            ss.alertText1 = ""
            ss.alertText2 = ""
            ss.alertSize = 0
            ss.alertStatus = 0
            ss.alertType = ""
            ss.alertSound = 0
            ss.alertHudVisual = 0
            ss.distanceTraveled = 0.0
            self.pm.send('selfdriveState', ss_msg)
            debug_print("DEBUG: Sent selfdriveState message")
    except Exception as e:
        debug_print(f"DEBUG: Error sending selfdriveState: {e}")
        import traceback
        traceback.print_exc()

    try:
        if self.sm is not None:
          self.sm.update(0)
          # 简化检查，直接尝试访问
          try:
              car_params = self.sm['carParams']
              debug_print(f"DEBUG: carParams available: True")
              debug_print(f"DEBUG: carParams.carFingerprint: '{car_params.carFingerprint}'")
              debug_print(f"DEBUG: carParams.fingerprintSource: {car_params.fingerprintSource}")
              # 安全地获取carFw长度
              try:
                  car_fw_attr = getattr(car_params, 'carFw', [])
                  car_fw_length = len(car_fw_attr) if hasattr(car_fw_attr, '__len__') else 0
                  debug_print(f"DEBUG: carParams.carFw: {car_fw_length} items")
              except:
                  debug_print(f"DEBUG: carParams.carFw: unavailable")
              try:
                debug_print(f"DEBUG: carParams.brand: {car_params.brand}")
              except Exception as e:
                debug_print(f"DEBUG: carParams.brand error: {e}")
              try:
                debug_print(f"DEBUG: carParams.alternativeExperience: {car_params.alternativeExperience}")
              except Exception as e:
                debug_print(f"DEBUG: carParams.alternativeExperience error: {e}")
          except:
              debug_print(f"DEBUG: carParams available: False, sm is None")
        else:
          debug_print(f"DEBUG: carParams available: False, sm is None")
    except Exception as e:
      debug_print(f"DEBUG: carParams available: False, error: {e}")

    if self.params.get_bool("ObdMultiplexingEnabled") != self.obd_multiplexing:
      self.obd_multiplexing = not self.obd_multiplexing
      self.params.put_bool("ObdMultiplexingChanged", True)

    # 模拟发送pandaStates消息
    try:
      if self.pm:
        # 使用正确的消息创建方法
        from cereal import messaging
        from cereal import log
        msg = messaging.new_message('pandaStates', 1)
        msg.valid = True
        ps = msg.pandaStates[0]

        # 设置panda状态字段
        ps.ignitionLine = simulator_state.ignition
        ps.pandaType = log.PandaState.PandaType.blackPanda
        ps.controlsAllowed = True
        # 注意：safetyModel应该是枚举值而不是字符串
        # 使用BYD的安全模型枚举值
        ps.safetyModel = 9  # BYD safety model enum value
        ps.alternativeExperience = 0
        ps.safetyParam = 1

        # 设置CAN状态信息
        # 创建CAN状态对象
        can_state0 = log.PandaState.PandaCanState.new_message()
        can_state0.busOff = False
        can_state0.busOffCnt = 0
        can_state0.errorWarning = False
        can_state0.errorPassive = False
        can_state0.lastError = log.PandaState.PandaCanState.LecErrorCode.noError
        can_state0.lastStoredError = log.PandaState.PandaCanState.LecErrorCode.noError
        can_state0.lastDataError = log.PandaState.PandaCanState.LecErrorCode.noError
        can_state0.lastDataStoredError = log.PandaState.PandaCanState.LecErrorCode.noError
        can_state0.receiveErrorCnt = 0
        can_state0.transmitErrorCnt = 0
        can_state0.totalErrorCnt = 0
        can_state0.totalTxLostCnt = 0
        can_state0.totalRxLostCnt = 0
        can_state0.totalTxCnt = 1000
        can_state0.totalRxCnt = 1000
        can_state0.totalFwdCnt = 0
        can_state0.canSpeed = 500
        can_state0.canDataSpeed = 500
        can_state0.canfdEnabled = False
        can_state0.brsEnabled = False
        can_state0.canfdNonIso = False
        can_state0.irq0CallRate = 0
        can_state0.irq1CallRate = 0
        can_state0.irq2CallRate = 0
        can_state0.canCoreResetCnt = 0

        # 设置CAN状态字段
        ps.canState0 = can_state0

        # canState1 (复制canState0的设置)
        can_state1 = log.PandaState.PandaCanState.new_message()
        can_state1.busOff = False
        can_state1.busOffCnt = 0
        can_state1.errorWarning = False
        can_state1.errorPassive = False
        can_state1.lastError = log.PandaState.PandaCanState.LecErrorCode.noError
        can_state1.lastStoredError = log.PandaState.PandaCanState.LecErrorCode.noError
        can_state1.lastDataError = log.PandaState.PandaCanState.LecErrorCode.noError
        can_state1.lastDataStoredError = log.PandaState.PandaCanState.LecErrorCode.noError
        can_state1.receiveErrorCnt = 0
        can_state1.transmitErrorCnt = 0
        can_state1.totalErrorCnt = 0
        can_state1.totalTxLostCnt = 0
        can_state1.totalRxLostCnt = 0
        can_state1.totalTxCnt = 1000
        can_state1.totalRxCnt = 1000
        can_state1.totalFwdCnt = 0
        can_state1.canSpeed = 500
        can_state1.canDataSpeed = 500
        can_state1.canfdEnabled = False
        can_state1.brsEnabled = False
        can_state1.canfdNonIso = False
        can_state1.irq0CallRate = 0
        can_state1.irq1CallRate = 0
        can_state1.irq2CallRate = 0
        can_state1.canCoreResetCnt = 0
        ps.canState1 = can_state1

        # canState2 (复制canState0的设置)
        can_state2 = log.PandaState.PandaCanState.new_message()
        can_state2.busOff = False
        can_state2.busOffCnt = 0
        can_state2.errorWarning = False
        can_state2.errorPassive = False
        can_state2.lastError = log.PandaState.PandaCanState.LecErrorCode.noError
        can_state2.lastStoredError = log.PandaState.PandaCanState.LecErrorCode.noError
        can_state2.lastDataError = log.PandaState.PandaCanState.LecErrorCode.noError
        can_state2.lastDataStoredError = log.PandaState.PandaCanState.LecErrorCode.noError
        can_state2.receiveErrorCnt = 0
        can_state2.transmitErrorCnt = 0
        can_state2.totalErrorCnt = 0
        can_state2.totalTxLostCnt = 0
        can_state2.totalRxLostCnt = 0
        can_state2.totalTxCnt = 1000
        can_state2.totalRxCnt = 1000
        can_state2.totalFwdCnt = 0
        can_state2.canSpeed = 500
        can_state2.canDataSpeed = 500
        can_state2.canfdEnabled = False
        can_state2.brsEnabled = False
        can_state2.canfdNonIso = False
        can_state2.irq0CallRate = 0
        can_state2.irq1CallRate = 0
        can_state2.irq2CallRate = 0
        can_state2.canCoreResetCnt = 0
        ps.canState2 = can_state2

        self.pm.send('pandaStates', msg)
    except Exception as e:
      self.debug_print(f"DEBUG: Error sending pandaStates: {e}")
      import traceback
      traceback.print_exc()

  def update(self, simulator_state):
    try:
      self.send_can_messages(simulator_state)

      if self.idx % 5 == 0:  # send panda state at 20hz
        self.send_panda_state(simulator_state)

      self.idx += 1
    except Exception:
      traceback.print_exc()
      raise

# 添加调试打印函数
def debug_print(*args, **kwargs):
    # 检查是否启用了调试输出（默认情况下不启用）
    if os.environ.get('SIM_DISABLE_DEBUG', '1') == '1':
        return
    print(*args, **kwargs)
