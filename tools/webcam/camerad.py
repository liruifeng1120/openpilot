#!/usr/bin/env python3
import threading
import os
from collections import namedtuple

from msgq.visionipc import VisionIpcServer, VisionStreamType
from cereal import messaging

from openpilot.tools.webcam.camera import CameraMJPG
from openpilot.tools.webcam.camera_amd_gpu import CameraAMDOptimized, create_camera
from openpilot.common.realtime import Ratekeeper

WIDE_CAM = os.getenv("WIDE_CAM")
# AMD GPU硬件加速配置
USE_AMD_GPU = os.getenv("USE_AMD_GPU", "1") == "1"  # 默认启用AMD GPU加速
CAMERA_BACKEND = os.getenv("CAMERA_BACKEND", "amd_optimized")  # amd_optimized, cpu

# 关键参数配置 - 可通过环境变量自定义
CAMERA_WIDTH = int(os.getenv("CAMERA_WIDTH", "2592"))
CAMERA_HEIGHT = int(os.getenv("CAMERA_HEIGHT", "1944"))
CAMERA_FPS = float(os.getenv("CAMERA_FPS", "20.0"))

CameraType = namedtuple("CameraType", ["msg_name", "stream_type", "cam_id"])
CAMERAS = [
  CameraType("roadCameraState", VisionStreamType.VISION_STREAM_ROAD, os.getenv("ROAD_CAM", "0")),
  # CameraType("driverCameraState", VisionStreamType.VISION_STREAM_DRIVER, os.getenv("DRIVER_CAM", "2")),
]
if WIDE_CAM:
  CAMERAS.append(CameraType("wideRoadCameraState", VisionStreamType.VISION_STREAM_WIDE_ROAD, WIDE_CAM))

class Camerad:
  def __init__(self):
    self.pm = messaging.PubMaster([c.msg_name for c in CAMERAS])
    self.vipc_server = VisionIpcServer("camerad")

    print(f"摄像头配置: {len(CAMERAS)}个设备, AMD GPU: {USE_AMD_GPU}, 后端: {CAMERA_BACKEND}")
    print(f"目标参数: {CAMERA_WIDTH}x{CAMERA_HEIGHT}@{CAMERA_FPS}fps")

    self.cameras = []
    for c in CAMERAS:
      cam_device = f"/dev/video{c.cam_id}"
      print(f"正在初始化 {c.msg_name} 设备: {cam_device}")

      # 选择摄像头后端并传入关键参数
      if USE_AMD_GPU:
        try:
          cam = create_camera(
            c.msg_name, c.stream_type, cam_device,
            backend=CAMERA_BACKEND,
            target_width=CAMERA_WIDTH,
            target_height=CAMERA_HEIGHT,
            target_fps=CAMERA_FPS,
            num_workers=2,  # AMD GPU优化: 较少线程
            max_queue_size=3  # 减少延迟
          )
          print(f"✓ AMD GPU加速启用 ({CAMERA_BACKEND}): {cam.W}x{cam.H}@{cam.fps}fps")

          # 验证关键参数
          if cam.W != CAMERA_WIDTH or cam.H != CAMERA_HEIGHT:
            print(f"⚠ 分辨率警告: 期望 {CAMERA_WIDTH}x{CAMERA_HEIGHT}, 实际 {cam.W}x{cam.H}")
          if abs(cam.fps - CAMERA_FPS) > 1.0:
            print(f"⚠ 帧率警告: 期望 {CAMERA_FPS}fps, 实际 {cam.fps}fps")

        except Exception as e:
          print(f"✗ AMD GPU初始化失败，降级到CPU: {e}")
          cam = CameraMJPG(c.msg_name, c.stream_type, cam_device)
          print(f"✓ CPU解码启用: {cam.W}x{cam.H}@{getattr(cam, 'fps', 'N/A')}fps")
      else:
        cam = CameraMJPG(c.msg_name, c.stream_type, cam_device)
        print(f"✓ CPU解码启用: {cam.W}x{cam.H}@{getattr(cam, 'fps', 'N/A')}fps")

      self.cameras.append(cam)
      self.vipc_server.create_buffers(c.stream_type, 20, cam.W, cam.H)

    self.vipc_server.start_listener()

  def _send_yuv(self, yuv, frame_id, pub_type, yuv_type):
    eof = int(frame_id * 0.05 * 1e9)
    self.vipc_server.send(yuv_type, yuv, frame_id, eof, eof)
    dat = messaging.new_message(pub_type, valid=True)
    msg = {
      "frameId": frame_id,
      "transform": [1.0, 0.0, 0.0,
                    0.0, 1.0, 0.0,
                    0.0, 0.0, 1.0]
    }
    setattr(dat, pub_type, msg)
    self.pm.send(pub_type, dat)

  def camera_runner(self, cam):
    rk = Ratekeeper(20, None)
    for yuv in cam.read_frames():
      self._send_yuv(yuv, cam.cur_frame_id, cam.cam_type_state, cam.stream_type)
      cam.cur_frame_id += 1
      rk.keep_time()

  def run(self):
    threads = []
    for cam in self.cameras:
      cam_thread = threading.Thread(target=self.camera_runner, args=(cam,))
      cam_thread.start()
      threads.append(cam_thread)

    for t in threads:
      t.join()


def main():
  camerad = Camerad()
  camerad.run()


if __name__ == "__main__":
  main()
