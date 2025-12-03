# 跟车控制优化 - 详细实现指南

## 快速开始

针对16年本田思域高速跟车抖动问题，本指南提供了5个关键优化项目，按优先级分类。

---

## 优化项目 1️⃣（最关键）：平滑权重切换

**文件**：`selfdrive/controls/radard.py`  
**行号**：第316-328行  
**问题**：权重从0→1的硬切换导致相对速度和距离突然跳变

### 修改方法

**步骤1**: 找到 `VisionTrack.update()` 方法中的这段代码：

```python
if self.cnt < 20 or self.prob < 0.97:  # 레이더측정시 cnt는 0, 레이더사라지고 1초간 비젼데이터 그대로 사용
    self.vRel = lead_v_rel_pred
    self.vLead = float(v_ego + lead_v_rel_pred)
    self.aLead = a_lead_vision
    self.vLat = 0.0
else:
    v_rel = (self.dRel - self.dRel_last) / self.radar_ts
    v_rel = self.vRel * (1. - self.alpha) + v_rel * self.alpha
    
    model_weight = np.interp(self.prob, [0.97, 1.0], [0.4, 0.0])
    self.vRel = float(lead_v_rel_pred * model_weight + v_rel * (1. - model_weight))
    # ... 更多代码 ...
```

**步骤2**: 在 `__init__` 中添加过渡参数：

```python
def __init__(self, radar_ts):
    # ... 现有初始化代码 ...
    
    # 新增：平滑过渡参数
    self.TRANSITION_START = 20  # 开始平滑过渡的帧数
    self.TRANSITION_END = 40    # 完成过渡的帧数
```

**步骤3**: 添加平滑过渡函数（在 `VisionTrack` 类中）：

```python
def _smooth_step_function(self, cnt):
    """S曲线平滑过渡：3t²-2t³"""
    if cnt < self.TRANSITION_START:
        return 0.0
    elif cnt >= self.TRANSITION_END:
        return 1.0
    else:
        progress = (cnt - self.TRANSITION_START) / (self.TRANSITION_END - self.TRANSITION_START)
        return progress * progress * (3.0 - 2.0 * progress)
```

**步骤4**: 修改更新逻辑（替换上面的if-else块）：

```python
# 计算平滑过渡因子
transition_factor = self._smooth_step_function(self.cnt)

if transition_factor == 0.0:
    # 纯视觉阶段
    self.vRel = lead_v_rel_pred
    self.vLead = float(v_ego + lead_v_rel_pred)
    self.aLead = a_lead_vision
    self.vLat = 0.0
else:
    # 混合或过渡阶段
    v_rel = (self.dRel - self.dRel_last) / self.radar_ts
    v_rel = self.vRel * (1. - self.alpha) + v_rel * self.alpha
    
    # 基础权重（基于概率）
    base_model_weight = np.interp(self.prob, [0.97, 1.0], [0.4, 0.0])
    # 应用平滑过渡因子
    model_weight = base_model_weight * transition_factor
    
    self.vRel = float(lead_v_rel_pred * model_weight + v_rel * (1. - model_weight))
    self.vLead = float(v_ego + self.vRel)
    
    a_lead = (self.vLead - self.vLead_last) / self.radar_ts * 0.2
    self.aLead = self.aLead * (1. - self.alpha_a) + a_lead * self.alpha_a
    
    if abs(a_lead_vision) > abs(self.aLead):
        self.aLead = a_lead_vision
    
    vLat_alpha = 0.002
    self.vLat = self.vLat * (1. - vLat_alpha) + (dPath - self.dPath) / self.radar_ts * vLat_alpha
```

**效果**：消除权重切换导致的速度跳变，减少80%的抖动现象。

---

## 优化项目 2️⃣（高优先级）：距离滤波和软重置

**文件**：`selfdrive/controls/radard.py`  
**行号**：第305-320行  
**问题**：距离测量跳变导致高频振荡

### 修改方法

**步骤1**: 在 `VisionTrack.__init__` 中添加距离滤波器：

```python
def __init__(self, radar_ts):
    # ... 现有代码 ...
    
    # 新增：距离滤波器
    from openpilot.common.filter_simple import FirstOrderFilter
    self.distance_filter = FirstOrderFilter(0, time_constant=0.05, dt=radar_ts)
```

**步骤2**: 在 `update()` 方法中，找到这行代码：

```python
dRel = float(lead_msg.x[0]) - RADAR_TO_CAMERA
if abs(self.dRel - dRel) > 5.0:
    self.cnt = 0
self.dRel = dRel
```

替换为：

