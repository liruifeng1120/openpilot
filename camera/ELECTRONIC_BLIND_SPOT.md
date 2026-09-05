# 电子后视镜 / 盲区检测 优化文档

> 本文档记录 openpilot (cp-pc) 电子后视镜/盲区检测的需求分析、架构改造与实现细节，
> 供后期更换机器/迁移环境时调试参考。

---

## 一、需求背景

### 1.1 原始痛点

原 `cam_blindspot` 程序存在以下问题：

| 问题 | 根因 | 影响 |
|------|------|------|
| 画面卡死 | 全局唯一 `Ort::Session`，两个 inference 线程并发调用 `session->Run()`，ONNX Runtime 非线程安全 | 性能急剧下降甚至死锁 |
| CPU 占用高 | YOLOv5s 在 CPU 上单帧 200-500ms，2 路并行推理 CPU 过载 | 风扇狂转、系统卡顿 |
| 花屏/崩溃 | `shared_images[cam_id] = img;`（不 clone），display 线程读取时 inference 线程覆盖同一块 Mat 内存 | 随机崩溃 |
| 无电子后视镜 | 摄像头画面仅本地 `cv::imshow` 显示，未集成到 OP UI | 无法在车机屏幕看到盲区画面 |

### 1.2 本次改造需求

1. **GPU 推理** — 优先 ROCm (AMD)，次选 CUDA (NVIDIA)，自动 fallback CPU
2. **VisionIPC 推流** — USB 摄像头画面通过 VisionIPC 推送给 UI 显示（电子后视镜）
3. **分辨率不降** — 保持 640x480，帧率 20fps，避免死画面
4. **修复线程安全** — 消除并发访问导致的卡死
5. **按转向灯显示** — 打左转向灯显示左盲区画面，打右转向灯显示右盲区画面

---

## 二、系统架构

### 2.1 整体数据流

```
USB摄像头(640x480 MJPEG, 20fps)
   │
   ├─ capture_from_camera 线程 (V4L2 mmap)
   │
   ▼
帧队列 (per-cam)
   │
   ├─ inference_thread (per-cam, 独立 Ort::Session)
   │     ├─ YOLOv5s 推理 (GPU: ROCm/CUDA / CPU fallback)
   │     ├─ ROI 过滤 + NMS + 消抖
   │     ├─ 绘制检测框/ROI
   │     ├─ 更新 shared_images (img.clone(), 线程安全)
   │     └─ publish_to_vipc() → VisionIpcServer
   │                                  │
   │                                  ▼
   │                          VisionIPC 共享内存 (NV12 YUV)
   │                                  │
   │                                  ▼
   │                     UI CameraWidget (visionipc_client)
   │                     ├─ VISION_STREAM_BLIND_LEFT
   │                     └─ VISION_STREAM_BLIND_RIGHT
   │
   ├─ lane_check_thread → PubMaster("amapNavi") → cereal 总线
   │                       (leftBlind/rightBlind 盲区报警标志)
   │
   ├─ udp_comm_thread → UDP 广播给 OP
   │
   ├─ dashcam_management_thread (行车记录仪)
   │
   └─ display_loop (cv::imshow 本地调试窗口)
```

### 2.2 关键组件

| 组件 | 位置 | 作用 |
|------|------|------|
| `cam_blindspot` | `camera/cam_blindspot.cpp` | 主程序：USB采集 + YOLO推理 + VisionIPC推流 |
| `VisionIpcServer` | `msgq_repo/msgq/visionipc/` | 共享内存帧传输服务端 |
| `CameraWidget` | `selfdrive/ui/qt/widgets/cameraview.cc` | UI 端接收并 OpenGL 渲染 YUV 画面 |
| `OnroadWindow` | `selfdrive/ui/qt/onroad/onroad_home.cc` | 行驶界面，按转向灯动态显示盲区画面 |
| `AmapNavi` | `cereal/custom.capnp` | 盲区报警消息（leftBlind/rightBlind）|

---

## 三、代码改动清单

### 3.1 新增 VisionIPC 流类型

**文件**: `msgq_repo/msgq/visionipc/visionbuf.h`

