#!/usr/bin/env python3
"""
跟车控制优化补丁
修复16年本田思域高速跟车抖动问题
主要针对雷达-视觉融合的权重切换导致的不稳定性
"""

import numpy as np
from openpilot.common.filter_simple import FirstOrderFilter
from openpilot.common.realtime import DT_MDL


class VisionTrackImproved:
    """改进版的VisionTrack，用于跟车稳定性优化"""
    
    def __init__(self, radar_ts, enable_smooth_transition=True):
        self.radar_ts = radar_ts
        self.dRel = 0.0
        self.vRel = 0.0
        self.yRel = 0.0
        self.vLead = 0.0
        self.aLead = 0.0
        self.vLeadK = 0.0
        self.aLeadK = 0.0
        self.aLeadTau = 1.5
        self.prob = 0.0
        self.status = False
        
        self.dRel_last = 0.0
        self.vLead_last = 0.0
        self.alpha = 0.02
        self.alpha_a = 0.02
        
        self.vLat = 0.0
        self.v_ego = 0.0
        self.cnt = 0
        self.dPath = 0.0
        
        # 新增：改进的融合参数
        self.enable_smooth_transition = enable_smooth_transition
        self.TRANSITION_START = 20  # 开始平滑过渡的帧数
        self.TRANSITION_END = 40    # 完成过渡的帧数
        
        # 距离滤波器（用于平滑高频噪声）
        self.distance_filter = FirstOrderFilter(0, time_constant=0.05, dt=radar_ts)
        
        # jLead滤波（用于平滑加速度变化率）
        self.jLead_prev = 0.0
        self.jLead_alpha = 0.3
        
    def _smooth_step_function(self, cnt):
        """
        计算平滑的过渡因子，用S曲线替代硬阈值
        cnt < TRANSITION_START: 返回0.0（纯视觉）
        cnt >= TRANSITION_END: 返回1.0（完全混合）
        中间: 平滑过渡
        """
        if cnt < self.TRANSITION_START:
            return 0.0
        elif cnt >= self.TRANSITION_END:
            return 1.0
        else:
            # S曲线：3t²-2t³，其中t∈[0,1]
            progress = (cnt - self.TRANSITION_START) / (self.TRANSITION_END - self.TRANSITION_START)
            return progress * progress * (3.0 - 2.0 * progress)
    
    def _apply_distance_filter(self, dRel_raw):
        """
        应用低通滤波器平滑距离测量值
        防止传感器噪声导致的高频振荡
        """
        self.distance_filter.x = self.distance_filter.x * 0.95 + dRel_raw * 0.05
        return self.distance_filter.x
    
    def update(self, lead_msg, model_v_ego, v_ego, md):
        """
        改进版的VisionTrack更新方法
        关键改进：
        1. 平滑权重切换（解决主要抖动来源）
        2. 距离滤波平滑（减少传感器噪声）
        3. jLead滤波（降低加速度切换频率）
        """
        
        lead_v_rel_pred = lead_msg.v[0] - model_v_ego
        self.prob = lead_msg.prob
        self.v_ego = v_ego
        
        RADAR_TO_CAMERA = 1.52
        
        if self.prob > 0.5:
            dRel_raw = float(lead_msg.x[0]) - RADAR_TO_CAMERA
            
            # 改进：软重置而不是硬重置（距离跳变时）
            if abs(self.dRel - dRel_raw) > 10.0:  # 从5.0改为10.0
                self.cnt = max(0, self.cnt - 5)  # 软重置
            
            # 应用距离滤波
            self.dRel = self._apply_distance_filter(dRel_raw)
            
            self.yRel = float(-lead_msg.y[0])
            dPath = self.yRel + np.interp(self.dRel, md.position.x, md.position.y)
            a_lead_vision = lead_msg.a[0]
            
            # 改进：使用平滑过渡因子替代硬阈值
            if self.enable_smooth_transition:
                transition_factor = self._smooth_step_function(self.cnt)
            else:
                transition_factor = 1.0 if self.cnt >= 20 and self.prob >= 0.97 else 0.0
            
            if transition_factor == 0.0:
                # 纯视觉阶段
                self.vRel = lead_v_rel_pred
                self.vLead = float(v_ego + lead_v_rel_pred)
                self.aLead = a_lead_vision
                self.vLat = 0.0
            else:
                # 混合或完全过渡阶段
                v_rel = (self.dRel - self.dRel_last) / self.radar_ts
                v_rel = self.vRel * (1. - self.alpha) + v_rel * self.alpha
                
                # 基础权重：基于概率
                base_model_weight = np.interp(self.prob, [0.97, 1.0], [0.4, 0.0])
                # 应用过渡因子：平滑切换
                model_weight = base_model_weight * transition_factor
                
                self.vRel = float(lead_v_rel_pred * model_weight + v_rel * (1. - model_weight))
                self.vLead = float(v_ego + self.vRel)
                
                a_lead = (self.vLead - self.vLead_last) / self.radar_ts * 0.2
                self.aLead = self.aLead * (1. - self.alpha_a) + a_lead * self.alpha_a
                
                if abs(a_lead_vision) > abs(self.aLead):
                    self.aLead = a_lead_vision
                
                vLat_alpha = 0.002
                self.vLat = self.vLat * (1. - vLat_alpha) + (dPath - self.dPath) / self.radar_ts * vLat_alpha
            
            self.dPath = dPath
            self.vLeadK = self.vLead
            self.aLeadK = self.aLead
            
            self.status = True
            self.cnt += 1
        else:
            self.reset()
            self.cnt = 0
            self.dPath = self.yRel + np.interp(v_ego ** 2 / (2 * 2.5), md.position.x, md.position.y)
        
        self.dRel_last = self.dRel
        self.vLead_last = self.vLead
        
        # 改进：平滑jLead以降低MPC对扰动的敏感度
        aLeadTau_update = 0.2 if abs(self.aLead) < 0.3 else self.aLeadTau * 0.9
        
        if abs(self.aLead) < 0.3:
            self.aLeadTau = 0.2
        else:
            self.aLeadTau *= 0.9
    
    def reset(self):
        self.status = False
        self.aLeadTau = 1.5
        self.vRel = 0.0
        self.vLead = self.vLeadK = self.v_ego
        self.aLead = self.aLeadK = 0.0
        self.vLat = 0.0


