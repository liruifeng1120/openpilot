#!/usr/bin/env python3
"""
简化的C++摄像头Python包装器 - 同步读取模式
专为匹配您当前的代码模式而设计
"""
import numpy as np
import time
import os
from typing import Optional, Tuple

class CameraCppMJPG:
    """
    C++ MJPEG摄像头类，替代原有的Python实现
    采用同步读取模式，类似您现在的CameraMJPG类
    """

    def __init__(self, cam_type_state=None, stream_type=None, camera_id=0):
        """
        初始化C++ MJPEG摄像头，兼容原有接口

        Args:
            cam_type_state: 兼容参数
            stream_type: 兼容参数
            camera_id: 摄像头设备ID或路径
        """
        try:
            camera_id = int(camera_id)
        except ValueError:
            pass  # 允许字符串路径如 /dev/video0

        self.cam_type_state = cam_type_state
        self.stream_type = stream_type
        self.device_id = camera_id

        # 设备路径处理
        if isinstance(camera_id, int):
            self.device_path = f"/dev/video{camera_id}"
        else:
            self.device_path = str(camera_id)

        # 使用与原代码相同的参数
        self.width = 2592
        self.height = 1944
        self.fps = 20

        print(f"初始化C++ MJPEG摄像头")
        print(f"设备: {self.device_path}")
        print(f"分辨率: {self.width}x{self.height}")
        print(f"帧率: {self.fps} fps")

        self.frame_count = 0
        self.cur_frame_id = 0
        self.current_format = "MJPG"
        self._opened = False

        # 检查并初始化
        self._initialize()

    def _initialize(self):
        """内部初始化方法"""
        try:
            # 检查设备文件是否存在
            if not os.path.exists(self.device_path):
                raise IOError(f"摄像头设备不存在: {self.device_path}")

            # 检查设备权限
            if not os.access(self.device_path, os.R_OK | os.W_OK):
                raise IOError(f"摄像头设备权限不足: {self.device_path}")

            print("C++摄像头设备检查通过")

            # 这里将来会调用实际的C++初始化
            # 现在先标记为已打开进行测试
            self._opened = True

            print("C++摄像头初始化完成")

        except Exception as e:
            print(f"C++摄像头初始化失败: {e}")
            self._opened = False
            raise IOError(f"无法打开摄像头设备 {self.device_path}: {e}")

    def read_frames(self):
        """
        持续读取帧的生成器，兼容原有接口
        这个方法类似您原代码中的read_frames
        """
        while True:
            success, frame_bgr = self.read()
            if not success or frame_bgr is None:
                break

            # 转换为NV12格式（兼容原代码）
            yield self._bgr_to_nv12(frame_bgr)

        self.release()

    def read(self) -> Tuple[bool, Optional[np.ndarray]]:
        """
        读取单帧，同步模式，类似您现在的代码模式
        兼容OpenCV的cap.read()接口

        Returns:
            success: 是否成功读取
            frame: BGR格式的帧数据（类似OpenCV输出）
        """
        if not self._opened:
            return False, None

        try:
            # 这里将来会调用C++的同步read方法
            # 现在模拟成功读取
            success = self._simulate_cpp_read()

            if success:
                # 模拟返回BGR帧（实际会从C++获取并解码MJPEG）
                frame = self._create_test_frame()

                self.frame_count += 1
                self.cur_frame_id += 1

                # 每100帧打印一次帧率统计（类似原代码）
                if self.frame_count % 100 == 0:
                    print(f"C++摄像头已读取 {self.frame_count} 帧")

                return True, frame
            else:
                return False, None

        except Exception as e:
            print(f"C++摄像头读取帧失败: {e}")
            return False, None

    def _simulate_cpp_read(self) -> bool:
        """模拟C++读取（实际实现中会调用真正的C++方法）"""
        # 模拟90%成功率
        import random
        return random.random() > 0.1

    def _create_test_frame(self) -> np.ndarray:
        """创建测试帧（实际实现中会从C++获取真实帧）"""
        # 创建一个简单的测试图像
        frame = np.zeros((self.height, self.width, 3), dtype=np.uint8)

        # 添加一些变化（模拟真实视频）
        frame[:, :, 0] = (self.frame_count % 256)  # 变化的蓝色通道
        frame[100:200, 100:200] = [0, 255, 0]      # 绿色方块

        return frame

    @staticmethod
    def _bgr_to_nv12(bgr_frame):
        """BGR转NV12，兼容原代码"""
        try:
            import av
            frame = av.VideoFrame.from_ndarray(bgr_frame, format='bgr24')
            return frame.reformat(format='nv12').to_ndarray().data.tobytes()
        except ImportError:
            # 如果没有av库，返回简单的字节数据
            return bgr_frame.tobytes()

    def isOpened(self) -> bool:
        """检查摄像头是否打开（兼容OpenCV接口）"""
        return self._opened

    def release(self):
        """释放资源"""
        if self._opened:
            print("释放C++摄像头资源")
            # 这里将来会调用C++的cleanup方法
            self._opened = False

    def get_device_info(self) -> str:
        """获取设备信息"""
        return f"C++摄像头驱动 - 设备: {self.device_path}, 分辨率: {self.width}x{self.height}, 格式: {self.current_format}"

    def __del__(self):
        """析构函数"""
        self.release()


def test_cpp_camera_sync():
    """测试C++摄像头的同步读取模式"""
    print("=" * 50)
    print("测试C++摄像头同步读取模式")
    print("=" * 50)

    try:
        # 创建摄像头实例（使用与您代码相同的参数）
        camera = CameraCppMJPG(cam_type_state="webcam", stream_type="test", camera_id=0)

        if not camera.isOpened():
            print("摄像头打开失败")
            return False

        print("摄像头打开成功")
        print(camera.get_device_info())

        # 同步读取测试（类似您的代码模式）
        print("开始同步读取测试...")
        for i in range(10):
            success, frame = camera.read()
            if success and frame is not None:
                print(f"成功读取第 {i+1} 帧, 尺寸: {frame.shape}")
            else:
                print(f"第 {i+1} 帧读取失败")

            time.sleep(0.05)  # 50ms间隔

        # 测试生成器模式（read_frames）
        print("\n测试生成器模式...")
        frame_gen = camera.read_frames()
        for i, frame_data in enumerate(frame_gen):
            if i >= 5:  # 只测试5帧
                break
            print(f"生成器模式第 {i+1} 帧，数据长度: {len(frame_data)}")

        camera.release()
        print("测试完成")
        return True

    except Exception as e:
        print(f"测试失败: {e}")
        import traceback
        traceback.print_exc()
        return False


if __name__ == "__main__":
    # 运行同步读取测试
    success = test_cpp_camera_sync()
    print(f"\n测试结果: {'成功' if success else '失败'}")