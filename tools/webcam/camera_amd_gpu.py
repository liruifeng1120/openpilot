#!/usr/bin/env python3
"""
AMD GPU硬件加速摄像头解码器 - 优化版
专门针对MJPG解码优化，确保关键参数（分辨率、帧率）得到保持
"""

import av
import cv2
import numpy as np
import threading
import queue
import os
import subprocess
import logging
import time
from concurrent.futures import ThreadPoolExecutor
from typing import Generator, Optional, Dict, Tuple

# 配置日志
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class AMDGPUDecoder:
    """AMD显卡硬件解码器 - 专门针对MJPEG优化"""

    def __init__(self, width: int = 2592, height: int = 1944, target_fps: float = 20.0):
        self.width = width
        self.height = height
        self.target_fps = target_fps
        self.decoder_ctx = None
        self.hw_device_ctx = None
        self.vaapi_device = None

        # 初始化硬件解码器
        self.hw_initialized = self._init_amd_decoder()

        # 性能统计
        self.decode_count = 0
        self.decode_time_total = 0
        self.last_report_time = time.time()

        logger.info(f"AMD GPU解码器: {width}x{height}@{target_fps}fps, "
                   f"硬件加速: {'✓' if self.hw_initialized else '✗'}")

    def _init_amd_decoder(self) -> bool:
        """初始化AMD硬件解码器"""
        try:
            # 检查AMD设备和驱动
            amd_info = self._detect_amd_hardware()
            if not amd_info['available']:
                logger.warning(f"AMD硬件不可用: {amd_info['reason']}")
                return False

            # 设置AMD优化环境变量
            self._setup_amd_environment()

            # 创建VA-API硬件解码上下文
            try:
                # 创建MJPEG解码器
                self.decoder_ctx = av.codec.CodecContext.create('mjpeg', 'r')

                # 配置VA-API硬件加速
                self.decoder_ctx.options = {
                    'hwaccel': 'vaapi',
                    'hwaccel_device': amd_info['device'],
                    'hwaccel_output_format': 'vaapi',
                    'extra_hw_frames': '8',
                    'async_depth': '4'  # AMD异步解码优化
                }

                # 设置解码器参数
                self.decoder_ctx.thread_count = 2  # AMD GPU适合较少线程
                self.decoder_ctx.thread_type = 'AUTO'

                logger.info(f"AMD VA-API硬件解码器初始化成功 - 设备: {amd_info['device']}")
                return True

            except Exception as e:
                logger.warning(f"VA-API硬件解码器创建失败: {e}")
                return False

        except Exception as e:
            logger.error(f"AMD硬件解码器初始化失败: {e}")
            return False

    def _detect_amd_hardware(self) -> Dict[str, any]:
        """检测AMD硬件和驱动支持"""
        result = {
            'available': False,
            'device': None,
            'driver': None,
            'mjpeg_support': False,
            'reason': ''
        }

        try:
            # 检查DRI设备
            for device in ['/dev/dri/renderD128', '/dev/dri/renderD129', '/dev/dri/card0']:
                if os.path.exists(device) and os.access(device, os.R_OK | os.W_OK):
                    result['device'] = device
                    break

            if not result['device']:
                result['reason'] = '未找到可访问的AMD DRI设备'
                return result

            # 检查VA-API和AMD驱动
            try:
                vainfo_result = subprocess.run(
                    ['vainfo', '--display', 'drm', '--device', result['device']],
                    capture_output=True, text=True, timeout=5
                )

                if vainfo_result.returncode == 0:
                    output = vainfo_result.stdout.lower()

                    # 检查AMD驱动
                    amd_keywords = ['amd', 'radeon', 'radv', 'mesa gallium']
                    if any(keyword in output for keyword in amd_keywords):
                        result['driver'] = 'AMD'

                        # 检查MJPEG解码支持
                        if 'mjpeg' in output or 'motion jpeg' in output:
                            result['mjpeg_support'] = True
                            result['available'] = True
                            logger.info("AMD VA-API MJPEG硬件解码支持确认")
                        else:
                            result['reason'] = 'AMD驱动不支持MJPEG硬件解码'
                    else:
                        result['reason'] = '未检测到AMD VA-API驱动'
                else:
                    result['reason'] = f'vainfo执行失败: {vainfo_result.stderr}'

            except subprocess.TimeoutExpired:
                result['reason'] = 'vainfo命令执行超时'
            except FileNotFoundError:
                result['reason'] = 'vainfo命令未找到，请安装mesa-va-drivers'

        except Exception as e:
            result['reason'] = f'AMD硬件检测失败: {e}'

        return result

    def _setup_amd_environment(self):
        """设置AMD优化环境变量"""
        # AMD Mesa驱动优化
        os.environ['LIBVA_DRIVER_NAME'] = 'radeonsi'
        os.environ['AMD_VULKAN_ICD'] = 'RADV'
        os.environ['MESA_GL_VERSION_OVERRIDE'] = '4.6'

        # AMD性能优化
        os.environ['AMD_DISABLE_PERFCOUNTERS'] = '1'  # 减少性能计数器开销
        os.environ['RADV_PERFTEST'] = 'aco'  # 启用ACO编译器

        # VA-API优化
        os.environ['LIBVA_MESSAGING_LEVEL'] = '1'  # 减少日志输出

        logger.debug("AMD环境变量优化完成")

    def decode_frame(self, mjpeg_data: bytes) -> Optional[np.ndarray]:
        """使用AMD GPU解码MJPEG帧"""
        decode_start = time.time()

        try:
            if self.hw_initialized and self.decoder_ctx:
                # AMD硬件解码路径
                packet = av.Packet(mjpeg_data)

                try:
                    frames = self.decoder_ctx.decode(packet)
                    if frames:
                        frame = frames[0]

                        # 验证帧尺寸
                        if frame.width != self.width or frame.height != self.height:
                            logger.warning(f"帧尺寸不匹配: 期望{self.width}x{self.height}, "
                                         f"实际{frame.width}x{frame.height}")

                        # 从VA-API surface传输到CPU并转换格式
                        result_frame = self._process_vaapi_frame(frame)

                        # 性能统计
                        self._update_decode_stats(decode_start)

                        return result_frame

                except Exception as decode_error:
                    logger.debug(f"硬件解码失败，降级到CPU: {decode_error}")
                    return self._cpu_decode_fallback(mjpeg_data)
            else:
                # CPU解码后备方案
                return self._cpu_decode_fallback(mjpeg_data)

        except Exception as e:
            logger.error(f"AMD解码失败: {e}")
            return None

    def _process_vaapi_frame(self, frame) -> np.ndarray:
        """处理VA-API帧，转换为NV12格式"""
        try:
            # 检查是否为VA-API surface
            if hasattr(frame, 'format') and 'vaapi' in str(frame.format):
                # 硬件帧，需要传输到CPU
                cpu_frame = frame.to_cpu()
                nv12_frame = cpu_frame.reformat(format='nv12')
            else:
                # 软件帧，直接转换
                nv12_frame = frame.reformat(format='nv12')

            return nv12_frame.to_ndarray()

        except Exception as e:
            logger.warning(f"VA-API帧处理失败: {e}")
            # 尝试直接转换
            try:
                return frame.to_ndarray(format='nv12')
            except:
                raise

    def _cpu_decode_fallback(self, mjpeg_data: bytes) -> Optional[np.ndarray]:
        """CPU解码后备方案"""
        try:
            # 使用OpenCV进行CPU解码
            nparr = np.frombuffer(mjpeg_data, np.uint8)
            img = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
            if img is not None:
                return self._bgr_to_nv12(img)
        except Exception as e:
            logger.error(f"CPU后备解码失败: {e}")
        return None

    @staticmethod
    def _bgr_to_nv12(bgr_frame: np.ndarray) -> np.ndarray:
        """BGR转NV12格式"""
        frame = av.VideoFrame.from_ndarray(bgr_frame, format='bgr24')
        return frame.reformat(format='nv12').to_ndarray()

    def _update_decode_stats(self, start_time: float):
        """更新解码性能统计"""
        self.decode_count += 1
        self.decode_time_total += time.time() - start_time

        # 每100帧报告一次性能
        if self.decode_count % 100 == 0:
            current_time = time.time()
            duration = current_time - self.last_report_time

            if duration > 0:
                avg_decode_time = self.decode_time_total / self.decode_count * 1000
                fps = 100 / duration

                logger.info(f"AMD解码性能: {fps:.1f}fps, 平均解码时间: {avg_decode_time:.2f}ms")

                self.last_report_time = current_time
                self.decode_time_total = 0
                self.decode_count = 0