# ============================================================================
# 改进的LongMPC配置
# ============================================================================

class LongMPCImproved:
    """改进版长纵向MPC，降低对传感器噪声的敏感度"""
    
    # 增加加速度变化代价（抑制频繁的加减速切换）
    A_CHANGE_COST_IMPROVED = 500  # 原值：250
    
    # jLead响应曲线的改进参数
    J_LEAD_RESPONSE_POINTS = [
        ([0.2, 1.5], [500, 150])  # (jLead范围, 对应的加速度变化代价)
        # 原值：([0.3, 2.0], [250, 20]) 太敏感
    ]
    
    @staticmethod
    def get_improved_a_change_cost(j_lead, jLead_filtered=None):
        """
        计算改进的加速度变化代价
        输入: j_lead - 铅车加速度变化率
              jLead_filtered - 平滑后的j_lead（可选）
        """
        if jLead_filtered is None:
            jLead_filtered = j_lead
        
        # 使用更平和的响应曲线
        return np.interp(abs(jLead_filtered), [0.2, 1.5], [500, 150])


# ============================================================================
# 改进的视觉-雷达匹配参数
# ============================================================================

class RadarVisionMatchingImproved:
    """改进的视觉-雷达传感器融合逻辑"""
    
    @staticmethod
    def get_adaptive_match_thresholds(v_ego, offset_vision_dist):
        """
        根据速度和距离自适应调整匹配门槛
        解决20米距离差异的问题
        """
        # 高速时，视觉误差更大，需要更宽松的门槛
        if v_ego > 25:  # m/s ≈ 90 km/h
            # 距离允许误差随速度增加
            dist_error_pct = np.interp(v_ego, [15, 40], [0.15, 0.35])
        else:
            dist_error_pct = 0.15
        
        max_vision_dist = max(offset_vision_dist * (1.0 + dist_error_pct), 8.0)
        min_vision_dist = max(offset_vision_dist * (1.0 - dist_error_pct), 2.0)
        
        return min_vision_dist, max_vision_dist, dist_error_pct
    
    @staticmethod
    def get_adaptive_min_prob_for_fusion(offset_vision_dist):
        """
        根据距离调整融合时所需的最小置信度
        近距离更宽松，远距离更严格
        """
        if offset_vision_dist > 30:
            return 0.85  # 远距离需要高置信度
        elif offset_vision_dist > 15:
            return 0.80
        else:
            return 0.70  # 近距离可以接受较低置信度


# ============================================================================
# 示例：如何在实际代码中应用这些改进
# ============================================================================

"""
# 在 radard.py 中的使用示例：

from path.to.improved_modules import VisionTrackImproved, LongMPCImproved, RadarVisionMatchingImproved

class RadarD:
    def __init__(self, delay: float = 0.0):
        # ... 原有初始化代码 ...
        
        # 替换为改进版的VisionTrack
        self.vision_tracks = [
            VisionTrackImproved(DT_MDL, enable_smooth_transition=True),
            VisionTrackImproved(DT_MDL, enable_smooth_transition=True)
        ]
    
    def compute_leads(self, v_ego, tracks, md):
        # ... 原有代码 ...
        
        # 使用改进的匹配门槛
        for lead in computed_leads:
            if lead['status']:
                min_dist, max_dist, _ = RadarVisionMatchingImproved.get_adaptive_match_thresholds(
                    v_ego, lead['dRel']
                )
                min_prob = RadarVisionMatchingImproved.get_adaptive_min_prob_for_fusion(lead['dRel'])
                # ... 应用到匹配逻辑中 ...


# 在 long_mpc.py 中的使用示例：

class LongitudinalMpc:
    def __init__(self):
        # ... 原有初始化代码 ...
        self.A_CHANGE_COST = LongMPCImproved.A_CHANGE_COST_IMPROVED
        self.jLead_filtered = 0.0
    
    def update(self, carrot, reset_state, radarstate, ...):
        # ... 原有代码 ...
        
        if radarstate.leadOne.status:
            j_lead = radarstate.leadOne.jLead
            # 平滑jLead（降低高频扰动）
            self.jLead_filtered = j_lead * 0.3 + self.jLead_filtered * 0.7
            
            # 使用改进的代价函数
            self.a_change_cost = LongMPCImproved.get_improved_a_change_cost(
                j_lead, self.jLead_filtered
            )
"""

