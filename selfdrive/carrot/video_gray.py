#!/usr/bin/env python3
"""
OpenPilot MJPEG 灰度视频流
最低 CPU 稳定版（5FPS）
"""

import time
import threading
import cv2
import numpy as np
from flask import Flask, Response, render_template_string

# =============================
# 配置
# =============================
FPS = 5
INTERVAL = 1.0 / FPS
JPEG_QUALITY = 50

# =============================
# 全局
# =============================
latest_jpeg = None
frame_lock = threading.Lock()

has_client = False
client_lock = threading.Lock()

# =============================
# Flask
# =============================
app = Flask(__name__)

HTML = """
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>OpenPilot Gray Video (5FPS)</title>
<style>
body {
  margin: 0;
  background: black;
  display: flex;
  justify-content: center;
  align-items: center;
  height: 100vh;
}
img {
  max-width: 100%;
  max-height: 100%;
  object-fit: contain;
}
</style>
</head>
<body>
<img src="/video">
</body>
</html>
"""

# =============================
# Y → JPEG（修正版）
# =============================
def y_to_jpeg(buf, stride, w, h):
    try:
        data = np.frombuffer(buf, dtype=np.uint8)

        # 只取 Y plane
        y_plane = data[:h * stride]
        y_plane = y_plane.reshape((h, stride))[:, :w]

        ok, jpg = cv2.imencode(
            ".jpg",
            y_plane,
            [cv2.IMWRITE_JPEG_QUALITY, JPEG_QUALITY]
        )
        return jpg.tobytes() if ok else None
    except Exception as e:
        print("Y->JPEG error:", e)
        return None

# =============================
# camera_thread（5FPS）
# =============================
def camera_thread():
    global latest_jpeg

    from msgq.visionipc.visionipc_pyx import (
        VisionIpcClient, VisionStreamType
    )

    vipc = VisionIpcClient(
        "camerad",
        VisionStreamType.VISION_STREAM_ROAD,
        True
    )

    for _ in range(5):
        if vipc.connect(False):
            break
        time.sleep(1)
    else:
        print("Camera connect failed")
        return

    w, h, stride = vipc.width, vipc.height, vipc.stride
    print(f"Camera: {w}x{h}, stride={stride}")

    last_t = 0.0

    while True:
        buf = vipc.recv()
        if not buf:
            continue

        now = time.time()
        if now - last_t < INTERVAL:
            continue

        with client_lock:
            if not has_client:
                continue

        last_t = now

        jpeg = y_to_jpeg(buf.data, stride, w, h)
        if not jpeg:
            continue

        with frame_lock:
            latest_jpeg = jpeg

# =============================
# MJPEG generator（5FPS）
# =============================
def mjpeg_generator():
    global has_client

    with client_lock:
        has_client = True

    last_t = 0.0

    try:
        while True:
            dt = time.time() - last_t
            if dt < INTERVAL:
                time.sleep(INTERVAL - dt)

            with frame_lock:
                frame = latest_jpeg

            if frame is None:
                continue

            last_t = time.time()

            yield (
                b"--frame\r\n"
                b"Content-Type: image/jpeg\r\n\r\n" +
                frame +
                b"\r\n"
            )
    finally:
        with client_lock:
            has_client = False

# =============================
# 路由
# =============================
@app.route("/")
def index():
    return render_template_string(HTML)

@app.route("/video")
def video():
    return Response(
        mjpeg_generator(),
        mimetype="multipart/x-mixed-replace; boundary=frame"
    )

# =============================
# main
# =============================
def main():
    t = threading.Thread(target=camera_thread, daemon=True)
    t.start()

    print("Server: http://<device-ip>:8888")
    app.run(
        host="0.0.0.0",
        port=8888,
        threaded=True,
        debug=False
    )

if __name__ == "__main__":
    main()