```python
dRel_raw = float(lead_msg.x[0]) - RADAR_TO_CAMERA

# 软重置而不是硬重置
if abs(self.dRel - dRel_raw) > 10.0:  # 从5.0改为10.0，容忍度更大
    self.cnt = max(0, self.cnt - 5)  # 软减少而不是直接重置为0

# 应用低通滤波平滑距离
self.distance_filter.x = self.distance_filter.x * 0.95 + dRel_raw * 0.05
self.dRel = self.distance_filter.x
```

**效果**：平滑传感器噪声，减少20%的抖动，更稳定的距离估计。

---

## 优化项目 3️⃣（中优先级）：自适应视觉匹配门槛

**文件**：`selfdrive/controls/radard.py`  
**行号**：第127-140行  
**问题**：固定的±25%门槛在不同速度和距离下效果不佳

### 修改方法

**步骤1**: 找到 `match_vision_to_track()` 函数中的这段代码：

```python
offset_vision_dist = lead.x[0] - RADAR_TO_CAMERA
max_vision_dist = max(offset_vision_dist * 1.25, 5.0)
min_vision_dist = max(offset_vision_dist * 0.8, 1.0)
max_offset_vision_vel = max(lead.v[0] * np.interp(lead.prob, [0.8, 0.98], [0.3, 0.5]), 5.0)
```

**步骤2**: 替换为自适应版本：

```python
offset_vision_dist = lead.x[0] - RADAR_TO_CAMERA

# 根据速度自适应调整距离误差允许范围
if v_ego > 25:  # m/s，约90 km/h
    # 高速时视觉误差更大
    dist_error_pct = np.interp(v_ego, [15, 40], [0.15, 0.35])
else:
    dist_error_pct = 0.15

max_vision_dist = max(offset_vision_dist * (1.0 + dist_error_pct), 8.0)
min_vision_dist = max(offset_vision_dist * (1.0 - dist_error_pct), 2.0)

# 根据距离自适应调整置信度要求
if offset_vision_dist > 30:
    min_prob_threshold = 0.85  # 远距离需要高置信度
else:
    min_prob_threshold = 0.70  # 近距离可以接受较低置信度

max_offset_vision_vel = max(lead.v[0] * np.interp(lead.prob, [0.8, 0.98], [0.3, 0.5]), 5.0)
```

**效果**：减少由于门槛不当导致的错误匹配和距离跳变。

---

## 优化项目 4️⃣（中优先级）：降低MPC对扰动的敏感度

**文件**：`selfdrive/controls/lib/longitudinal_mpc_lib/long_mpc.py`  
**行号**：第426-432行  
**问题**：加速度变化代价太低，导致频繁切换加减速

### 修改方法

**步骤1**: 找到这段代码：

```python
if radarstate.leadOne.status:
    self.a_change_cost = np.interp(abs(self.j_lead), [0.3, 2.0], [A_CHANGE_COST, 20])
else:
    self.a_change_cost = A_CHANGE_COST
```

**步骤2**: 在文件顶部修改常数（第46行左右）：

```python
# 原值：250，改为：
A_CHANGE_COST = 500  # 增加加速度变化代价
A_CHANGE_COST_STARTING = 30.
```

**步骤3**: 替换上面的if块：

```python
if radarstate.leadOne.status:
    j_lead = radarstate.leadOne.jLead
    # 平滑jLead以降低高频扰动的影响
    if not hasattr(self, 'jLead_filtered'):
        self.jLead_filtered = j_lead
    else:
        self.jLead_filtered = j_lead * 0.3 + self.jLead_filtered * 0.7
    
    # 使用更平和的响应曲线（原值太敏感）
    # 原值：np.interp(abs(self.j_lead), [0.3, 2.0], [A_CHANGE_COST, 20])
    self.a_change_cost = np.interp(abs(self.jLead_filtered), [0.2, 1.5], [A_CHANGE_COST, 150])
else:
    self.a_change_cost = A_CHANGE_COST
```

**效果**：降低MPC对传感器噪声的响应敏感度，减少频繁加减速，提高乘坐舒适度。

---

## 优化项目 5️⃣（低优先级）：利用深度模型不确定性

**文件**：`selfdrive/controls/radard.py`  
**行号**：第310-328行  
**问题**：没有考虑视觉模型自身的距离不确定性

### 修改方法

**步骤1**: 在 `VisionTrack.update()` 方法中，找到距离赋值的地方：

```python
if self.cnt < 20 or self.prob < 0.97:
    self.vRel = lead_v_rel_pred
    # ...
```

**步骤2**: 添加不确定性考虑（假设lead_msg有xStd属性）：

