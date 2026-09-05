#!/usr/bin/env python3
import threading
import os
from collections import namedtuple
from queue import Queue, Empty

from msgq.visionipc import VisionIpcServer, VisionStreamType
from openpilot.cereal import messaging

from openpilot.tools.webcam.camera import Camera
from openpilot.common.realtime import Ratekeeper

WIDE_CAM = os.getenv("WIDE_CAM")
USE_CAMPATH = os.getenv("USE_CAMPATH")
CameraType = namedtuple("CameraType", ["msg_name", "stream_type", "cam_id"])
if USE_CAMPATH:
  CAMERAS = [
    CameraType("roadCameraState", VisionStreamType.VISION_STREAM_ROAD, "pci-0000:c4:00.3-usb-0:4:1.0-video-index0"),
  ]
else:
  CAMERAS = [
    CameraType("roadCameraState", VisionStreamType.VISION_STREAM_ROAD, os.getenv("ROAD_CAM", "0")),
  ]
if WIDE_CAM:
  if USE_CAMPATH:
    CAMERAS.append(CameraType("wideRoadCameraState", VisionStreamType.VISION_STREAM_WIDE_ROAD, "pci-0000:c6:00.4-usb-0:1.2:1.0-video-index0"))
  else:
    CAMERAS.append(CameraType("wideRoadCameraState", VisionStreamType.VISION_STREAM_WIDE_ROAD, WIDE_CAM))

class Camerad:
  def __init__(self):
    self.pm = messaging.PubMaster([c.msg_name for c in CAMERAS])
    self.vipc_server = VisionIpcServer("camerad")

    self.cameras = []
    self.frame_queues = {}  # {stream_type: Queue}

    for c in CAMERAS:
      if USE_CAMPATH:
        cam_device = f"/dev/v4l/by-path/{c.cam_id}"
      else:
        cam_device = f"/dev/video{c.cam_id}"
      print(f"opening {c.msg_name} at {cam_device}")
      cam = Camera(c.msg_name, c.stream_type, cam_device)
      self.cameras.append(cam)
      self.vipc_server.create_buffers(c.stream_type, 20, cam.W, cam.H)
      self.frame_queues[c.stream_type] = Queue(maxsize=20)

    self.vipc_server.start_listener()

    self.enable_sync = len(CAMERAS) > 1
    print(f"[SYNC] Frame sync {'enabled' if self.enable_sync else 'disabled'} for {len(CAMERAS)} camera(s)")

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

  def camera_reader(self, cam):
    for yuv in cam.read_frames():
      try:
        self.frame_queues[cam.stream_type].put(yuv, timeout=0.1)
      except Exception:
        pass

  def sync_and_send(self):
    rk = Ratekeeper(20, None)
    global_frame_id = 0
    last_frames = {}

    while True:
      for cam in self.cameras:
        queue = self.frame_queues[cam.stream_type]
        try:
          while True:
            yuv = queue.get_nowait()
            last_frames[cam.stream_type] = yuv
        except Empty:
          pass

      if self.enable_sync:
        if len(last_frames) == len(self.cameras):
          for cam in self.cameras:
            self._send_yuv(last_frames[cam.stream_type], global_frame_id, cam.cam_type_state, cam.stream_type)
          global_frame_id += 1
      else:
        for cam in self.cameras:
          if cam.stream_type in last_frames:
            self._send_yuv(last_frames[cam.stream_type], global_frame_id, cam.cam_type_state, cam.stream_type)
            global_frame_id += 1

      rk.keep_time()

      if global_frame_id % 200 == 0:
        print(f"[SYNC] Frame {global_frame_id}")

  def run(self):
    threads = []

    # 启动每个摄像头的读取线程
    for cam in self.cameras:
      reader_thread = threading.Thread(target=self.camera_reader, args=(cam,))
      reader_thread.start()
      threads.append(reader_thread)

    # 启动同步发送线程
    sender_thread = threading.Thread(target=self.sync_and_send)
    sender_thread.start()
    threads.append(sender_thread)

    for t in threads:
      t.join()


def main():
  camerad = Camerad()
  camerad.run()


if __name__ == "__main__":
  main()