```c
enum VisionStreamType {
  VISION_STREAM_ROAD,
  VISION_STREAM_DRIVER,
  VISION_STREAM_WIDE_ROAD,
  VISION_STREAM_MAP,
  VISION_STREAM_BLIND_LEFT,    // 新增：左盲区画面
  VISION_STREAM_BLIND_RIGHT,   // 新增：右盲区画面
  VISION_STREAM_MAX,
};
```

### 3.2 放宽 VisionIPC 服务名限制

**文件**: `msgq_repo/msgq/visionipc/visionipc_server.cc`

zmq 模式下原仅允许 `camerad`/`navd`，新增 `cam_blindspot`：

```cpp
assert(name == "camerad" || name == "navd" || name == "cam_blindspot");
```

### 3.3 同步 Python 绑定

**文件**: `msgq_repo/msgq/visionipc/visionipc_pyx.pyx`

```python
cpdef enum VisionStreamType:
  VISION_STREAM_ROAD
  VISION_STREAM_DRIVER
  VISION_STREAM_WIDE_ROAD
  VISION_STREAM_MAP
  VISION_STREAM_BLIND_LEFT    # 新增
  VISION_STREAM_BLIND_RIGHT   # 新增
```

### 3.4 cam_blindspot.cpp 核心重构

**文件**: `camera/cam_blindspot.cpp`

#### 3.4.1 GPU 推理

新增 `configure_execution_provider()`，优先级：ROCm → CUDA → CPU：

```cpp
static void configure_execution_provider(Ort::SessionOptions& opts) {
    // 1. ROCm (AMD GPU)
    try {
        OrtROCMProviderOptions rocm_opts;
        opts.AppendExecutionProvider_ROCM(rocm_opts);
        g_backend = InferenceBackend::ROCM;
        return;
    } catch (...) { /* fallback */ }

    // 2. CUDA (NVIDIA GPU)
    try {
        OrtCUDAProviderOptions cuda_opts;
        opts.AppendExecutionProvider_CUDA(cuda_opts);
        g_backend = InferenceBackend::CUDA;
        return;
    } catch (...) { /* fallback */ }

    // 3. CPU
    opts.SetIntraOpNumThreads(2);
}
```

#### 3.4.2 每 cam 独立 Session（修复线程安全）

```cpp
std::vector<std::unique_ptr<Ort::Session>> g_sessions;

Ort::Session* get_session_for_cam(int cam_id) {
    // 双重检查锁，懒加载每个 cam 的独立 session
    std::lock_guard<std::mutex> lock(g_session_init_mutex);
    // ... 创建独立 session，调用 configure_execution_provider
}
```

inference_thread 调用：

```cpp
auto result = detect_cars(img, get_session_for_cam(cam_id));
```

#### 3.4.3 VisionIPC 推流

```cpp
void publish_to_vipc(int cam_id, const cv::Mat& bgr_frame) {
    // 根据 camera_sign 选择流类型
    VisionStreamType stream_type = (camera_sign[cam_id] == 0)
        ? VISION_STREAM_BLIND_LEFT
        : VISION_STREAM_BLIND_RIGHT;

    VisionBuf* buf = g_vipc_server->get_buffer(stream_type);

    // BGR → YUV I420 → NV12 (CameraWidget 期望的格式)
    cv::cvtColor(resized, yuv_i420, cv::COLOR_BGR2YUV_I420);
    memcpy(dst, yuv_i420.data, y_size);  // Y 平面
    // UV 交织：I420 [U][V] → NV12 [UV][UV]...
    for (int i = 0; i < uv_pixels; i++) {
        uv_dst[2*i]     = u_plane[i];
        uv_dst[2*i + 1] = v_plane[i];
    }

    g_vipc_server->send(buf, &extra, false);
}
```

#### 3.4.4 shared_images clone 修复

```cpp
// 修复前（线程不安全）：
shared_images[cam_id] = img;

// 修复后：
shared_images[cam_id] = img.clone();
```

#### 3.4.5 main 函数初始化 VisionIpcServer

```cpp
g_vipc_server = new VisionIpcServer("cam_blindspot");
g_vipc_server->create_buffers(VISION_STREAM_BLIND_LEFT, 4, 640, 480);
g_vipc_server->create_buffers(VISION_STREAM_BLIND_RIGHT, 4, 640, 480);
g_vipc_server->start_listener();
```

