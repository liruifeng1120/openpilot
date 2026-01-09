#!/usr/bin/env python3
"""
OpenPilot 低 CPU 灰度视频流（稳定版）
"""

import time
import threading
import numpy as np
import cv2
from flask import Flask, Response, render_template_string

# =====================
# Flask
# =====================
app = Flask(__name__)

HTML = """
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>OpenPilot Gray Stream</title>
<style>
body { margin:0; background:#000; overflow:hidden; }
img { width:100vw; height:100vh; object-fit:contain; }
#info {
  position:fixed; left:10px; bottom:10px;
  color:#0f0; font-family:monospace; font-size:12px;
}
</style>
</head>
<body>
<img id="v" src="/video">
<div id="info">connecting...</div>
<script>
const v = document.getElementById("v");
const info = document.getElementById("info");
let last = Date.now(), cnt = 0;

v.onload = () => {
  const now = Date.now();
  const dt = now - last;
  last = now;
  cnt++;
  info.textContent = `lat ${dt} ms | fps ${(1000/dt).toFixed(1)}`;
  setTimeout(()=>v.src="/video?t="+Date.now(), 0);
};
v.onerror = ()=>setTimeout(()=>v.src="/video?t="+Date.now(), 200);
</script>
</body>
</html>
"""

# =====================
# 全局状态
# =====================
latest_jpeg = None
frame_lock = threading.Lock()

has_client = False
last_client_ts = 0.0

TARGET_W = 640          # 强烈建议 <= 640
JPEG_QUALITY = 55       # CPU/画质最优点
CLIENT_TIMEOUT = 2.0    # 秒：多久算“没人看”

# =====================
# Flask 路由
# =====================
@app.route("/")
def index():
    return render_template_string(HTML)

@app.route("/video")
def video():
    global has_client, last_client_ts
    has_client = True
    last_client_ts = time.time()

    def gen():
        while True:
            with frame_lock:
                data = latest_jpeg
            if data:
                yield (b"--frame\r\n"
                       b"Content-Type: image/jpeg\r\n\r\n" +
                       data + b"\r\n")
            time.sleep(0.005)  # 极低频，client 驱动
    return Response(gen(),
        mimetype="multipart/x-mixed-replace; boundary=frame")

# =====================
# 核心：Y → JPEG（最低 CPU）
# =====================
def y_to_jpeg_low_cpu(yuv, stride, w, h):
    # 只取 Y plane（零拷贝）
    y = np.frombuffer(yuv, dtype=np.uint8,
                      count=stride*h).reshape(h, stride)[:, :w]

    # resize（决定 CPU 的关键）
    if w > TARGET_W:
        nh = int(h * TARGET_W / w)
        y = cv2.resize(y, (TARGET_W, nh),
                      interpolation=cv2.INTER_AREA)

    _, jpeg = cv2.imencode(
        ".jpg", y,
        [cv2.IMWRITE_JPEG_QUALITY, JPEG_QUALITY]
    )
    return jpeg.tobytes()

# =====================
# 相机线程
# =====================
def camera_thread():
    global latest_jpeg, has_client

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
        print("camera connect failed")
        return

    w, h, s = vipc.width, vipc.height, vipc.stride
    print(f"camera {w}x{h} stride {s}")

    enc_cnt = 0
    t0 = time.time()

    while True:
        # ---- client gating（最重要）----
        if not has_client or time.time() - last_client_ts > CLIENT_TIMEOUT:
            has_client = False
            time.sleep(0.05)
            continue

        buf = vipc.recv()
        if not buf:
            time.sleep(0.001)
            continue

        jpeg = y_to_jpeg_low_cpu(buf.data, s, w, h)
        with frame_lock:
            latest_jpeg = jpeg

        enc_cnt += 1
        if time.time() - t0 > 5:
            print(f"encode fps: {enc_cnt/5:.1f}")
            enc_cnt = 0
            t0 = time.time()

# =====================
# main
# =====================
def main():
    print("="*60)
    print("OpenPilot LOW CPU Gray Stream")
    print("http://0.0.0.0:8888")
    print("="*60)

    threading.Thread(
        target=camera_thread,
        daemon=True
    ).start()

    app.run(
        host="0.0.0.0",
        port=8888,
        threaded=True,
        debug=False,
        use_reloader=False
    )

if __name__ == "__main__":
    main()
