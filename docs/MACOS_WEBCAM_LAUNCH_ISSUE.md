# macOS 上 sunnypilot Webcam 模式无法启动问题排查与解决

## 一、问题现象

在 macOS（Darwin）环境下运行以下命令启动 sunnypilot webcam 模拟模式：

```bash
USE_WEBCAM=1 ROAD_CAM=0 system/manager/manager.py
```

表现为：

- 进程启动后只打印两行警告，随后**卡住不动**，无任何输出、不崩溃、不进入主循环。
- 打印的两行警告：
  ```
  uv.lock: needs update
  missing public key: /Users/a1-6/.comma/persist/comma/id_rsa.pub
  ```

> 注：这两条警告本身**不影响启动**，可放心忽略：
> - `uv.lock: needs update` 仅提示虚拟环境与锁文件有轻微偏差。
> - `missing public key` 是 PC 环境缺少设备注册密钥的正常提示，设备会以 `UnregisteredDevice` 身份运行。

## 二、排查过程

### 2.1 确认 `manager_init()` 是卡住点

manager 启动分为两个阶段：

1. `manager_init()` —— 初始化参数、注册、preimport 所有启用的进程模块
2. `manager_thread()` —— 主循环，负责拉起各子进程

通过设置 `PREPAREONLY=1`（该模式下只执行 `manager_init()` 后直接返回）测试，发现**同样卡住**，确认问题出在 `manager_init()` 阶段，而非主循环。

### 2.2 用 `faulthandler` 精确定位卡住代码行

利用 Python 的 `faulthandler.dump_traceback_later()`，在超时后自动 dump 出所有线程的调用堆栈，精确锁定了卡住位置：

```
File "/Users/a1-6/sunnypilot-pc/tools/bodyteleop/web.py", line 9 in <module>   # import pyaudio
File ".../site-packages/pyaudio/__init__.py", line 111 in <module>              # import pyaudio._portaudio as pa
File ".../openpilot/system/manager/process.py", line 214 in prepare             # importlib.import_module()
File ".../openpilot/system/manager/manager.py", line 114 in manager_init
```

### 2.3 根因确认

**卡住点**：`manager_init()` 中的进程 preimport 环节（`process.py` 的 `prepare()`）在导入 `tools/bodyteleop/web.py` 时，其第 9 行 `import pyaudio` 触发挂起。

**深层原因**：`pyaudio` 在加载其 C 扩展 `pyaudio._portaudio`（`pyaudio/__init__.py` 第 111 行）时，会在 macOS 上初始化 CoreAudio/PortAudio 音频栈，**在多进程（multiprocessing fork）context 下永久挂起**。

**为何单独测试正常**：直接 `python3 -c "import pyaudio"` 可以秒过；只有在 manager 完整的、经过 fork 与多线程初始化的环境里，`pyaudio._portaudio` 的初始化才会死锁。

**影响范围**：`webjoystick` 进程（`tools/bodyteleop/web.py`）是 bodyteleop（车身遥控）功能，仅在 `notcar`（非真实车辆）模式下运行，**webcam 模拟驾驶场景完全用不到**，禁用它是安全的。

## 三、解决方案

修改 `system/manager/process_config.py`，为 `webjoystick` 进程加上 `enabled=not PC`，使其在 PC 环境下不参与 preimport：

```python
# 修改前
PythonProcess("webjoystick", "tools.bodyteleop.web", notcar),

# 修改后
PythonProcess("webjoystick", "tools.bodyteleop.web", notcar, enabled=not PC),
```

**原理**：`PythonProcess.prepare()` 只在 `self.enabled` 为 True 时才执行 preimport（见 `process.py`）。通过 `enabled=not PC` 在 PC 上禁用该进程，从而跳过对 `tools.bodyteleop.web` 的导入，绕开 `pyaudio` 挂起问题。

**验证**：修改后重新执行 `manager_init()`，输出正常完成，不再卡住：

```
=== calling manager_init ===
=== manager_init done, PREPAREONLY OK ===
```

## 四、最终启动命令

```bash
cd /Users/a1-6/sunnypilot-pc
source .venv/bin/activate
USE_WEBCAM=1 ROAD_CAM=0 system/manager/manager.py
```

## 五、其他可忽略的提示

| 提示 | 说明 | 处理 |
|------|------|------|
| `uv.lock: needs update` | 虚拟环境与锁文件轻微偏差 | 忽略；如需消除可运行 `uv sync` |
| `missing public key` | PC 缺少设备注册公钥 | 忽略；设备以 `UnregisteredDevice` 运行 |
| `system/manager/process_config.py: needs update` | 本地修改触发的提示，无害 | 忽略 |

## 六、备注

- 若未来需在**真实车辆**上使用 bodyteleop 功能，可移除 `enabled=not PC` 限制（或仅在非 PC 设备上启用）。
- 本问题仅影响 **macOS** 平台；Linux（Ubuntu）上 `pyaudio` 的 `_portaudio` 初始化行为不同，不会触发此挂起。
- 排查工具备忘：`faulthandler.dump_traceback_later(seconds, exit=True)` 是定位"无输出卡死"类问题的利器，可在超时后 dump 出精确调用栈。