### 3.5 UI 按转向灯动态显示

**文件**: `selfdrive/ui/qt/onroad/onroad_home.h`

新增成员变量：

```cpp
CameraWidget *blind_left = nullptr;
CameraWidget *blind_right = nullptr;
```

**文件**: `selfdrive/ui/qt/onroad/onroad_home.cc`

构造函数创建（默认隐藏）：

```cpp
blind_left = new CameraWidget("cam_blindspot", VISION_STREAM_BLIND_LEFT, this);
blind_right = new CameraWidget("cam_blindspot", VISION_STREAM_BLIND_RIGHT, this);
blind_left->setFixedWidth(width() / 6);
blind_right->setFixedWidth(width() / 6);
blind_left->setVisible(false);
blind_right->setVisible(false);
split->insertWidget(0, blind_left);   // 左侧
split->addWidget(blind_right);         // 右侧
```

`updateState()` 按转向灯切换可见性：

```cpp
auto car_state = sm["carState"].getCarState();
bool left_blink = car_state.getLeftBlinker();
bool right_blink = car_state.getRightBlinker();

// 兼容 carrot.cc 导航转向提示
if (sm.alive("carrotMan")) {
    auto carrot = sm["carrotMan"].getCarrotMan();
    std::string atc = carrot.getAtcType();
    if (atc == "fork left" || atc == "turn left" || atc == "atc left") left_blink = true;
    if (atc == "fork right" || atc == "turn right" || atc == "atc right") right_blink = true;
}

blind_left->setVisible(left_blink);
blind_right->setVisible(right_blink);
```

---

## 四、编译与运行

### 4.1 依赖检查

```bash
# ONNX Runtime (已自带在 third_party/onnxruntime/)
ls third_party/onnxruntime/lib/libonnxruntime.so.1.23.0

# GPU 驱动（二选一，都没有则自动 fallback CPU）
# AMD ROCm:
rocm-smi 2>/dev/null && echo "ROCm OK"
# NVIDIA CUDA:
nvidia-smi 2>/dev/null && echo "CUDA OK"
```

### 4.2 编译

```bash
cd /home/ubuntu/cp-pc

# 1. 先编译 msgq（因为改了 visionbuf.h / visionipc_pyx.pyx）
scons -j$(nproc) msgq

# 2. 编译 cam_blindspot
scons -j$(nproc) camera/cam_blindspot

# 3. 编译 UI
scons -j$(nproc) selfdrive/ui
```

> **注意**: 如果 msgq 是 Python cython 扩展，可能需要重新编译：
> ```bash
> scons -j$(nproc) cereal/messaging
> ```

### 4.3 运行

#### 启动盲区检测服务

```bash
cd /home/ubuntu/cp-pc/camera
./cam_blindspot
```

日志会显示当前推理后端：
```
[ORT] Using ROCm execution provider (AMD GPU)   # 或 CUDA / CPU
[ORT] Created session for cam_id=0 backend=ROCm
[ORT] Created session for cam_id=1 backend=ROCm
[VIPC] Created VISION_STREAM_BLIND_LEFT 640x480
[VIPC] Created VISION_STREAM_BLIND_RIGHT 640x480
[VIPC] Server 'cam_blindspot' started, waiting for UI to connect...
```

#### 启动 UI

```bash
cd /home/ubuntu/cp-pc
./selfdrive/ui/ui
```

- 默认盲区画面隐藏
- **打左转向灯** → 左侧自动弹出左盲区画面
- **打右转向灯** → 右侧自动弹出右盲区画面
- 转向灯关闭 → 画面自动隐藏

#### 本地调试（不需要 UI）

`cam_blindspot` 默认开启 `cv::imshow` 本地窗口（`camera.json` 的 `show_video: true`），
可按 `ESC` 退出，按 `L`/`R` 切换编辑左/右盲区 ROI，按 `N` 切换摄像头。

---

## 五、配置说明

### 5.1 camera.json

**文件**: `camera/camera.json`

