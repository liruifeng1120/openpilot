#!/usr/bin/env python3
"""
跟车性能诊断脚本
用于分析高速跟车时的抖动问题原因
"""

import numpy as np
from collections import deque
from dataclasses import dataclass
from typing import List, Optional


@dataclass
class RoadEvent:
    """路面事件"""
    timestamp: float
    event_type: str  # 'weight_switch', 'distance_jump', 'vision_update', 'radar_update'
    value: float  # 事件关联的数值


class FollowCarAnalyzer:
    """跟车性能分析工具"""
    
    def __init__(self, sample_rate: float = 100.0):  # Hz
        self.dt = 1.0 / sample_rate
        self.sample_rate = sample_rate
        
        # 历史数据缓存（保留60秒）
        self.max_samples = int(sample_rate * 60)
        
        self.timestamps = deque(maxlen=self.max_samples)
        self.accel = deque(maxlen=self.max_samples)
        self.distance = deque(maxlen=self.max_samples)
        self.v_ego = deque(maxlen=self.max_samples)
        self.v_lead = deque(maxlen=self.max_samples)
        self.v_rel = deque(maxlen=self.max_samples)
        self.vision_prob = deque(maxlen=self.max_samples)
        self.radar_status = deque(maxlen=self.max_samples)
        self.vision_track_cnt = deque(maxlen=self.max_samples)
        self.model_weight = deque(maxlen=self.max_samples)
        
        # 事件日志
        self.events: List[RoadEvent] = []
        self.current_time = 0.0
    
    def update(self, accel, distance, v_ego, v_lead, vision_prob, 
               radar_status, vision_track_cnt, model_weight):
        """添加一个采样点"""
        self.timestamps.append(self.current_time)
        self.accel.append(accel)
        self.distance.append(distance)
        self.v_ego.append(v_ego)
        self.v_lead.append(v_lead)
        self.v_rel.append(v_lead - v_ego)
        self.vision_prob.append(vision_prob)
        self.radar_status.append(radar_status)
        self.vision_track_cnt.append(vision_track_cnt)
        self.model_weight.append(model_weight)
        
        self.current_time += self.dt
    
    def log_event(self, event_type: str, value: float):
        """记录一个事件"""
        self.events.append(RoadEvent(
            timestamp=self.current_time,
            event_type=event_type,
            value=value
        ))
    
    def compute_metrics(self) -> dict:
        """计算性能指标"""
        if len(self.accel) < 2:
            return {}
        
        accel_list = list(self.accel)
        distance_list = list(self.distance)
        v_rel_list = list(self.v_rel)
        vision_prob_list = list(self.vision_prob)
        model_weight_list = list(self.model_weight)
        
        # 计算jerk（加速度变化率）
        accel_diff = np.diff(accel_list)
        jerk = accel_diff / self.dt
        jerk_std = np.std(jerk)
        jerk_max = np.max(np.abs(jerk))
        
        # 加速度统计
        accel_mean = np.mean(accel_list)
        accel_std = np.std(accel_list)
        accel_max = np.max(np.abs(accel_list))
        
        # 距离统计
        distance_std = np.std(distance_list)
        distance_diff = np.diff(distance_list)
        distance_jump_indices = np.where(np.abs(distance_diff) > 0.5)[0]
        distance_jump_count = len(distance_jump_indices)
        
        # 相对速度统计
        v_rel_std = np.std(v_rel_list)
        v_rel_diff = np.diff(v_rel_list)
        v_rel_jump_indices = np.where(np.abs(v_rel_diff) > 0.3)[0]
        v_rel_jump_count = len(v_rel_jump_indices)
        
        # 权重切换分析
        weight_diff = np.diff(model_weight_list)
        weight_switch_indices = np.where(np.abs(weight_diff) > 0.1)[0]
        weight_switch_count = len(weight_switch_indices)
        
        # 关联性分析：权重切换与加速度突变的关联
        accel_spike_indices = np.where(np.abs(accel_diff) > 0.5)[0]
        if len(weight_switch_indices) > 0 and len(accel_spike_indices) > 0:
            # 检查是否在权重切换后500ms内有加速度突变
            correlation_count = 0
            for accel_idx in accel_spike_indices:
                for weight_idx in weight_switch_indices:
                    time_diff = abs((accel_idx - weight_idx) * self.dt)
                    if 0 <= time_diff <= 0.5:  # 500ms窗口
                        correlation_count += 1
            correlation_ratio = correlation_count / len(accel_spike_indices)
        else:
            correlation_ratio = 0.0
        
        return {
            'jerk_std': jerk_std,           # 标准差（低越好）
            'jerk_max': jerk_max,           # 峰值（低越好）
            'accel_mean': accel_mean,       # 平均（应接近0）
            'accel_std': accel_std,         # 标准差（低越好）
            'accel_max': accel_max,         # 峰值（低越好）
            'distance_std': distance_std,   # 距离波动（低越好）
            'distance_jump_count': distance_jump_count,  # 距离跳变次数
            'v_rel_std': v_rel_std,         # 相对速度标准差
            'v_rel_jump_count': v_rel_jump_count,  # 相对速度跳变次数
            'weight_switch_count': weight_switch_count,  # 权重切换次数
            'weight_accel_correlation': correlation_ratio,  # 权重切换与加速度的关联比例
            'sample_count': len(self.accel),
            'duration_sec': self.current_time,
        }
    
    def analyze_root_causes(self) -> dict:
        """分析根本原因"""
        metrics = self.compute_metrics()
        
        if not metrics:
            return {'status': 'insufficient_data'}
        
        causes = []
        severity = 0.0
        
        # 检查1：权重切换与加速度的关联
        weight_accel_corr = metrics.get('weight_accel_correlation', 0.0)
        if weight_accel_corr > 0.3:
            causes.append({
                'cause': '权重切换导致的加速度跳变',
                'severity': min(1.0, weight_accel_corr),
                'evidence': f"在{metrics['weight_switch_count']}次权重切换中，"
                           f"{int(metrics['weight_switch_count'] * weight_accel_corr)}次"
                           f"伴随加速度突变(>0.5 m/s²)",
                'solution': '优先级1：实施平滑权重过渡'
            })
            severity += 0.4 * weight_accel_corr
        
        # 检查2：距离跳变
        distance_jump_count = metrics.get('distance_jump_count', 0)
        total_samples = metrics.get('sample_count', 1)
        jump_frequency = distance_jump_count / (total_samples - 1) if total_samples > 1 else 0
        
        if jump_frequency > 0.01:  # 平均每100个采样点有1次跳变
            causes.append({
                'cause': '距离测量噪声/跳变',
                'severity': min(1.0, jump_frequency * 10),
                'evidence': f"在{total_samples}个采样中发现{distance_jump_count}次"
                           f"距离跳变(>0.5m)，频率={jump_frequency*100:.2f}%",
                'solution': '优先级2：添加距离滤波和软重置'
            })
            severity += 0.3 * jump_frequency * 10
        
        # 检查3：相对速度不稳定
        v_rel_std = metrics.get('v_rel_std', 0)
        if v_rel_std > 0.5:
            causes.append({
                'cause': '相对速度估计不稳定',
                'severity': min(1.0, v_rel_std / 2.0),
                'evidence': f"相对速度标准差={v_rel_std:.3f} m/s (应<0.2 m/s)",
                'solution': '优先级3：使用自适应融合门槛'
            })
            severity += 0.2 * min(1.0, v_rel_std / 2.0)
        
        # 检查4：加速度变化率过高
        jerk_std = metrics.get('jerk_std', 0)
        if jerk_std > 0.5:
            causes.append({
                'cause': '加速度变化频繁(jerk过高)',
                'severity': min(1.0, jerk_std / 1.0),
                'evidence': f"加速度变化率标准差={jerk_std:.3f} m/s³ (应<0.2 m/s³)",
                'solution': '优先级4：降低MPC对扰动的敏感度'
            })
            severity += 0.1 * min(1.0, jerk_std / 1.0)
        
        return {
            'status': 'analysis_complete',
            'overall_severity': min(1.0, severity),
            'causes': causes,
            'metrics': metrics,
        }
    
    def print_diagnosis_report(self):
        """打印诊断报告"""
        analysis = self.analyze_root_causes()
        
        if analysis['status'] != 'analysis_complete':
            print("数据不足，无法进行分析")
            return
        
        print("\n" + "="*70)
        print("跟车控制诊断报告")
        print("="*70)
        
        metrics = analysis['metrics']
        print(f"\n【数据统计】")
        print(f"  采样点数: {metrics['sample_count']}")
        print(f"  采样时长: {metrics['duration_sec']:.1f}秒")
        print(f"  采样频率: {self.sample_rate:.1f}Hz")
        
        print(f"\n【加速度指标】")
        print(f"  平均值:     {metrics['accel_mean']:>7.3f} m/s²  (理想: 0)")
        print(f"  标准差:     {metrics['accel_std']:>7.3f} m/s²  (理想: <0.1)")
        print(f"  最大值:     {metrics['accel_max']:>7.3f} m/s²  (理想: <0.5)")
        
        print(f"\n【加速度变化率(Jerk)指标】")
        print(f"  标准差:     {metrics['jerk_std']:>7.3f} m/s³  (理想: <0.2)")
        print(f"  最大值:     {metrics['jerk_max']:>7.3f} m/s³  (理想: <0.5)")
        
        print(f"\n【距离指标】")
        print(f"  标准差:     {metrics['distance_std']:>7.3f} m    (理想: <0.3)")
        print(f"  跳变次数:   {metrics['distance_jump_count']:>7d} 次   (理想: 0)")
        
        print(f"\n【相对速度指标】")
        print(f"  标准差:     {metrics['v_rel_std']:>7.3f} m/s  (理想: <0.2)")
        print(f"  跳变次数:   {metrics['v_rel_jump_count']:>7d} 次   (理想: <5)")
        
        print(f"\n【融合质量指标】")
        print(f"  权重切换次数: {metrics['weight_switch_count']:>5d} 次")
        print(f"  权重-加速度关联: {metrics['weight_accel_correlation']*100:>5.1f}%  (理想: <10%)")
        
        print(f"\n【问题诊断】")
        print(f"  整体严重度: {analysis['overall_severity']*100:>5.1f}%")
        
        if analysis['causes']:
            print(f"\n  发现{len(analysis['causes'])}个问题根源:\n")
            for i, cause in enumerate(analysis['causes'], 1):
                severity_bar = "█" * int(cause['severity'] * 20)
                print(f"  {i}. {cause['cause']}")
                print(f"     严重度: [{severity_bar:<20}] {cause['severity']*100:.0f}%")
                print(f"     证据:   {cause['evidence']}")
                print(f"     方案:   {cause['solution']}")
                print()
        else:
            print("\n  未发现明显问题。系统表现良好！")
        
        print("="*70 + "\n")
    
    def get_problem_regions(self, accel_threshold: float = 0.5) -> List[dict]:
        """找出加速度异常的时间段"""
        regions = []
        accel_list = list(self.accel)
        
        in_region = False
        region_start = 0
        
        for i, accel in enumerate(accel_list):
            if abs(accel) > accel_threshold:
                if not in_region:
                    region_start = i
                    in_region = True
            else:
                if in_region:
                    regions.append({
                        'start_idx': region_start,
                        'end_idx': i,
                        'start_time': self.timestamps[region_start],
                        'end_time': self.timestamps[i],
                        'max_accel': max([abs(a) for a in accel_list[region_start:i]]),
                    })
                    in_region = False
        
        if in_region:
            regions.append({
                'start_idx': region_start,
                'end_idx': len(accel_list),
                'start_time': self.timestamps[region_start],
                'end_time': self.timestamps[-1],
                'max_accel': max([abs(a) for a in accel_list[region_start:]]),
            })
        
        return regions


