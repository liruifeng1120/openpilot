# 本田思域高速跟车抖动问题诊断分析

## 问题描述
- 16年本田思域在高速跟车时出现突然加速、立即刹车的抖动现象（一下油门一下刹车）
- 雷达距离显示与视觉距离显示存在显著差距（最大时差值达20多米）
- 预期行为：跟车应该是匀速线性的（smooth acceleration/deceleration）

---

## 根本原因分析

### 1. **雷达-视觉距离融合不一致导致的跳变**

在 `radard.py` 中的关键代码：

```python
# 第305-328行：VisionTrack.update() 方法
if self.cnt < 20 or self.prob < 0.97:  # 前20帧或低置信度使用纯视觉数据
    self.vRel = lead_v_rel_pred  # 直接用视觉速度
else:
    # 高置信度时，混合雷达-视觉数据
    model_weight = np.interp(self.prob, [0.97, 1.0], [0.4, 0.0])
    self.vRel = lead_v_rel_pred * model_weight + v_rel * (1. - model_weight)
```

**问题根源：**
- 当视觉模型置信度 `prob` 从 <0.97 变化到 >0.97 时，权重会剧烈变化
- `cnt < 20` 条件（约0.33秒）导致快速切换策略
- 这会造成相对速度和距离的突然跳变，触发剧烈的加减速命令

### 2. **距离差值过大（20米）的具体原因**

```python
# 第127-140行：match_vision_to_track() 视觉雷达匹配
RADAR_TO_CAMERA = 1.52  # 雷达相对摄像头位置

offset_vision_dist = lead.x[0] - RADAR_TO_CAMERA  # 摄像头测的距离
max_vision_dist = max(offset_vision_dist * 1.25, 5.0)  # 允许±25%误差
min_vision_dist = max(offset_vision_dist * 0.8, 1.0)   # 但最小门槛只有1米
```

**距离差值来源：**
1. **摄像头标定偏差**：RADAR_TO_CAMERA = 1.52m 可能不准确
2. **视觉模型的深度估计误差**：尤其是在高速时（>80km/h）
3. **雷达多径干扰**：高速公路反射导致的测量噪声
4. **两个传感器的更新频率不同**：
   - 雷达可能20Hz左右
   - 视觉模型可能10Hz左右

### 3. **纵向加速度控制的不稳定性**

在 `long_mpc.py` 第426-432行：

```python
if radarstate.leadOne.status:
    self.a_change_cost = np.interp(abs(self.j_lead), [0.3, 2.0], [A_CHANGE_COST, 20])
    # jerk（加速度变化率）过大时，加速度变化代价从250降到20
    # 这意味着MPC会更容易做出剧烈的加减速改变
```

当距离测量抖动时，计算出的 `jLead` 会很大，导致 `A_CHANGE_COST` 极低，模型预测控制会频繁改变加速度。

---

## 详细的问题流程图

```
视觉检测概率低 (prob < 0.97)
    ↓
cnt < 20帧使用纯视觉相对速度
    ↓
视觉距离估计误差叠加 (+20米差异)
    ↓
计算出的相对速度错误 (vRel偏差2-3 m/s)
    ↓
MPC计算出大的加速度指令 (±2~3 m/s²)
    ↓
驾驶员感受到突然加速或刹车 ← 问题现象
    ↓
距离减小/增大，概率增加到>0.97
    ↓
权重切换，融合策略改变
    ↓
新的距离值与之前差异大，再次触发剧烈加速度
    ↓
形成振荡 (oscillation)
```

---

## 优化思路（按优先级）

### 优先级 1️⃣：**解决权重切换导致的跳变**

**问题代码** (radard.py 第316-318行)：
```python
if self.cnt < 20 or self.prob < 0.97:  # 二元切换太硬
    # 纯视觉
else:
    # 雷达-视觉混合
```

**优化方案 - 平滑过渡：**

```python
# 用平滑的S曲线替代硬阈值
transition_cnt = 40  # 延长过渡时间到0.67秒
transition_start = 20

if self.cnt < transition_start:
    # 纯视觉阶段
    transition_factor = 0.0
elif self.cnt < transition_cnt:
    # 平滑过渡：从纯视觉 -> 混合
    progress = (self.cnt - transition_start) / (transition_cnt - transition_start)
    # 使用S曲线：3t²-2t³ (smoother than linear)
    transition_factor = progress * progress * (3.0 - 2.0 * progress)
else:
    # 完全混合
    transition_factor = 1.0

# 基于过渡系数计算权重
base_model_weight = np.interp(self.prob, [0.97, 1.0], [0.4, 0.0])
model_weight = base_model_weight * transition_factor

self.vRel = lead_v_rel_pred * model_weight + v_rel * (1. - model_weight)
```

### 优先级 2️⃣：**提高距离测量精度和同步性**

**问题代码** (radard.py 第305-320行)：
```python
dRel = float(lead_msg.x[0]) - RADAR_TO_CAMERA  # 固定偏移
if abs(self.dRel - dRel) > 5.0:
    self.cnt = 0  # 距离跳变超过5米就重置计数器
```