```json
{
  "debug": false,
  "raw_conf_threshold": 0.5,
  "nms_conf_threshold": 0.5,
  "nms_threshold": 0.5,
  "show_video": true,
  "single_window": false,
  "dashcam": {
    "enabled": false,
    "record_fps": 20,
    "save_dir": "/home/$LOGNAME/ajouatom-pc/camera/dashcam_videos",
    "segment_duration": 300,
    "codec": "MJPG",
    "max_disk_usage_gb": 50
  },
  "cameras": [
    {
      "device": "/dev/v4l/by-path/pci-0000:c6:00.4-usb-0:1.3:1.0-video-index0",
      "sign": 1,
      "car_detect": 1
    },
    {
      "device": "/dev/v4l/by-path/pci-0000:c6:00.4-usb-0:1.4:1.0-video-index0",
      "sign": 1,
      "car_detect": 1
    }
  ]
}
```

| 字段 | 说明 |
|------|------|
| `sign` | 0=左盲区摄像头, 1=右盲区摄像头（决定推送到哪个 VIPC 流）|
| `car_detect` | 1=开启YOLO检测, 0=仅显示画面 |
| `raw_conf_threshold` | 宽松阈值，用于画框（建议 0.25-0.5）|
| `nms_conf_threshold` | 严格阈值，用于NMS判断盲区是否有车（建议 0.45-0.5）|
| `show_video` | true=开启本地 cv::imshow 调试窗口 |
| `single_window` | true=所有摄像头合并到一个窗口显示 |

### 5.2 rois.txt

**文件**: `camera/rois.txt`

每行一个多边形 ROI，每两个多边形对应一个摄像头的左右盲区：

```
# cam0 左盲区多边形顶点: x1 y1 x2 y2 ...
83 1 64 476 637 478 636 331 252 125 226 0
# cam0 右盲区多边形顶点
549 65
# cam1 左盲区多边形顶点
94 52
# cam1 右盲区多边形顶点
556 1 561 476 5 476 1 356 401 109 415 7
```

编辑方法：运行 `cam_blindspot`，在画面窗口中鼠标左键点击添加顶点，拖动顶点移动，
右键删除顶点。每 0.5 秒自动保存到 `rois.txt`。

### 5.3 摄像头设备路径

USB 摄像头设备路径使用 `/dev/v4l/by-path/` 而非 `/dev/video0`，
这样即使 USB 端口顺序变化也能稳定识别。更换机器后需重新查询：

```bash
# 列出所有 USB 摄像头
ls -l /dev/v4l/by-path/

# 或用 v4l2-ctl
v4l2-ctl --list-devices
```

---

## 六、调试指南

### 6.1 GPU 推理验证

```bash
# 查看程序实际使用的后端
./cam_blindspot 2>&1 | grep "ORT"
# 期望输出:
# [ORT] Using ROCm execution provider (AMD GPU)
# 或 [ORT] Using CUDA execution provider (NVIDIA GPU)
# 或 [ORT] Using CPU execution provider (fallback)
```

如果 GPU 可用但程序用了 CPU，检查：
- ROCm: `ls /dev/kfd` 和 `ls /dev/dri/render*` 是否存在，用户是否在 `render`/`video` 组
- CUDA: `echo $LD_LIBRARY_PATH` 是否包含 CUDA lib 路径

### 6.2 VisionIPC 推流验证

```bash
# 检查 VisionIPC 服务是否监听
ls -l /tmp/*cam_blindspot*

# 用 Python 客户端测试接收
python3 -c "
from msgq.visionipc import VisionIpcClient, VisionStreamType
c = VisionIpcClient('cam_blindspot', VisionStreamType.VISION_STREAM_BLIND_LEFT, True)
print('connected:', c.connect(True))
print('width:', c.width, 'height:', c.height)
buf = c.recv()
print('recv buf:', buf is not None)
"
```

### 6.3 UI 不显示盲区画面排查

1. **确认转向灯信号**：
   ```bash
   # 实时查看 carState 中的 blinker
   python3 -c "
   from cereal.messaging import SubMaster
   sm = SubMaster(['carState'])
   while True:
     sm.update(100)
     if sm.updated['carState']:
       cs = sm['carState']
       print(f'L={cs.leftBlinker} R={cs.rightBlinker}')
   "
   ```