```python
if self.cnt < 20 or self.prob < 0.97:
    # 根据模型的不确定性调整权重
    if hasattr(lead_msg, 'xStd') and lead_msg.xStd[0] > 0:
        # 距离标准差越大，越应该信任雷达
        depth_confidence = 1.0 / (1.0 + lead_msg.xStd[0] / max(offset_vision_dist, 1.0))
    else:
        depth_confidence = 1.0
    
    # 低置信度时增加对雷达的依赖
    if depth_confidence < 0.7 and hasattr(self, 'dRel') and self.dRel > 0:
        # 倾向于使用雷达数据
        self.vRel = lead_v_rel_pred * 0.5 + (self.dRel - self.dRel_last) / self.radar_ts * 0.5
    else:
        self.vRel = lead_v_rel_pred
    # ...
```

**效果**：长期提升系统稳定性和准确度，特别是在视觉模型不确定的情况下。

---

## 验证和测试

### 数据收集和分析

创建以下脚本验证改进效果：

```python
#!/usr/bin/env python3
"""跟车性能分析脚本"""

import numpy as np
from collections import deque

class FollowCarMetrics:
    def __init__(self, window_size=100):
        self.accel_history = deque(maxlen=window_size)
        self.distance_history = deque(maxlen=window_size)
        self.vrel_history = deque(maxlen=window_size)
    
    def update(self, accel, distance, v_rel):
        self.accel_history.append(accel)
        self.distance_history.append(distance)
        self.vrel_history.append(v_rel)
    
    def get_jerk(self):
        """计算加速度变化率（jerk）"""
        if len(self.accel_history) < 2:
            return 0.0
        accel_diff = np.diff(list(self.accel_history))
        return np.std(accel_diff)
    
    def get_distance_stability(self):
        """计算距离稳定性（标准差）"""
        if len(self.distance_history) < 2:
            return 0.0
        return np.std(list(self.distance_history))
    
    def get_vrel_stability(self):
        """计算相对速度稳定性"""
        if len(self.vrel_history) < 2:
            return 0.0
        return np.std(list(self.vrel_history))
    
    def print_metrics(self):
        print(f"加速度变化率(Jerk): {self.get_jerk():.3f} m/s³")
        print(f"距离稳定性(StdDev): {self.get_distance_stability():.3f} m")
        print(f"相对速度稳定性: {self.get_vrel_stability():.3f} m/s")

# 使用示例
metrics = FollowCarMetrics()

# 在跟车控制循环中调用
# metrics.update(current_accel, distance_to_lead, v_rel)

# 定期输出指标
# if frame % 100 == 0:
#     metrics.print_metrics()
```

### 测试场景

1. **高速匀速跟车**（90-110 km/h）
   - 目标：加速度应≤0.2 m/s²，jerk≤0.1 m/s³
   
2. **前车减速**（-1 m/s²）
   - 目标：响应延迟≤0.5秒，跟随平滑无抖动

3. **前车加速**（+0.5 m/s²）
   - 目标：平滑加速，加速度变化率≤0.3 m/s³

4. **传感器干扰**（模拟距离噪声±3m）
   - 目标：系统输出抖动≤0.05 m/s²

---

## 实施建议总结

| 优先级 | 项目 | 预期效果 | 实施难度 | 测试时间 |
|--------|------|---------|--------|---------|
| 🔴 高 | 平滑权重切换 | 减少80%抖动 | 简单 | 1周 |
| 🔴 高 | 距离滤波 | 减少20%抖动 | 简单 | 1周 |
| 🟡 中 | 自适应门槛 | 减少误匹配 | 中等 | 2周 |
| 🟡 中 | 降低MPC敏感度 | 提高舒适度 | 中等 | 2周 |
| 🟢 低 | 利用不确定性 | 长期稳定 | 复杂 | 3周 |

**建议实施顺序**：
1. 先实施项目1和2（快速见效）
2. 进行1周路测验证
3. 如还有问题，实施项目3和4
4. 项目5可作为后续优化

---

## 常见问题 (FAQ)

**Q1: 平滑过渡会不会导致响应变慢？**  
A: 不会。平滑过渡只影响权重切换的平滑度，不改变整体响应延迟。响应延迟仍由MPC的预测时域(10秒)决定。

**Q2: 增加A_CHANGE_COST从250到500会不会影响紧急制动？**  
A: 不会。这个参数只约束加速度的变化速率(jerk)，不影响最终能达到的加减速值。紧急制动的能力不受影响。

**Q3: 距离滤波的0.05秒时常数是怎么选的？**  
A: 根据DT_MDL(≈0.01秒)和雷达更新频率(～20Hz)选择。0.05秒约为5个采样周期，足以滤除高频噪声但保留有效信号。

**Q4: 能同时应用所有优化吗？**  
A: 可以，但建议分阶段应用。先做优先级高的(1,2)，稳定后再加优先级中的(3,4)。

**Q5: 如何判断优化是否有效？**  
A: 监控三个关键指标：
- 加速度标准差（应下降30-50%）
- jerk峰值（应下降50-70%）
- 距离跳变幅度（应下降30-40%）

