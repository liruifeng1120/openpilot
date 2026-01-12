#!/usr/bin/env python3
"""
OpenPilot 彩色视频流服务器
正式版本 - 优化性能与颜色校正
"""

# ==============================
# 依赖检查（不影响 OP 启动）
# ==============================
def check_dependencies_or_exit():
    import importlib.util
    import sys

    missing = []

    if importlib.util.find_spec("flask") is None:
        missing.append("flask")

    if importlib.util.find_spec("cv2") is None:
        missing.append("opencv-python (cv2)")

    if missing:
        print("=" * 60)
        print("Lane server disabled: missing dependencies")
        for m in missing:
            print(f" - {m}")
        print("OpenPilot will continue normally.")
        print("=" * 60)
        sys.exit(0)   # 关键：正常退出，不抛异常


check_dependencies_or_exit()

# ==============================
# 只有通过检查，才继续
# ==============================
import time
import threading
import numpy as np
from flask import Flask, Response, render_template_string
import cv2

app = Flask(__name__)

# ==============================
# 全局变量
# ==============================
latest_jpeg = None
latest_gray = None
vipc_width = None
vipc_height = None
vipc_stride = None

target_width = 416
target_height = 416
JPEG_QUALITY = 50
gray_img = False

# 请求统计
req_lock = threading.Lock()
req_count = 0
req_window_start = time.time()
REQ_WINDOW = 2.0
latest_req_count = 0
req_frame_time = 0.05

frame_lock = threading.Lock()
last_snapshot_time = 0

status_text = "waiting app connect..."
status_lock = threading.Lock()

# ==============================
# HTML
# ==============================
HTML = """
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>Lane Detect</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body {
    margin: 0;
    background: black;
    color: #0f0;
    font-family: monospace;
    font-size: 18px;
    display: flex;
    align-items: center;
    justify-content: center;
    height: 100vh;
}
#text { white-space: pre; }
</style>
</head>
<body>
<div id="text">waiting app connect...</div>
<script>
const el = document.getElementById("text");
function update() {
    fetch("/status")
        .then(r => r.text())
        .then(t => el.textContent = t)
        .catch(() => {});
}
update();
setInterval(update, 1000);
</script>
</body>
</html>
"""

# ==============================
# Flask 路由
# ==============================
@app.route('/')
def index():
    return render_template_string(HTML)

@app.route("/status")
def status():
    with status_lock:
        return status_text

@app.route("/roadrgb.jpg")
def roadrgb():
    global latest_jpeg, last_snapshot_time, gray_img
    global req_count, latest_req_count, req_frame_time, req_window_start

    gray_img = False
    last_snapshot_time = time.time()

    with req_lock:
        now = time.time()
        if now - req_window_start > REQ_WINDOW:
            req_window_start = now
            latest_req_count = req_count
            if latest_req_count >= 1:
                req_frame_time = 2.0 / latest_req_count
            req_count = 0
        req_count += 1

    with frame_lock:
        jpeg = latest_jpeg

    if jpeg is None:
        placeholder = np.zeros((target_height, target_width, 3), dtype=np.uint8)
        _, jpeg = cv2.imencode(".jpg", placeholder)
        jpeg = jpeg.tobytes()

    return Response(jpeg, mimetype="image/jpeg")

@app.route("/roadgray.jpg")
def roadgray():
    global latest_gray, last_snapshot_time, gray_img
    global req_count, latest_req_count, req_frame_time, req_window_start

    gray_img = True
    last_snapshot_time = time.time()

    with req_lock:
        now = time.time()
        if now - req_window_start > REQ_WINDOW:
            req_window_start = now
            latest_req_count = req_count
            if latest_req_count >= 1:
                req_frame_time = 2.0 / latest_req_count
            req_count = 0
        req_count += 1

    with frame_lock:
        jpeg = latest_gray

    if jpeg is None:
        placeholder = np.zeros((target_height, target_width), dtype=np.uint8)
        ok, jpeg = cv2.imencode(
            ".jpg", placeholder,
            [cv2.IMWRITE_JPEG_QUALITY, JPEG_QUALITY]
        )
        jpeg = jpeg.tobytes() if ok else b""

    return Response(jpeg, mimetype="image/jpeg")