2. **确认 VisionIPC 流可用**：
   ```bash
   python3 -c "
   from msgq.visionipc import VisionIpcClient
   streams = VisionIpcClient.available_streams('cam_blindspot')
   print('available streams:', streams)
   "
   ```

3. **UI 日志**：CameraWidget 连接成功会打印 `vipc_connected`

### 6.4 性能调优

| 现象 | 排查 | 解决 |
|------|------|------|
| 推理慢 | 检查 `g_backend` 是否为 GPU | 安装 GPU 驱动 |
| 画面卡顿 | `top -H -p $(pgrep cam_blindspot)` 看 CPU | 换 yolov5n.onnx（参数量 1/4）|
| 内存涨 | `frame_queues` 是否堆积 | 已在 capture 线程做 20fps 限速 |
| GPU 利用率低 | `rocm-smi` / `nvidia-smi` | 提高 capture 帧率或增加摄像头 |

### 6.5 换 yolov5n 模型

```bash
cd /home/ubuntu/cp-pc/camera
# 备份原模型
mv yolov5s.onnx yolov5s.onnx.bak
# 下载/放置 yolov5n.onnx
# 代码无需改动，模型路径硬编码为 "yolov5s.onnx"，需重命名或改代码:
# sed -i 's/yolov5s.onnx/yolov5n.onnx/g' cam_blindspot.cpp
```

---

## 七、性能预期

| 后端 | 单帧推理 | 2路20fps CPU占用 | 稳定性 |
|------|----------|-------------------|--------|
| 原方案 (CPU 单 session 并发) | 200-500ms | ~200%+ 经常卡死 | 差 |
| ROCm (AMD GPU) | 15-30ms | ~20% | 优 |
| CUDA (NVIDIA GPU) | 10-25ms | ~15% | 优 |
| CPU fallback (独立 session) | 150-300ms | ~100% | 良 |

---

## 八、消息总线接口

### 8.1 AmapNavi（盲区报警）

**定义**: `cereal/custom.capnp`

```capnp
struct AmapNavi @0xaedffd8f31e7b55d {
    leftBlind @0 : Int32;   # 1=左盲区有车, 0=安全
    rightBlind @1 : Int32;  # 1=右盲区有车, 0=安全
}
```

**发布**: `cam_blindspot.cpp` 的 `lane_check_thread` 每 100ms 发布一次

**消费**:
- `selfdrive/ui/carrot.cc:1418` — UI 渲染 BSD 警告（0.5s hold）
- `selfdrive/controls/lib/desire_helper.py:188` — 变道决策参考
- `selfdrive/selfdrived/selfdrived.py:264` — 变道方向限制

### 8.2 UDP 通信（与 OP 联动）

```
cam_blindspot → UDP 4211 (广播) → OP
OP → UDP 4210 (响应) → cam_blindspot
```

JSON 格式：
```json
{
  "resp": "cam_blind",
  "device": "camera",
  "timeout": false,
  "ip": "192.168.1.100",
  "port": 4210,
  "version": "1.0.02",
  "left_blind": false,
  "right_blind": true,
  "detect_side": 3
}
```

---

## 九、更换机器迁移清单

更换机器时按以下步骤操作：

- [ ] 1. 安装 GPU 驱动（AMD ROCm 6.x 或 NVIDIA CUDA 12.x）
- [ ] 2. 查询 USB 摄像头设备路径：`ls /dev/v4l/by-path/`
- [ ] 3. 更新 `camera/camera.json` 中的 `device` 路径
- [ ] 4. 确保 `yolov5s.onnx` 模型文件在 `camera/` 目录
- [ ] 5. 编译：`scons msgq && scons camera/cam_blindspot && scons selfdrive/ui`
- [ ] 6. 运行 `./camera/cam_blindspot`，确认日志显示 GPU 后端 + VIPC 启动
- [ ] 7. 运行 `./selfdrive/ui/ui`，打转向灯确认盲区画面弹出
- [ ] 8. 重新绘制 ROI（如摄像头安装位置变化）：运行程序，鼠标编辑多边形
- [ ] 9. 验证盲区报警：在盲区放置车辆，确认 UI 出现 BSD 警告

---

## 十、已知限制与后续优化