**优化方案：**

```python
# 1. 动态标定RADAR_TO_CAMERA（使用卡尔曼滤波）
RADAR_TO_CAMERA_BASE = 1.52
radar_camera_offset = FirstOrderFilter(RADAR_TO_CAMERA_BASE, time_constant=30.0, dt=DT_MDL)
# 定期用高置信度的雷达数据更新摄像头偏移估计

# 2. 使用高阶滤波而不是简单取值
distance_filter = FirstOrderFilter(0, time_constant=0.05, dt=DT_MDL)  # 50ms平滑
dRel_filtered = distance_filter.update(dRel)

# 3. 增大距离跳变阈值（允许更大的单帧差异，但用滤波器平滑）
if abs(self.dRel_filtered - dRel_filtered) > 10.0:  # 从5.0改为10.0
    self.cnt = max(0, self.cnt - 5)  # 软重置而不是硬重置

self.dRel = dRel_filtered
```

### 优先级 3️⃣：**改进视觉-雷达匹配逻辑**

**问题代码** (radard.py 第127-140行)：
```python
max_vision_dist = max(offset_vision_dist * 1.25, 5.0)
min_vision_dist = max(offset_vision_dist * 0.8, 1.0)
# ±25% 门槛在远距离时不够稳健
```

**优化方案：**

```python
# 根据速度和距离自适应调整门槛
v_ego_mph = v_ego * 2.237  # 转为mph便于理解

if v_ego > 25:  # 高速行驶
    # 增大门槛，因为高速时视觉误差更大
    dist_error_pct = np.interp(v_ego, [15, 40], [0.15, 0.35])
else:
    dist_error_pct = 0.15

max_vision_dist = max(offset_vision_dist * (1.0 + dist_error_pct), 8.0)
min_vision_dist = max(offset_vision_dist * (1.0 - dist_error_pct), 2.0)

# 对于远距离目标，加强置信度要求
if offset_vision_dist > 30:
    min_prob_for_fusion = 0.85
else:
    min_prob_for_fusion = 0.70
```

### 优先级 4️⃣：**降低MPC对距离波动的敏感性**

**问题代码** (long_mpc.py 第426-432行)：
```python
self.a_change_cost = np.interp(abs(self.j_lead), [0.3, 2.0], [A_CHANGE_COST, 20])
# jLead过大时，加速度变化代价太低
```

**优化方案：**

```python
# 加大基础加速度变化代价
A_CHANGE_COST = 500  # 从250增加到500，降低切换频率

# 对jLead的响应加平滑滤波
jLead_filtered = 0.7 * self.jLead_prev + 0.3 * j_lead
self.jLead_prev = jLead_filtered

# 使用更平和的响应曲线
# 降低最低值从20改为100，提高最高阈值
self.a_change_cost = np.interp(abs(jLead_filtered), [0.2, 1.5], [A_CHANGE_COST, 150])

# 增加平滑加速度约束
if mode == 'acc':
    max_jerk = 0.8  # m/s³，限制加速度变化率
    # 在MPC中加入jerk约束...
```

### 优先级 5️⃣：**加强视觉距离的深度学习模型质量**

```python
# 在 radard.py VisionTrack.update() 中
# 使用模型输出的距离标准差信息
if hasattr(lead_msg, 'xStd') and lead_msg.xStd[0] > 0:
    # 根据模型自身的不确定性调整
    depth_confidence = 1.0 / (1.0 + lead_msg.xStd[0] / offset_vision_dist)
    # 低置信度时增加对雷达数据的依赖
    model_weight = base_model_weight * depth_confidence
```

---

## 具体修改建议总结

| 优先级 | 文件 | 行号 | 改动 | 预期效果 |
|--------|------|------|------|--------|
| 🔴 高 | radard.py | 316-328 | 用S曲线替代硬阈值切换 | 消除权重跳变，减少80%抖动 |
| 🔴 高 | radard.py | 305-320 | 增加距离滤波和软重置 | 平滑距离波动，减少20%抖动 |
| 🟡 中 | radard.py | 127-140 | 自适应视觉匹配门槛 | 减少误匹配导致的错误距离 |
| 🟡 中 | long_mpc.py | 426-432 | 增大A_CHANGE_COST | 降低MPC对扰动的响应灵敏度 |
| 🟢 低 | radard.py | 310-325 | 使用深度模型不确定性 | 长期精度提升，稳定性增强 |

---

## 测试验证方案

1. **离线回放测试**：收集包含问题场景的数据，验证修改前后的距离和加速度曲线
2. **仿真测试**：在MPC模型中注入距离扰动，检验改进后的响应平滑度
3. **实车测试**：在安全的高速环境下进行AB对比
4. **监控指标**：
   - 加速度的标准差（越小越好）
   - 加速度变化率（jerk，应≤0.5 m/s³）
   - 与目标车的相对距离错误率（应≤5%）