class CameraAMDOptimized:
    """AMD GPU优化的高性能摄像头类"""

    def __init__(self, cam_type_state, stream_type, camera_id,
                 target_width: int = 2592, target_height: int = 1944,
                 target_fps: float = 20.0, num_workers: int = 2, max_queue_size: int = 3):
        try:
            camera_id = int(camera_id)
        except ValueError:
            pass

        self.cam_type_state = cam_type_state
        self.stream_type = stream_type
        self.cur_frame_id = 0

        # 关键参数 - 用户指定的分辨率和帧率
        self.target_width = target_width
        self.target_height = target_height
        self.target_fps = target_fps

        logger.info(f"目标参数: {target_width}x{target_height}@{target_fps}fps")

        # 初始化摄像头
        self.cap = cv2.VideoCapture(camera_id)
        if not self.cap.isOpened():
            raise IOError(f"无法打开摄像头设备 {camera_id}")

        # 配置摄像头为目标参数
        self._configure_camera_parameters()

        # 验证实际获得的参数
        self.W = int(self.cap.get(cv2.CAP_PROP_FRAME_WIDTH))
        self.H = int(self.cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
        self.fps = self.cap.get(cv2.CAP_PROP_FPS)

        # 检查参数是否符合要求
        self._validate_camera_parameters()

        # 初始化AMD GPU解码器
        self.amd_decoder = AMDGPUDecoder(self.W, self.H, self.fps)

        # 优化的多线程配置 (AMD GPU用较少线程更高效)
        self.num_workers = num_workers
        self.frame_queue = queue.Queue(maxsize=max_queue_size)
        self.output_queue = queue.Queue(maxsize=max_queue_size)
        self.stop_event = threading.Event()

        # 性能监控
        self.frame_count = 0
        self.drop_count = 0
        self.start_time = time.time()
        self.last_fps_report = time.time()

        # 启动帧读取线程
        self.read_thread = threading.Thread(target=self._optimized_frame_reader, daemon=True)
        self.read_thread.start()

    def _configure_camera_parameters(self):
        """配置摄像头参数，确保关键参数得到保持"""
        logger.info("开始配置摄像头关键参数...")

        # 1. 设置MJPEG格式 (关键：确保硬件解码兼容)
        fourcc = cv2.VideoWriter_fourcc(*'MJPG')
        success1 = self.cap.set(cv2.CAP_PROP_FOURCC, fourcc)

        # 2. 设置目标分辨率 (关键参数)
        success2 = self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, self.target_width)
        success3 = self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, self.target_height)

        # 3. 设置目标帧率 (关键参数)
        success4 = self.cap.set(cv2.CAP_PROP_FPS, self.target_fps)

        # 4. AMD GPU优化设置
        self.cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)  # 最小缓冲延迟

        # 自动曝光和对焦设置 (提高稳定性)
        self.cap.set(cv2.CAP_PROP_AUTO_EXPOSURE, 0.25)  # 自动曝光
        self.cap.set(cv2.CAP_PROP_AUTOFOCUS, 0)  # 关闭自动对焦减少延迟

        # AMD特定优化
        if os.path.exists('/dev/dri/renderD128'):
            logger.info("检测到AMD显卡设备，启用GPU加速优化")
            # 设置环境变量优化AMD性能
            os.environ['AMD_DISABLE_PERFCOUNTERS'] = '1'
            os.environ['MESA_GL_VERSION_OVERRIDE'] = '4.6'

        # 验证设置结果
        actual_fourcc = int(self.cap.get(cv2.CAP_PROP_FOURCC))
        fourcc_str = ''.join([chr((actual_fourcc >> 8 * i) & 0xFF) for i in range(4)])

        logger.info(f"摄像头配置结果:")
        logger.info(f"  格式: {fourcc_str} ({'成功' if success1 else '失败'})")
        logger.info(f"  分辨率: {self.target_width}x{self.target_height} ({'成功' if success2 and success3 else '失败'})")
        logger.info(f"  帧率: {self.target_fps}fps ({'成功' if success4 else '失败'})")

    def _validate_camera_parameters(self):
        """验证摄像头参数是否符合要求"""
        param_warnings = []

        if self.W != self.target_width or self.H != self.target_height:
            param_warnings.append(f"分辨率不匹配! 目标: {self.target_width}x{self.target_height}, "
                                f"实际: {self.W}x{self.H}")

        if abs(self.fps - self.target_fps) > 1.0:
            param_warnings.append(f"帧率不匹配! 目标: {self.target_fps}fps, "
                                f"实际: {self.fps}fps")

        if param_warnings:
            logger.warning("摄像头参数警告:")
            for warning in param_warnings:
                logger.warning(f"  {warning}")
        else:
            logger.info(f"✓ 摄像头参数验证通过: {self.W}x{self.H}@{self.fps}fps")

    def _optimized_frame_reader(self):
        """优化的帧读取线程 - 针对AMD GPU优化"""
        consecutive_failures = 0
        max_failures = 10

        while not self.stop_event.is_set():
            try:
                if self.frame_queue.full():
                    # 队列满时跳过当前帧减少延迟
                    ret, _ = self.cap.read()  # 丢弃帧
                    if ret:
                        self.drop_count += 1
                    continue

                ret, frame = self.cap.read()
                if ret and frame is not None:
                    # 获取原始MJPEG数据用于硬件解码
                    success, mjpeg_data = cv2.imencode('.jpg', frame,
                        [cv2.IMWRITE_JPEG_QUALITY, 90])  # 稍微降低质量以提高速度

                    if success:
                        self.frame_queue.put(mjpeg_data.tobytes(), block=False)
                        consecutive_failures = 0
                    else:
                        consecutive_failures += 1
                else:
                    consecutive_failures += 1

                # 检查连续失败
                if consecutive_failures >= max_failures:
                    logger.error(f"连续{max_failures}次读取失败，停止读取")
                    break

            except queue.Full:
                self.drop_count += 1
            except Exception as e:
                logger.error(f"帧读取错误: {e}")
                consecutive_failures += 1
                time.sleep(0.001)

    def _amd_decode_worker(self):
        """AMD GPU解码工作线程"""
        while not self.stop_event.is_set() or not self.frame_queue.empty():
            try:
                mjpeg_data = self.frame_queue.get(timeout=0.1)

                # 使用AMD GPU解码
                yuv_frame = self.amd_decoder.decode_frame(mjpeg_data)

                if yuv_frame is not None:
                    try:
                        self.output_queue.put(yuv_frame.data.tobytes(), block=False)
                    except queue.Full:
                        self.drop_count += 1

                self.frame_queue.task_done()

            except queue.Empty:
                continue
            except Exception as e:
                logger.error(f"AMD解码工作线程错误: {e}")

    def read_frames(self) -> Generator[bytes, None, None]:
        """读取处理后的帧"""
        # 启动AMD解码工作线程
        workers = []
        for i in range(self.num_workers):
            t = threading.Thread(target=self._amd_decode_worker, daemon=True)
            t.start()
            workers.append(t)

        try:
            while not self.stop_event.is_set():
                try:
                    yuv_bytes = self.output_queue.get(timeout=0.5)

                    # 性能统计
                    self.frame_count += 1
                    self._report_performance()

                    yield yuv_bytes
                    self.output_queue.task_done()

                except queue.Empty:
                    if self.stop_event.is_set():
                        break
        finally:
            self._cleanup(workers)

    def _report_performance(self):
        """报告性能统计"""
        current_time = time.time()
        if current_time - self.last_fps_report >= 5.0:  # 每5秒报告一次
            elapsed = current_time - self.start_time
            if elapsed > 0:
                fps = self.frame_count / elapsed
                drop_rate = self.drop_count / (self.frame_count + self.drop_count) * 100 if (self.frame_count + self.drop_count) > 0 else 0

                logger.info(f"AMD摄像头性能: {fps:.1f}fps, 丢帧率: {drop_rate:.1f}%")

            self.last_fps_report = current_time

    def _cleanup(self, workers):
        """清理资源"""
        logger.info("开始清理AMD摄像头资源...")

        self.stop_event.set()

        # 等待读取线程结束
        if self.read_thread.is_alive():
            self.read_thread.join(timeout=1.0)

        # 等待工作线程结束
        for t in workers:
            if t.is_alive():
                t.join(timeout=1.0)

        # 释放摄像头
        if self.cap.isOpened():
            self.cap.release()

        logger.info("AMD摄像头资源清理完成")

    def __del__(self):
        """析构函数"""
        self.stop_event.set()
        if hasattr(self, 'cap') and self.cap.isOpened():
            self.cap.release()


# 为了兼容性，提供不同的实现选择
CameraMJPG = CameraAMDOptimized  # 使用AMD优化版本作为默认实现

def create_camera(cam_type_state, stream_type, camera_id, backend='amd_optimized', **kwargs):
    """
    创建摄像头实例

    Args:
        backend: 'amd_optimized', 'cpu'
        kwargs: 其他参数，如 target_width, target_height, target_fps
    """
    if backend == 'amd_optimized':
        return CameraAMDOptimized(cam_type_state, stream_type, camera_id, **kwargs)
    else:
        # 导入原始CPU实现作为后备
        try:
            from .camera import CameraMJPG as CameraCPU
            return CameraCPU(cam_type_state, stream_type, camera_id, **kwargs)
        except ImportError:
            return CameraAMDOptimized(cam_type_state, stream_type, camera_id, **kwargs)