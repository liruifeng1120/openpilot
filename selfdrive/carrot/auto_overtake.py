#!/usr/bin/env python3
"""
现代汽车自动超车控制器
集成到OpenPilot中的自动超车控制器
访问地址: http://op_ip:8088

优化改动说明：
1. 高速公路应急车道处理
2. 车道数对齐OpenPilot
3. 本车车道识别优化
4. 变道成功次数统计改进
5. 预打灯机制
6. 道路序号智能编号
7. 超车冷却时间优化
8. 返回喜好车道逻辑优化
"""

import os
import sys
import json
import time
import threading
import socket
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse, parse_qs

# 添加到OpenPilot路径
sys.path.append('/data/openpilot')
try:
    import cereal.messaging as messaging
    from common.realtime import Ratekeeper
    from common.params import Params
    OP_AVAILABLE = True
    print("✅ OpenPilot环境检测成功")
except ImportError:
    print("❌ 错误：未找到OpenPilot环境")
    sys.exit(1)

class AutoOvertakeController:
    def __init__(self):
        self.vehicle_data = self._init_vehicle_data()
        self.control_state = self._init_control_state()
        self.config = self._init_config()

        # 消息发布/订阅
        self.pm = messaging.PubMaster(['autoOvertake'])
        # 新增：订阅laneChangeState以获取OpenPilot变道状态
        self.sm = messaging.SubMaster([
            'carState', 'carControl', 'radarState',
            'modelV2', 'selfdriveState', 'liveLocationKalman', 'laneChangeState'
        ])
        self.params = Params()

        # UDP客户端用于发送指令
        self.udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.remote_ip = "127.0.0.1"
        self.remote_port = 4211

        # 指令索引
        self.cmd_index = 0
        self.last_command_time = 0

        # 变道检测相关
        self.last_steering_angle = 0
        self.lane_change_start_time = 0
        self.lane_change_direction = ""
        self.steering_threshold = 5.0  # 方向盘角度阈值
        
        # 盲区延时相关
        self.blindspot_detected_time = 0
        self.blindspot_waiting = False
        self.blindspot_direction = ""
        
        # 新增：转向灯预打灯相关
        self.turn_signal_pre_engaged = False
        self.turn_signal_start_time = 0
        self.turn_signal_direction = ""
        self.pre_signal_delay = 3000  # 预打灯3秒

        # 新增：返回喜好车道相关
        self.return_to_preferred_timer = 0
        self.return_in_progress = False

        # 线程控制
        self.running = True
        self.data_thread = None
        self.web_server = None

        # 加载持久化配置
        self.load_persistent_config()

        print("✅ 控制器初始化完成")

    def _init_vehicle_data(self):
        """初始化车辆数据结构 - 新增carrot_atc_type和op_lane_count字段"""
        return {
            'v_cruise_kph': 0, 'v_ego_kph': 0, 'IsOnroad': False,
            'desire_speed': 0, 'active': False, 'lead_speed': 0,
            'lead_distance': 0, 'lead_relative_speed': 0, 'lane_count': 3,
            'l_lane_width': 3.2, 'r_lane_width': 3.2, 'l_edge_dist': 1.5,
            'r_edge_dist': 1.5, 'road_curvature': 0.0, 'steering_angle': 0.0,
            'lat_a': 0.0, 'max_curve': 0.0, 'atc_type': 'none',
            'left_blindspot': False, 'right_blindspot': False,
            'left_lead_speed': 0, 'left_lead_distance': 0, 'left_lead_relative_speed': 0,
            'right_lead_speed': 0, 'right_lead_distance': 0, 'right_lead_relative_speed': 0,
            'blinker': 'none', 'gas_press': False, 'break_press': False,
            'engaged': False, 'l_front_blind': False, 'r_front_blind': False,
            'carrot_atc_type': 'none',  # 新增：carrot服务的ATC类型
            'op_lane_count': 3,  # 新增：OpenPilot检测到的车道数
            'lane_change_state': 'off'  # 新增：OpenPilot变道状态
        }

    def _init_control_state(self):
        """初始化控制状态 - 新增预打灯和返回喜好车道状态字段"""
        return {
            'current_status': '就绪', 'last_command': '', 'blinker_state': 'none',
            'cruise_active': False, 'isOvertaking': False, 'overtakeState': '等待超车条件',
            'overtakeReason': '分析道路情况中...', 'overtakingCompleted': False,
            'overtakeSuccessCount': 0, 'lastOvertakeDirection': '',
            'lastOvertakeTime': 0, 'lastLaneChangeCommandTime': 0,
            'lane_change_in_progress': False,
            'blindspot_waiting': False, 'blindspot_wait_start': 0,
            'turn_signal_pre_engaged': False,  # 新增：转向灯预打灯状态
            'turn_signal_start_time': 0,       # 新增：转向灯开始时间
            'turn_signal_direction': '',       # 新增：转向灯方向
            'return_to_preferred_in_progress': False,  # 新增：返回喜好车道状态
            'follow_start_time': 0  # 新增：跟车开始时间
        }

    def _init_config(self):
        """初始化配置参数 - 修改冷却时间为8秒，新增预打灯和返回喜好车道参数"""
        return {
            'road_type': 'highway', 'lane_count': 3, 'preferred_lane': 2,
            'current_lane_number': 2, 'autoOvertakeEnabled': False,
            'shouldReturnToLane': True, 'autoLaneCountEnabled': True,
            'HIGHWAY_MIN_SPEED': 75.0, 'NORMAL_ROAD_MIN_SPEED': 40.0,
            'CRUISE_SPEED_RATIO_THRESHOLD': 0.8, 'FOLLOW_DISTANCE_THRESHOLD': 100,
            'MIN_FOLLOW_TIME': 5000, 'OVERTAKE_COOLDOWN': 8000, 'RETURN_DELAY': 10000,  # 优化：冷却时间改为8秒
            'MIN_LANE_WIDTH': 2.5, 'SAFE_LANE_WIDTH': 3.0, 'SIDE_LEAD_DISTANCE_MIN': 15.0,
            'SIDE_RELATIVE_SPEED_THRESHOLD': 20, 'CURVATURE_THRESHOLD': 0.02,
            'STEERING_THRESHOLD': 20.0, 'LEAD_RELATIVE_SPEED_THRESHOLD': -5.0,
            'BLINDSPOT_WAIT_TIME': 2000,  # 盲区等待时间(ms)
            'PRE_SIGNAL_DELAY': 3000,     # 新增：预打灯延时(ms)
            'RETURN_TO_PREFERRED_DELAY': 5000  # 新增：返回喜好车道延时(ms)
        }

    def load_persistent_config(self):
        """从持久化存储加载配置"""
        try:
            config_json = self.params.get("AutoOvertakeConfig")
            if config_json is not None:
                saved_config = json.loads(config_json)
                print(f"📥 加载保存的配置")
                for key, value in saved_config.items():
                    if key in self.config:
                        self.config[key] = value
            else:
                print("📥 使用默认配置")
        except Exception as e:
            print(f"⚠️ 加载配置失败: {e}")

    def save_persistent_config(self):
        """保存配置到持久化存储"""
        try:
            self.params.put("AutoOvertakeConfig", json.dumps(self.config))
            print("✅ 配置已保存")
        except Exception as e:
            print(f"⚠️ 保存配置失败: {e}")

    def update_vehicle_data(self):
        """更新车辆数据 - 新增OpenPilot变道状态和车道数检测"""
        try:
            # 基础在线状态
            isOnroad = self.params.get_bool("IsOnroad")
            self.vehicle_data['IsOnroad'] = isOnroad

            if isOnroad:
              self.sm.update(100)  # 100ms超时
            else:
              self.sm.update(0)  # 不阻塞，不触发等待

            if isOnroad:
                # 车辆状态数据
                if self.sm.alive['carState']:
                    carState = self.sm['carState']

                    # 速度相关
                    v_ego_kph = int(carState.vEgo * 3.6 + 0.5) if carState.vEgo else 0
                    v_cruise_kph = carState.vCruise

                    self.vehicle_data.update({
                        'v_ego_kph': v_ego_kph,
                        'v_cruise_kph': v_cruise_kph,
                        'cruise_speed': v_cruise_kph,
                        'steering_angle': round(carState.steeringAngleDeg, 1) if carState.steeringAngleDeg else 0.0,
                        'blinker': self._get_blinker_state(carState.leftBlinker, carState.rightBlinker),
                        'gas_press': carState.gasPressed,
                        'break_press': carState.brakePressed,
                        'engaged': carState.cruiseState.enabled,
                        'left_blindspot': bool(carState.leftBlindspot),
                        'right_blindspot': bool(carState.rightBlindspot)
                    })

                    # 加速度
                    if carState.aEgo:
                        self.vehicle_data['lat_a'] = round(carState.aEgo, 1)

                # 雷达数据 - 前车
                if self.sm.alive['radarState']:
                    radarState = self.sm['radarState']

                    # 主前车
                    if radarState.leadOne.status:
                        leadOne = radarState.leadOne
                        self.vehicle_data.update({
                            'lead_distance': int(leadOne.dRel),
                            'lead_speed': int(leadOne.vLead * 3.6),
                            'lead_relative_speed': int(leadOne.vRel * 3.6)
                        })

                    # 左侧前车
                    if radarState.leadLeft.status:
                        leadLeft = radarState.leadLeft
                        self.vehicle_data.update({
                            'left_lead_distance': int(leadLeft.dRel),
                            'left_lead_speed': int(leadLeft.vLead * 3.6),
                            'left_lead_relative_speed': int(leadLeft.vRel * 3.6)
                        })

                    # 右侧前车
                    if radarState.leadRight.status:
                        leadRight = radarState.leadRight
                        self.vehicle_data.update({
                            'right_lead_distance': int(leadRight.dRel),
                            'right_lead_speed': int(leadRight.vLead * 3.6),
                            'right_lead_relative_speed': int(leadRight.vRel * 3.6)
                        })

                # 期望速度
                self.vehicle_data['desire_speed'] = 90

            # 模型数据 - 车道信息和盲区
            if self.sm.alive['modelV2']:
                modelV2 = self.sm['modelV2']
                meta = modelV2.meta

                self.vehicle_data.update({
                    'blinker': meta.blinker,
                    'l_front_blind': meta.leftFrontBlind,
                    'r_front_blind': meta.rightFrontBlind,
                    'l_lane_width': round(meta.laneWidthLeft, 1),
                    'r_lane_width': round(meta.laneWidthRight, 1),
                    'l_edge_dist': round(meta.distanceToRoadEdgeLeft, 1),
                    'r_edge_dist': round(meta.distanceToRoadEdgeRight, 1)
                })

                # 新增：从模型数据推断车道数
                self.estimate_lane_count_from_model()

            # 自驾状态
            if self.sm.alive['selfdriveState']:
                selfdriveState = self.sm['selfdriveState']
                self.vehicle_data['active'] = "on" if selfdriveState.active else "off"

            # 新增：变道状态监测
            if self.sm.alive['laneChangeState']:
                laneChangeState = self.sm['laneChangeState']
                self.vehicle_data['lane_change_state'] = laneChangeState.state
                # 优化：如果OpenPilot检测到变道完成，增加成功计数
                if hasattr(laneChangeState, 'laneChangeCompleted') and laneChangeState.laneChangeCompleted:
                    if not hasattr(self, 'last_lane_change_completed') or not self.last_lane_change_completed:
                        self.control_state['overtakeSuccessCount'] += 1
                        self.last_lane_change_completed = True
                        print(f"✅ OpenPilot变道成功检测，总成功次数: {self.control_state['overtakeSuccessCount']}")
                else:
                    self.last_lane_change_completed = False

            # 新增：尝试获取carrot服务的ATC类型
            try:
                carrot_atc_type = self.params.get("CarrotATCTYPE")
                if carrot_atc_type is not None:
                    self.vehicle_data['carrot_atc_type'] = carrot_atc_type.decode('utf-8')
            except Exception as e:
                print(f"获取carrot ATC类型失败: {e}")

        except Exception as e:
            print(f"更新车辆数据错误: {e}")

    def estimate_lane_count_from_model(self):
        """新增：根据模型数据估计车道数，考虑高速公路应急车道"""
        vd = self.vehicle_data
        
        # 计算估计的车道数
        if vd['l_lane_width'] > 0 and vd['r_lane_width'] > 0:
            total_width = vd['l_edge_dist'] + vd['r_edge_dist']
            avg_lane_width = (vd['l_lane_width'] + vd['r_lane_width']) / 2
            
            if avg_lane_width > 0:
                estimated_lanes = max(2, min(5, round(total_width / avg_lane_width)))
                
                # 优化：高速公路最右侧为应急车道，不计入可用车道
                if self.config['road_type'] == 'highway':
                    estimated_lanes = max(2, estimated_lanes - 1)
                    print(f"🛣️ 高速公路检测：总{estimated_lanes+1}车道，可用{estimated_lanes}车道")
                
                vd['op_lane_count'] = estimated_lanes
                
                # 如果自动车道计数启用，更新配置
                if self.config['autoLaneCountEnabled']:
                    old_count = self.config['lane_count']
                    self.config['lane_count'] = estimated_lanes
                    if old_count != estimated_lanes:
                        print(f"🔄 自动更新车道数: {old_count} -> {estimated_lanes}")

    def _get_blinker_state(self, left_blinker, right_blinker):
        """获取转向灯状态"""
        if left_blinker and right_blinker:
            return "hazard"
        elif left_blinker:
            return "left"
        elif right_blinker:
            return "right"
        else:
            return "none"

    def send_command(self, cmd_type, arg):
        """发送控制命令"""
        self.cmd_index += 1
        command = {
            "index": self.cmd_index,
            "cmd": cmd_type,
            "arg": arg,
            "timestamp": int(time.time() * 1000)
        }

        try:
            message = json.dumps(command).encode('utf-8')
            self.udp_socket.sendto(message, (self.remote_ip, self.remote_port))
            self.control_state['last_command'] = f"{cmd_type}: {arg}"
            self.last_command_time = time.time()
            print(f"📤 发送指令: {command}")
            return True
        except Exception as e:
            print(f"❌ 发送指令错误: {e}")
            return False

    def send_turn_signal(self, direction):
        """新增：发送转向灯指令"""
        return self.send_command("TURN_SIGNAL", direction)

    def check_overtake_conditions(self):
        """检查超车条件 - 修改为部分条件为或关系"""
        vd = self.vehicle_data
        cs = self.control_state
        cfg = self.config
        now = time.time() * 1000

        # 基础状态检查（仍然需要叠加满足）
        if not vd['IsOnroad']:
            cs['overtakeReason'] = "车辆不在道路上"
            return False

        if not vd['engaged']:
            cs['overtakeReason'] = "巡航未激活"
            return False

        if vd['lead_distance'] <= 0:
            cs['overtakeReason'] = "前方无车辆"
            return False

        # 新增：检查carrot ATC类型，如果不是"none"则不变道
        if vd['carrot_atc_type'] != 'none':
            cs['overtakeReason'] = f"OpenPilot正在控制中 (ATC: {vd['carrot_atc_type']})"
            return False

        # 以下四个条件改为"或"关系，满足任意一个即可
        trigger_conditions = []
        
        # 条件1: 前车相对速度检查
        condition1 = vd['lead_relative_speed'] <= cfg['LEAD_RELATIVE_SPEED_THRESHOLD']
        if condition1:
            trigger_conditions.append(f"前车相对速度{vd['lead_relative_speed']}km/h")
        
        # 条件2: 前车跟车距离检查
        condition2 = vd['lead_distance'] <= cfg['FOLLOW_DISTANCE_THRESHOLD']
        if condition2:
            trigger_conditions.append(f"前车距离{vd['lead_distance']}m")
        
        # 条件3: 巡航速度百分比检查
        speed_ratio = vd['v_ego_kph'] / vd['v_cruise_kph'] if vd['v_cruise_kph'] > 0 else 1.0
        condition3 = speed_ratio <= cfg['CRUISE_SPEED_RATIO_THRESHOLD']
        if condition3:
            trigger_conditions.append(f"速度比例{speed_ratio*100:.0f}%")
        
        # 条件4: 最小跟车时间检查
        condition4 = (now - cs.get('follow_start_time', now)) >= cfg['MIN_FOLLOW_TIME']
        if condition4:
            trigger_conditions.append("跟车时间达标")

        # 四个条件中任意一个满足即可触发
        if not (condition1 or condition2 or condition3 or condition4):
            cs['overtakeReason'] = "未满足任何触发条件"
            return False

        # 其他条件仍然需要叠加满足
        if cfg['road_type'] == 'highway' and vd['v_ego_kph'] < cfg['HIGHWAY_MIN_SPEED']:
            cs['overtakeReason'] = f"高速公路车速{vd['v_ego_kph']}km/h低于最低超车速度"
            return False

        if cfg['road_type'] == 'normal' and vd['v_ego_kph'] < cfg['NORMAL_ROAD_MIN_SPEED']:
            cs['overtakeReason'] = f"普通公路车速{vd['v_ego_kph']}km/h低于最低超车速度"
            return False

        # 优化：冷却时间改为8秒
        if now - cs['lastOvertakeTime'] < cfg['OVERTAKE_COOLDOWN']:
            remaining = (cfg['OVERTAKE_COOLDOWN'] - (now - cs['lastOvertakeTime'])) / 1000
            cs['overtakeReason'] = f"超车冷却中，请等待{remaining:.0f}秒"
            return False

        # 记录触发原因
        cs['overtakeReason'] = f"触发条件: {', '.join(trigger_conditions)}"
        return True

    def check_lane_safety(self, side):
        """检查车道安全性 - 添加高速公路应急车道处理和盲区延时逻辑"""
        vd = self.vehicle_data
        cfg = self.config
        now = time.time() * 1000

        # 优化：高速公路最右侧车道为应急车道，禁止变道
        if cfg['road_type'] == 'highway' and side == "right" and self.config['current_lane_number'] == self.config['lane_count']:
            return False, "高速应急车道，禁止变道"

        if side == "left":
            # 检查车道宽度
            if vd['l_lane_width'] < cfg['MIN_LANE_WIDTH']:
                return False, "车道过窄⚠️禁止变道"

            # 检查盲区 - 如果有盲区车辆，启动等待
            if vd['left_blindspot'] or vd['l_front_blind']:
                if not self.control_state['blindspot_waiting']:
                    # 开始盲区等待
                    self.control_state['blindspot_waiting'] = True
                    self.control_state['blindspot_wait_start'] = now
                    self.control_state['blindspot_direction'] = "left"
                    return False, "盲区有车，等待2秒..."
                else:
                    # 检查是否等待足够时间
                    if now - self.control_state['blindspot_wait_start'] >= cfg['BLINDSPOT_WAIT_TIME']:
                        # 等待时间结束，再次检查盲区
                        if vd['left_blindspot'] or vd['l_front_blind']:
                            self.control_state['blindspot_waiting'] = False
                            return False, "等待后盲区仍有车"
                        else:
                            self.control_state['blindspot_waiting'] = False
                            return True, "盲区车辆已通过，安全"
                    else:
                        remaining = (cfg['BLINDSPOT_WAIT_TIME'] - (now - self.control_state['blindspot_wait_start'])) / 1000
                        return False, f"盲区等待中... {remaining:.1f}秒"
            else:
                # 没有盲区车辆，重置等待状态
                if self.control_state['blindspot_waiting'] and self.control_state['blindspot_direction'] == "left":
                    self.control_state['blindspot_waiting'] = False

            # 检查侧方车辆
            if vd['left_lead_distance'] > 0 and vd['left_lead_distance'] < cfg['SIDE_LEAD_DISTANCE_MIN']:
                return False, "侧前车距离过近"
            if abs(vd['left_lead_relative_speed']) > cfg['SIDE_RELATIVE_SPEED_THRESHOLD']:
                return False, "侧车相对速度过高"
            return True, "安全"

        elif side == "right":
            # 检查车道宽度
            if vd['r_lane_width'] < cfg['MIN_LANE_WIDTH']:
                return False, "车道过窄⚠️禁止变道"

            # 检查盲区 - 如果有盲区车辆，启动等待
            if vd['right_blindspot'] or vd['r_front_blind']:
                if not self.control_state['blindspot_waiting']:
                    # 开始盲区等待
                    self.control_state['blindspot_waiting'] = True
                    self.control_state['blindspot_wait_start'] = now
                    self.control_state['blindspot_direction'] = "right"
                    return False, "盲区有车，等待2秒..."
                else:
                    # 检查是否等待足够时间
                    if now - self.control_state['blindspot_wait_start'] >= cfg['BLINDSPOT_WAIT_TIME']:
                        # 等待时间结束，再次检查盲区
                        if vd['right_blindspot'] or vd['r_front_blind']:
                            self.control_state['blindspot_waiting'] = False
                            return False, "等待后盲区仍有车"
                        else:
                            self.control_state['blindspot_waiting'] = False
                            return True, "盲区车辆已通过，安全"
                    else:
                        remaining = (cfg['BLINDSPOT_WAIT_TIME'] - (now - self.control_state['blindspot_wait_start'])) / 1000
                        return False, f"盲区等待中... {remaining:.1f}秒"
            else:
                # 没有盲区车辆，重置等待状态
                if self.control_state['blindspot_waiting'] and self.control_state['blindspot_direction'] == "right":
                    self.control_state['blindspot_waiting'] = False

            # 检查侧方车辆
            if vd['right_lead_distance'] > 0 and vd['right_lead_distance'] < cfg['SIDE_LEAD_DISTANCE_MIN']:
                return False, "侧前车距离过近"
            if abs(vd['right_lead_relative_speed']) > cfg['SIDE_RELATIVE_SPEED_THRESHOLD']:
                return False, "侧车相对速度过高"
            return True, "安全"

        return False, "未知方向"

    def check_lane_change_success(self):
        """优化：检查变道是否成功 - 基于OpenPilot状态和方向盘动作"""
        if not self.control_state['lane_change_in_progress']:
            return False

        # 优化：使用OpenPilot的变道状态作为主要判断
        if self.vehicle_data['lane_change_state'] == 'completed':
            print("✅ OpenPilot确认变道完成")
            return True

        # 备用判断：基于方向盘动作
        current_steering = self.vehicle_data['steering_angle']
        current_blinker = self.vehicle_data['blinker']
        direction = self.lane_change_direction

        # 检查转向灯状态
        expected_blinker = "left" if direction == "LEFT" else "right"
        if current_blinker != expected_blinker:
            return False

        # 检查方向盘动作
        if direction == "LEFT" and current_steering < -self.steering_threshold:
            return True
        elif direction == "RIGHT" and current_steering > self.steering_threshold:
            return True

        return False

    def check_pre_signal_timeout(self):
        """新增：检查预打灯超时"""
        if not self.control_state['turn_signal_pre_engaged']:
            return False
            
        now = time.time() * 1000
        if now - self.control_state['turn_signal_start_time'] >= self.config['PRE_SIGNAL_DELAY']:
            return True
        return False

    def perform_auto_overtake(self):
        """执行自动超车 - 新增预打灯机制和返回喜好车道逻辑"""
        if not self.config['autoOvertakeEnabled'] or self.control_state['isOvertaking']:
            return

        # 优化：检查是否需要返回喜好车道
        if (self.config['shouldReturnToLane'] and 
            not self.control_state['return_to_preferred_in_progress'] and
            not self.control_state['isOvertaking'] and
            self.config['current_lane_number'] != self.config['preferred_lane'] and
            time.time() * 1000 - self.control_state.get('lastOvertakeTime', 0) > self.config['RETURN_TO_PREFERRED_DELAY']):
            
            self.return_to_preferred_lane()
            return

        if not self.check_overtake_conditions():
            return

        # 新增：预打灯阶段处理
        if self.control_state['turn_signal_pre_engaged']:
            if self.check_pre_signal_timeout():
                # 预打灯时间结束，重新检查安全性并执行变道
                left_safe, left_reason = self.check_lane_safety("left")
                right_safe, right_reason = self.check_lane_safety("right")
                
                # 获取预打灯方向
                pre_direction = self.control_state['turn_signal_direction']
                
                # 检查是否需要更改转向灯方向
                if pre_direction == "LEFT" and left_safe and self.config['current_lane_number'] > 1:
                    self.execute_lane_change("LEFT")
                elif pre_direction == "RIGHT" and right_safe and self.config['current_lane_number'] < self.config['lane_count']:
                    self.execute_lane_change("RIGHT")
                else:
                    # 预打灯方向不安全，选择其他安全方向
                    if left_safe and self.config['current_lane_number'] > 1:
                        if pre_direction != "LEFT":
                            self.send_turn_signal("LEFT")  # 立即更改转向灯
                        self.execute_lane_change("LEFT")
                    elif right_safe and self.config['current_lane_number'] < self.config['lane_count']:
                        if pre_direction != "RIGHT":
                            self.send_turn_signal("RIGHT")  # 立即更改转向灯
                        self.execute_lane_change("RIGHT")
                    else:
                        # 没有安全车道，取消超车
                        self.cancel_overtake()
                        reasons = []
                        if not left_safe: reasons.append(f"左侧:{left_reason}")
                        if not right_safe: reasons.append(f"右侧:{right_reason}")
                        self.control_state['overtakeState'] = "预打灯后无安全车道"
                        self.control_state['overtakeReason'] = " | ".join(reasons)
                
                # 重置预打灯状态
                self.control_state['turn_signal_pre_engaged'] = False
            return

        # 正常超车流程
        left_safe, left_reason = self.check_lane_safety("left")
        right_safe, right_reason = self.check_lane_safety("right")

        if left_safe and self.config['current_lane_number'] > 1:
            self.start_pre_signal("LEFT")
        elif right_safe and self.config['current_lane_number'] < self.config['lane_count']:
            self.start_pre_signal("RIGHT")
        else:
            reasons = []
            if not left_safe: reasons.append(f"左侧:{left_reason}")
            if not right_safe: reasons.append(f"右侧:{right_reason}")
            self.control_state['overtakeState'] = "等待安全变道时机"
            self.control_state['overtakeReason'] = " | ".join(reasons)

    def start_pre_signal(self, direction):
        """新增：开始预打灯阶段"""
        success = self.send_turn_signal(direction)
        if success:
            self.control_state['turn_signal_pre_engaged'] = True
            self.control_state['turn_signal_start_time'] = time.time() * 1000
            self.control_state['turn_signal_direction'] = direction
            
            if direction == "LEFT":
                self.control_state['overtakeState'] = "← 预打左转向灯"
                self.control_state['current_status'] = "预打左灯"
            else:
                self.control_state['overtakeState'] = "→ 预打右转向灯"
                self.control_state['current_status'] = "预打右灯"
                
            self.control_state['overtakeReason'] = f"预打{direction}转向灯，{self.config['PRE_SIGNAL_DELAY']/1000}秒后变道"
            print(f"🚦 预打灯指令: {direction}，等待{self.config['PRE_SIGNAL_DELAY']/1000}秒")

    def execute_lane_change(self, direction):
        """执行变道操作"""
        success = self.send_command("LANECHANGE", direction)
        if success:
            self.control_state['isOvertaking'] = True
            self.control_state['lane_change_in_progress'] = True
            self.control_state['lastOvertakeTime'] = time.time() * 1000
            self.control_state['lastOvertakeDirection'] = direction
            self.control_state['lastLaneChangeCommandTime'] = time.time() * 1000

            self.lane_change_start_time = time.time()
            self.lane_change_direction = direction
            self.last_steering_angle = self.vehicle_data['steering_angle']

            if direction == "LEFT":
                self.control_state['overtakeState'] = "← 正在向左变道超车"
                self.control_state['current_status'] = "自动左变道"
            else:
                self.control_state['overtakeState'] = "→ 正在向右变道超车"
                self.control_state['current_status'] = "自动右变道"
                
            print(f"🔄 执行变道: {direction}")

    def return_to_preferred_lane(self):
        """优化：返回喜好车道 - 安全自动返回逻辑"""
        current_lane = self.config['current_lane_number']
        preferred_lane = self.config['preferred_lane']
        
        if current_lane == preferred_lane:
            return
            
        self.control_state['return_to_preferred_in_progress'] = True
        
        if current_lane < preferred_lane:
            # 需要向左变道
            left_safe, left_reason = self.check_lane_safety("left")
            if left_safe:
                self.start_pre_signal("LEFT")
                self.control_state['overtakeReason'] = f"返回喜好车道{preferred_lane} (当前:{current_lane})"
                print(f"🔄 开始返回喜好车道: {current_lane} → {preferred_lane} (向左)")
            else:
                self.control_state['overtakeState'] = "等待返回喜好车道"
                self.control_state['overtakeReason'] = f"左侧不安全: {left_reason}"
        else:
            # 需要向右变道
            right_safe, right_reason = self.check_lane_safety("right")
            if right_safe:
                # 优化：高速公路最右侧为应急车道检查
                if self.config['road_type'] == 'highway' and preferred_lane == self.config['lane_count']:
                    self.control_state['overtakeState'] = "无法返回应急车道"
                    self.control_state['overtakeReason'] = "高速公路应急车道禁止行驶"
                    self.control_state['return_to_preferred_in_progress'] = False
                    return
                    
                self.start_pre_signal("RIGHT")
                self.control_state['overtakeReason'] = f"返回喜好车道{preferred_lane} (当前:{current_lane})"
                print(f"🔄 开始返回喜好车道: {current_lane} → {preferred_lane} (向右)")
            else:
                self.control_state['overtakeState'] = "等待返回喜好车道"
                self.control_state['overtakeReason'] = f"右侧不安全: {right_reason}"

    def check_overtake_completion(self):
        """检查超车完成状态 - 新增返回喜好车道完成检测"""
        if not self.control_state['lane_change_in_progress']:
            # 优化：检查返回喜好车道是否完成
            if (self.control_state['return_to_preferred_in_progress'] and 
                self.config['current_lane_number'] == self.config['preferred_lane']):
                self.control_state['return_to_preferred_in_progress'] = False
                self.control_state['overtakeState'] = f"已返回喜好车道{self.config['preferred_lane']}"
                self.control_state['overtakeReason'] = "返回喜好车道完成"
                print(f"✅ 返回喜好车道完成: 当前车道{self.config['current_lane_number']}")
            return

        now = time.time()

        # 检查超时
        if now - self.lane_change_start_time > 15:  # 15秒超时
            self.control_state['lane_change_in_progress'] = False
            self.control_state['isOvertaking'] = False
            self.control_state['overtakeState'] = "变道超时"
            self.control_state['overtakeReason'] = "未检测到变道动作"
            return

        # 检查变道成功
        if self.check_lane_change_success():
            self.control_state['isOvertaking'] = False
            self.control_state['lane_change_in_progress'] = False
            self.control_state['overtakingCompleted'] = True
            
            # 优化：更新车道编号
            if self.lane_change_direction == "LEFT":
                self.config['current_lane_number'] = max(1, self.config['current_lane_number'] - 1)
            else:
                self.config['current_lane_number'] = min(self.config['lane_count'], self.config['current_lane_number'] + 1)
                
            direction_text = "左" if self.lane_change_direction == "LEFT" else "右"
            self.control_state['overtakeState'] = f"{direction_text}超车成功"
            self.control_state['overtakeReason'] = f"检测到{direction_text}转向动作"
            self.control_state['current_status'] = "超车完成"
            print(f"✅ 变道成功检测: {direction_text}变道完成，当前车道{self.config['current_lane_number']}")

    def manual_overtake(self, lane):
        """手动变道 - 强制发送指令"""
        direction = "LEFT" if lane == "left" else "RIGHT"
        success = self.send_command("LANECHANGE", direction)
        if success:
            self.control_state['lastOvertakeDirection'] = direction
            self.control_state['lastLaneChangeCommandTime'] = time.time() * 1000

            if lane == "left":
                self.control_state['current_status'] = "强制左变道"
                self.control_state['overtakeState'] = "← 手动左变道"
            else:
                self.control_state['current_status'] = "强制右变道"
                self.control_state['overtakeState'] = "→ 手动右变道"
            self.control_state['overtakeReason'] = "用户强制变道指令"
            print(f"🔧 手动变道指令: {direction}")

    def cancel_overtake(self):
        """取消超车 - 重置所有相关状态"""
        success = self.send_command("CANCEL_OVERTAKE", "true")
        if success:
            self.control_state['current_status'] = "取消超车"
            self.control_state['isOvertaking'] = False
            self.control_state['lane_change_in_progress'] = False
            self.control_state['overtakingCompleted'] = False
            self.control_state['blindspot_waiting'] = False  # 重置盲区等待状态
            self.control_state['turn_signal_pre_engaged'] = False  # 重置预打灯状态
            self.control_state['return_to_preferred_in_progress'] = False  # 重置返回状态
            print("❌ 超车已取消")

    def change_speed(self, direction):
        """改变速度"""
        if direction == "UP":
            self.send_command("SPEED", direction)
        elif direction == "DOWN":
            self.send_command("SPEED", direction)

    def run_data_loop(self):
        """数据循环"""
        ratekeeper = Ratekeeper(10)  # 10Hz

        while self.running:
            try:
                self.update_vehicle_data()
                self.update_lane_number()
                self.update_curve_detection()
                
                # 更新跟车开始时间
                if self.vehicle_data['lead_distance'] > 0 and not hasattr(self, 'follow_start_time_set'):
                    self.control_state['follow_start_time'] = time.time() * 1000
                    self.follow_start_time_set = True
                    print("🚗 开始跟车计时")

                if self.config['autoOvertakeEnabled']:
                    self.perform_auto_overtake()
                    self.check_overtake_completion()

                ratekeeper.keep_time()
            except Exception as e:
                print(f"数据循环错误: {e}")
                time.sleep(0.1)

    def update_lane_number(self):
        """优化：更新车道编号 - 使用更准确的算法"""
        vd = self.vehicle_data
        cfg = self.config

        if vd['r_lane_width'] > 0 and vd['r_edge_dist'] > 0 and vd['l_edge_dist'] > 0:
            # 使用左右边缘距离和车道宽度来更准确计算当前车道
            total_road_width = vd['l_edge_dist'] + vd['r_edge_dist']
            avg_lane_width = (vd['l_lane_width'] + vd['r_lane_width']) / 2
            
            if avg_lane_width > 0:
                # 优化：计算从右侧起的车道位置 (1-based)，从右向左编号
                calculated_lane = round((vd['r_edge_dist'] / avg_lane_width) + 0.5)
                calculated_lane = max(1, min(cfg['lane_count'], calculated_lane))
                
                # 验证计算结果，如果与之前差异过大，使用平滑过渡
                if abs(calculated_lane - cfg['current_lane_number']) <= 1:
                    cfg['current_lane_number'] = calculated_lane
                # 否则保持原值，避免跳动

    def update_curve_detection(self):
        """更新弯道检测"""
        vd = self.vehicle_data
        cfg = self.config

        is_curve = (vd['max_curve'] >= 1.0 or
                   abs(vd['road_curvature']) > cfg['CURVATURE_THRESHOLD'] or
                   abs(vd['steering_angle']) > cfg['STEERING_THRESHOLD'])

        if is_curve and (self.control_state['isOvertaking'] or self.control_state['turn_signal_pre_engaged']):
            self.cancel_overtake()
            self.control_state['current_status'] = "弯道中取消超车"
            self.control_state['overtakeReason'] = "检测到弯道，安全第一"
            print("🔄 检测到弯道，取消超车")

    def start_web_server(self):
        """启动Web服务器"""
        from http.server import HTTPServer
        handler = self.create_web_handler()
        self.web_server = HTTPServer(('0.0.0.0', 8088), handler)
        print("🌐 Web服务器启动在端口 8088")
        self.web_server.serve_forever()

    def create_web_handler(self):
        """创建Web处理器"""
        controller = self

        class OvertakeHTTPHandler(BaseHTTPRequestHandler):
            def do_GET(self):
                if self.path == '/':
                    self.send_html_response()
                elif self.path == '/status':
                    self.send_json_status()
                else:
                  print(f"page {self.path} not found!")

            def do_POST(self):
                try:
                    content_length = int(self.headers.get('Content-Length', 0))
                    post_data = self.rfile.read(content_length).decode('utf-8')
                    data = json.loads(post_data) if post_data else {}

                    if self.path == '/control':
                        self.handle_control(data)
                    elif self.path == '/overtake':
                        self.handle_overtake(data)
                    elif self.path == '/config':
                        self.handle_config(data)
                    elif self.path == '/params':
                        self.handle_params(data)
                    else:
                        self.send_error(404, "接口未找到")
                except Exception as e:
                    print(f"请求处理错误: {e}")
                    self.send_error(400, "请求解析错误")

            def handle_control(self, data):
                cmd_type = data.get('type', '')
                value = data.get('value', '')
                if cmd_type == 'SPEED':
                    controller.change_speed(value)
                self.send_json_response({'status': 'success', 'command': f'{cmd_type}: {value}'})

            def handle_overtake(self, data):
                if 'manual' in data:
                    controller.manual_overtake(data['manual'])
                    self.send_json_response({'status': 'success', 'action': f'manual_{data["manual"]}'})
                elif 'cancel' in data:
                    controller.cancel_overtake()
                    self.send_json_response({'status': 'success', 'action': 'cancel'})
                elif 'auto' in data:
                    controller.config['autoOvertakeEnabled'] = bool(data['auto'])
                    controller.save_persistent_config()
                    self.send_json_response({'status': 'success', 'autoOvertake': controller.config['autoOvertakeEnabled']})
                elif 'return' in data:
                    controller.config['shouldReturnToLane'] = bool(data['return'])
                    controller.save_persistent_config()
                    self.send_json_response({'status': 'success', 'returnToLane': controller.config['shouldReturnToLane']})
                else:
                    self.send_json_response({'status': 'error', 'message': '未知操作'})

            def handle_config(self, data):
                if 'lanes' in data and not controller.config['autoLaneCountEnabled']:
                    controller.config['lane_count'] = int(data['lanes'])
                if 'preferred_lane' in data:
                    controller.config['preferred_lane'] = int(data['preferred_lane'])
                if 'road_type' in data:
                    controller.config['road_type'] = data['road_type']
                    controller.save_persistent_config()
                if 'auto_lane_count' in data:
                    controller.config['autoLaneCountEnabled'] = bool(data['auto_lane_count'])
                    controller.save_persistent_config()
                self.send_json_response({'status': 'success', 'config': controller.config})

            def handle_params(self, data):
                param_map = {
                    'highwayMinSpeed': 'HIGHWAY_MIN_SPEED',
                    'normalMinSpeed': 'NORMAL_ROAD_MIN_SPEED',
                    'speedRatio': 'CRUISE_SPEED_RATIO_THRESHOLD',
                    'followDistance': 'FOLLOW_DISTANCE_THRESHOLD',
                    'minLaneWidth': 'MIN_LANE_WIDTH',
                    'safeLaneWidth': 'SAFE_LANE_WIDTH',
                    'sideLeadDist': 'SIDE_LEAD_DISTANCE_MIN',
                    'sideRelSpeed': 'SIDE_RELATIVE_SPEED_THRESHOLD',
                    'leadRelSpeed': 'LEAD_RELATIVE_SPEED_THRESHOLD',
                    'blindspotWaitTime': 'BLINDSPOT_WAIT_TIME',
                    'preSignalDelay': 'PRE_SIGNAL_DELAY',
                    'returnToPreferredDelay': 'RETURN_TO_PREFERRED_DELAY'
                }

                for web_key, config_key in param_map.items():
                    if web_key in data:
                        controller.config[config_key] = float(data[web_key])

                if 'minFollowTime' in data:
                    controller.config['MIN_FOLLOW_TIME'] = int(data['minFollowTime']) * 1000
                if 'overtakeCooldown' in data:
                    controller.config['OVERTAKE_COOLDOWN'] = int(data['overtakeCooldown']) * 1000
                if 'returnDelay' in data:
                    controller.config['RETURN_DELAY'] = int(data['returnDelay']) * 1000

                controller.save_persistent_config()
                self.send_json_response({'status': 'success', 'message': '参数已保存'})

            def send_html_response(self):
                html = self.get_html_content()
                self.send_response(200)
                self.send_header('Content-type', 'text/html; charset=utf-8')
                self.end_headers()
                self.wfile.write(html.encode('utf-8'))

            def send_json_status(self):
                status_data = self.get_status_data()
                self.send_response(200)
                self.send_header('Content-type', 'application/json')
                self.end_headers()
                self.wfile.write(json.dumps(status_data, ensure_ascii=False).encode('utf-8'))

            def send_json_response(self, data):
                self.send_response(200)
                self.send_header('Content-type', 'application/json')
                self.end_headers()
                self.wfile.write(json.dumps(data, ensure_ascii=False).encode('utf-8'))

            def get_status_data(self):
                vd = controller.vehicle_data
                cs = controller.control_state
                cfg = controller.config

                # 检查车道宽度警告
                left_lane_narrow = vd.get('l_lane_width', 3.2) < cfg.get('MIN_LANE_WIDTH', 2.5)
                right_lane_narrow = vd.get('r_lane_width', 3.2) < cfg.get('MIN_LANE_WIDTH', 2.5)

                return {
                    'w': True,
                    'ip': self.get_local_ip(),
                    's': vd.get('v_ego_kph', 0),
                    'c': vd.get('v_cruise_kph', 0),
                    'd': vd.get('desire_speed', 0),
                    'ls': vd.get('lead_speed', 0),
                    'ld': vd.get('lead_distance', 0),
                    'lrs': vd.get('lead_relative_speed', 0),
                    'lb': bool(vd.get('left_blindspot', False)),
                    'rb': bool(vd.get('right_blindspot', False)),
                    'llw': float(vd.get('l_lane_width', 3.2)),
                    'rlw': float(vd.get('r_lane_width', 3.2)),
                    'led': float(vd.get('l_edge_dist', 1.5)),
                    'red': float(vd.get('r_edge_dist', 1.5)),
                    'lls': vd.get('left_lead_speed', 0),
                    'lld': vd.get('left_lead_distance', 0),
                    'llrs': vd.get('left_lead_relative_speed', 0),
                    'rls': vd.get('right_lead_speed', 0),
                    'rld': vd.get('right_lead_distance', 0),
                    'rlrs': vd.get('right_lead_relative_speed', 0),
                    'rt': cfg.get('road_type', 'highway'),
                    'lc': cfg.get('lane_count', 3),
                    'pl': cfg.get('preferred_lane', 2),
                    'cl': cfg.get('current_lane_number', 2),
                    'alc': cfg.get('autoLaneCountEnabled', True),
                    'os': cs.get('overtakeState', '等待超车条件'),
                    'or': cs.get('overtakeReason', '分析道路情况中...'),
                    'oc': cs.get('overtakeSuccessCount', 0),
                    'hms': cfg.get('HIGHWAY_MIN_SPEED', 75),
                    'nms': cfg.get('NORMAL_ROAD_MIN_SPEED', 40),
                    'sr': cfg.get('CRUISE_SPEED_RATIO_THRESHOLD', 0.8),
                    'fd': cfg.get('FOLLOW_DISTANCE_THRESHOLD', 100),
                    'mft': cfg.get('MIN_FOLLOW_TIME', 5000),
                    'mlw': cfg.get('MIN_LANE_WIDTH', 2.5),
                    'slw': cfg.get('SAFE_LANE_WIDTH', 3.0),
                    'sld': cfg.get('SIDE_LEAD_DISTANCE_MIN', 15),
                    'srs': cfg.get('SIDE_RELATIVE_SPEED_THRESHOLD', 20),
                    'ocd': cfg.get('OVERTAKE_COOLDOWN', 8000),
                    'rd': cfg.get('RETURN_DELAY', 10000),
                    'aoe': cfg.get('autoOvertakeEnabled', True),
                    'srtl': cfg.get('shouldReturnToLane', True),
                    'lrs_threshold': cfg.get('LEAD_RELATIVE_SPEED_THRESHOLD', -5.0),
                    'bwt': cfg.get('BLINDSPOT_WAIT_TIME', 2000),
                    'psd': cfg.get('PRE_SIGNAL_DELAY', 3000),
                    'rtpd': cfg.get('RETURN_TO_PREFERRED_DELAY', 5000),
                    'carrot_atc': vd.get('carrot_atc_type', 'none'),
                    'blindspot_waiting': cs.get('blindspot_waiting', False),
                    'turn_signal_pre_engaged': cs.get('turn_signal_pre_engaged', False),
                    'return_to_preferred_in_progress': cs.get('return_to_preferred_in_progress', False),
                    'op_lane_count': vd.get('op_lane_count', 3),
                    'left_lane_narrow': left_lane_narrow,
                    'right_lane_narrow': right_lane_narrow
                }

            def get_local_ip(self):
                try:
                    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                    s.connect(("8.8.8.8", 80))
                    ip = s.getsockname()[0]
                    s.close()
                    return ip
                except:
                    return "127.0.0.1"

            def get_html_content(self):
                # 读取HTML文件内容
                html_file_path = os.path.join(os.path.dirname(__file__), 'web_interface.html')
                try:
                    with open(html_file_path, 'r', encoding='utf-8') as f:
                        return f.read()
                except FileNotFoundError:
                    return "<html><body><h1>错误：未找到HTML界面文件</h1></body></html>"

            def log_message(self, format, *args):
                pass

        return OvertakeHTTPHandler

    def start(self):
        print("🚗 启动现代汽车自动超车控制器...")
        self.data_thread = threading.Thread(target=self.run_data_loop, daemon=True)
        self.data_thread.start()
        self.start_web_server()

    def stop(self):
        self.running = False
        if self.web_server:
            self.web_server.shutdown()
        if self.udp_socket:
            self.udp_socket.close()
        print("现代汽车自动超车控制器已停止")

def main():
    print("="*50)
    print("现代汽车自动超车控制器")
    print("访问地址: http://<op_ip>:8088")
    print("="*50)

    controller = AutoOvertakeController()
    try:
        controller.start()
    except KeyboardInterrupt:
        print("\n收到停止信号...")
    except Exception as e:
        print(f"运行错误: {e}")
    finally:
        controller.stop()

if __name__ == "__main__":
    main()