# ==============================
# 图像处理函数（原样保留）
# ==============================
def y_to_jpeg(buf, w, h, stride, target_w, target_h, quality=70):
    try:
        data = np.frombuffer(buf, dtype=np.uint8)
        y_plane = data[:h * stride].reshape(h, stride)[:, :w]

        scale = max(target_w / w, target_h / h)
        step = max(1, int(1 / scale))

        y_ds = y_plane[::step, ::step]
        ds_h, ds_w = y_ds.shape

        start_x = max(0, (ds_w - target_w) // 2)
        start_y = max(0, (ds_h - target_h) // 2)
        y_crop = y_ds[start_y:start_y+target_h, start_x:start_x+target_w]

        y_crop = y_crop[:target_h & ~1, :target_w & ~1]

        ok, jpg = cv2.imencode(
            ".jpg", y_crop,
            [cv2.IMWRITE_JPEG_QUALITY, quality]
        )
        return jpg.tobytes() if ok else None
    except Exception as e:
        print("Y->JPEG error:", e)
        return None

def convert_yuv_to_bgr(yuv_data, width, height, stride):
    try:
        y_size = height * stride
        y_plane = yuv_data[:y_size].reshape(height, stride)
        uv_plane = yuv_data[y_size:y_size+(height//2)*stride].reshape(height//2, stride)

        y_plane = y_plane[:, :width]
        uv_plane = uv_plane[:, :width]

        yuv_nv12 = np.vstack([y_plane, uv_plane])
        return cv2.cvtColor(yuv_nv12, cv2.COLOR_YUV2BGR_NV12)
    except:
        return None

def encode_to_jpg(image, target_w, target_h, quality):
    if image.shape[1] != target_w or image.shape[0] != target_h:
        image = cv2.resize(image, (target_w, target_h), interpolation=cv2.INTER_AREA)

    ok, jpeg = cv2.imencode(
        ".jpg", image,
        [cv2.IMWRITE_JPEG_QUALITY, quality]
    )
    return jpeg.tobytes() if ok else None

# ==============================
# Camera Thread
# ==============================
def camera_thread():
    global latest_jpeg, latest_gray, status_text, gray_img
    global vipc_width, vipc_height, vipc_stride

    try:
        from msgq.visionipc.visionipc_pyx import VisionIpcClient, VisionStreamType
    except Exception as e:
        print("VisionIPC unavailable:", e)
        return

    vipc = VisionIpcClient(
        "camerad",
        VisionStreamType.VISION_STREAM_ROAD,
        False
    )

    for _ in range(3):
        if vipc.connect(False):
            break
        time.sleep(1)
    else:
        return

    vipc_width = vipc.width
    vipc_height = vipc.height
    vipc_stride = vipc.stride

    last_print = time.time()
    frame_times = []

    while True:
        if time.time() - last_snapshot_time > 2.0:
            time.sleep(0.1)
            continue

        yuv_buf = vipc.recv()
        if not yuv_buf:
            time.sleep(0.05)
            continue

        start = time.time()

        if gray_img:
            jpeg = y_to_jpeg(
                yuv_buf.data,
                vipc_width, vipc_height, vipc_stride,
                target_width, target_height,
                JPEG_QUALITY
            )
            if jpeg:
                with frame_lock:
                    latest_gray = jpeg
        else:
            bgr = convert_yuv_to_bgr(
                yuv_buf.data,
                vipc_width, vipc_height, vipc_stride
            )
            if bgr is not None:
                jpeg = encode_to_jpg(
                    bgr, target_width, target_height, JPEG_QUALITY
                )
                if jpeg:
                    with frame_lock:
                        latest_jpeg = jpeg

        t = time.time() - start
        frame_times.append(t * 1000)

        if time.time() - last_print > 2.0 and frame_times:
            with status_lock:
                status_text = (
                    f"JPEG {target_width}x{target_height} | "
                    f"avg {np.mean(frame_times):.1f} ms | "
                    f"FPS {len(frame_times)/2:.1f}"
                )
            frame_times.clear()
            last_print = time.time()

        with req_lock:
            sleep_time = req_frame_time - t if req_frame_time > t else 0
        if sleep_time > 0:
            time.sleep(sleep_time)

# ==============================
# main
# ==============================
def main():
    import logging
    logging.getLogger("werkzeug").setLevel(logging.ERROR)

    print("=" * 60)
    print("Lane server started")
    print("http://0.0.0.0:8888")
    print("=" * 60)

    threading.Thread(target=camera_thread, daemon=True).start()

    from werkzeug.serving import run_simple
    run_simple("0.0.0.0", 8888, app, threaded=True, use_reloader=False)

if __name__ == "__main__":
    main()