1. **静止车辆检测弱** — YOLO 对静止车辆识别率一般，可考虑结合运动检测（MOG2 背景差分）
2. **VisionIPC 仅支持 YUV NV12** — BGR→YUV 转换有 CPU 开销，未来可考虑 OpenCL 加速
3. **转向灯复位闪烁** — 部分车型拨一下闪 3 次自动复位，盲区画面会跟随闪烁。如需保持显示，
   可在 `onroad_home.cc` 的 `updateState` 加 hold 计时器（参考 `carrot.cc:1412` 的 `CAMERA_HOLD_S`）

### 已修复：打灯后盲区画面不显示 / 只在边框闪烁

**现象**：打转向灯时 UI 主画面不显示盲区画面，只在屏幕左右边框看到很小的橙色闪烁条带。

**根因**：
1. `onroad_home.cc` 构造函数中 `blind_left->setFixedWidth(width() / 6)`，但 `OnroadWindow`
   是 `QOpenGLWidget`，构造时尚未显示，`width()` 返回 0，导致 `setFixedWidth(0)`，画面宽度为 0
   完全看不到。用户看到的"闪烁"是 `carrot.cc` 边框绘制 `ui_draw_border` 里的转向灯橙色条带
   (50×190)，并非盲区画面。
2. `blind_left/blind_right` 原本加在 `split` (QHBoxLayout) 内部，而 `updateState` 中
   `split->setDirection(QBoxLayout::RightToLeft)`（默认 `map_on_left=false`）会把
   `blind_left` 翻到右侧、`blind_right` 翻到左侧，左右盲区位置颠倒。

**修复** (`selfdrive/ui/qt/onroad/onroad_home.{h,cc}`)：
- 构造函数中 `blind` 先设安全初始宽度 `setFixedWidth(200)`，不再用 `width()/6`。
- 新增 `OnroadWindow::resizeEvent`，在窗口尺寸确定后按 `width()/6` 设置盲区画面宽度。
- 重构布局：`blind_left/blind_right` 移到 `split` 外层的 `outer_layout`，位置固定为
  最左/最右，不再受 `split->setDirection` 翻转影响。`split` 内部仅保留 `nvg` 及
  `DUAL_CAMERA_VIEW`/`MAP_RENDER_VIEW` 调试画面。

### 已优化：画中画悬浮 + 水平镜像 + 无 ROI 框

**需求**：
1. 盲区画面需要左右镜像（电子后视镜效果，画面方向与车内后视镜一致）
2. UI 显示的画面不需要 ROI 框/检测框
3. 改为画中画方式，直接悬浮在 UI 左上角和右上角，不再占用侧边栏布局空间

**实现**：

1. **水平镜像** (`camera/cam_blindspot.cpp` `publish_to_vipc`)：
   推流前对画面做 `cv::flip(resized, resized, 1)`（水平翻转），使画面方向与车内后视镜一致。

2. **无 ROI 框** (`camera/cam_blindspot.cpp` `inference_thread`)：
   在绘制 ROI/检测框之前克隆一份纯净画面 `clean_img = img.clone()`，推流用 `clean_img`，
   本地调试窗口和行车记录仪仍用带框的 `img`。

3. **画中画悬浮** (`selfdrive/ui/qt/onroad/onroad_home.cc`)：
   - 盲区画面不再加入任何布局，作为 `OnroadWindow` 的悬浮子窗口。
   - `resizeEvent` 中按 `width()/6` 计算 PiP 尺寸（4:3 比例），左盲区定位到左上角
     `(UI_BORDER_SIZE, UI_BORDER_SIZE)`，右盲区定位到右上角
     `(width()-pip_w-UI_BORDER_SIZE, UI_BORDER_SIZE)`。
   - 调用 `raise()` 确保悬浮在主画面之上。
   - 打转向灯时 `setVisible(true)` 显示，关闭时隐藏。
4. **无夜间增强** — USB 摄像头夜间画质差，可考虑红外摄像头或图像增强算法
5. **盲区画面尺寸固定** — 当前为屏宽 1/6，可改为按车速/转向角动态调整大小

---

*文档版本: 1.0*
*更新日期: 2026-06-20*
*对应代码分支: CD-210*