# 使用示例
if __name__ == '__main__':
    # 创建分析器
    analyzer = FollowCarAnalyzer(sample_rate=50)  # 50Hz采样
    
    # 模拟一个有问题的跟车场景
    # (实际使用时应该从真实数据源读取)
    current_accel = 0.0
    for i in range(1000):  # 20秒的数据
        # 模拟权重切换导致的速度跳变
        if i == 200:
            analyzer.log_event('weight_switch', 0.5)
            current_accel = 0.8  # 权重切换导致加速度跳变
        elif i == 210:
            current_accel = -0.6
        elif i == 500:
            analyzer.log_event('distance_jump', 2.5)
        else:
            # 正常跟车，平滑加速度
            current_accel *= 0.99
        
        # 模拟测量数据
        analyzer.update(
            accel=current_accel,
            distance=30.0 + 0.1 * np.sin(i * 0.01) + (2.5 if i == 500 else 0),
            v_ego=25.0,
            v_lead=25.0 + 0.05 * np.sin(i * 0.02),
            vision_prob=0.95 if i % 50 > 10 else 0.70,
            radar_status=True,
            vision_track_cnt=i if i < 20 else 20,
            model_weight=0.0 if i < 20 else min(1.0, (i - 20) / 20)
        )
    
    # 生成诊断报告
    analyzer.print_diagnosis_report()
    
    # 找出问题区域
    problem_regions = analyzer.get_problem_regions(accel_threshold=0.5)
    if problem_regions:
        print("发现的问题时间段:")
        for region in problem_regions:
            print(f"  {region['start_time']:.1f}s - {region['end_time']:.1f}s, "
                  f"最大加速度: {region['max_accel']:.3f} m/s²")
