#!/usr/bin/env python3
"""
OpenPilot 彩色视频流服务器
正式版本 - 优化性能与颜色校正
"""

import time
import threading
import numpy as np
from flask import Flask, Response, render_template_string
import cv2

app = Flask(__name__)

# 全局变量
latest_jpeg = None
frame_lock = threading.Lock()

HTML = """
<!DOCTYPE html>
<html>
<head>
    <title>OpenPilot 视频流</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { background: #000; color: white; font-family: monospace; }
        #container { position: relative; height: 100vh; }
        #video { width: 100vw; height: 100vh; object-fit: contain; }
        #info { 
            position: fixed; top: 10px; right: 10px; 
            background: rgba(0,0,0,0.7); padding: 10px; 
            border-radius: 5px; font-size: 12px;
        }
        #status { position: fixed; top: 10px; left: 10px; color: #0f0; }
    </style>
</head>
<body>
    <div id="container">
        <img id="video" src="/video">
        <div id="status">● 视频流运行中</div>
        <div id="info">
            延迟: <span id="latency">--</span>ms<br>
            分辨率: <span id="res">--</span><br>
            FPS: <span id="fps">--</span>
        </div>
    </div>
    
    <script>
        let frameCount = 0, lastTime = Date.now();
        const video = document.getElementById('video');
        const latencyEl = document.getElementById('latency');
        const fpsEl = document.getElementById('fps');
        const resEl = document.getElementById('res');
        
        video.onload = () => {
            const now = Date.now();
            const latency = now - lastTime;
            lastTime = now;
            frameCount++;
            
            latencyEl.textContent = latency;
            resEl.textContent = video.naturalWidth + '×' + video.naturalHeight;
            
            if (latency > 0) {
                fpsEl.textContent = (1000 / latency).toFixed(1);
            }
            
            setTimeout(() => video.src = '/video?t=' + Date.now(), 0);
        };
        
        video.onerror = () => setTimeout(() => video.src = '/video?t=' + Date.now(), 100);
        
        document.addEventListener('keydown', (e) => {
            if (e.key === 'f') document.fullscreenElement ? 
                document.exitFullscreen() : document.documentElement.requestFullscreen();
        });
    </script>
</body>
</html>
"""

@app.route('/')
def index():
    return render_template_string(HTML)

@app.route('/video')
def video():
    def generate():
        global latest_jpeg
        while True:
            with frame_lock:
                jpeg_data = latest_jpeg
            
            if jpeg_data:
                yield (b'--frame\r\nContent-Type: image/jpeg\r\n\r\n' + jpeg_data + b'\r\n')
            else:
                test_frame = np.zeros((480, 640, 3), dtype=np.uint8)
                test_frame[100:140, 100:540] = [0, 0, 255]      # 红
                test_frame[180:220, 100:540] = [0, 255, 0]      # 绿
                test_frame[260:300, 100:540] = [255, 0, 0]      # 蓝
                _, jpeg = cv2.imencode('.jpg', test_frame)
                yield (b'--frame\r\nContent-Type: image/jpeg\r\n\r\n' + jpeg.tobytes() + b'\r\n')
            
            time.sleep(0.033)
    
    return Response(generate(), mimetype='multipart/x-mixed-replace; boundary=frame')

def convert_yuv_to_bgr(yuv_data, width, height, stride):
    """YUV NV12 转 BGR (OpenCV默认顺序)"""
    try:
        y_size = height * stride
        
        y_plane = yuv_data[:y_size].reshape(height, stride)
        uv_plane = yuv_data[y_size:y_size + (height//2) * stride].reshape(height//2, stride)
        
        if stride > width:
            y_plane = y_plane[:, :width]
            uv_plane = uv_plane[:, :width]
        
        yuv_nv12 = np.vstack([y_plane, uv_plane])
        bgr_img = cv2.cvtColor(yuv_nv12, cv2.COLOR_YUV2BGR_NV12)
        
        return bgr_img
    except:
        return None

def optimize_image(image, target_width=960, quality=70):
    """优化图像尺寸和质量"""
    height, width = image.shape[:2]
    
    if width > target_width:
        scale = target_width / width
        new_height = int(height * scale)
        image = cv2.resize(image, (target_width, new_height), interpolation=cv2.INTER_LINEAR)
    
    encode_params = [cv2.IMWRITE_JPEG_QUALITY, quality, cv2.IMWRITE_JPEG_OPTIMIZE, 1]
    success, jpeg_data = cv2.imencode('.jpg', image, encode_params)
    
    return jpeg_data.tobytes() if success else None

def camera_thread():
    """相机捕获线程"""
    global latest_jpeg
    
    try:
        from msgq.visionipc.visionipc_pyx import VisionIpcClient, VisionStreamType
        
        vipc_client = VisionIpcClient("camerad", VisionStreamType.VISION_STREAM_ROAD, True)
        
        for i in range(3):
            if vipc_client.connect(False):
                break
            time.sleep(1)
        else:
            return
        
        width = vipc_client.width
        height = vipc_client.height
        stride = vipc_client.stride
        
        frame_times = []
        last_print = time.time()
        
        while True:
            try:
                yuv_buf = vipc_client.recv()
                
                if yuv_buf:
                    start_time = time.time()
                    
                    bgr_img = convert_yuv_to_bgr(yuv_buf.data, width, height, stride)
                    
                    if bgr_img is not None:
                        jpeg_data = optimize_image(bgr_img, target_width=960, quality=70)
                        
                        if jpeg_data:
                            with frame_lock:
                                latest_jpeg = jpeg_data
                            
                            frame_times.append((time.time() - start_time) * 1000)
                            
                            current_time = time.time()
                            if current_time - last_print >= 2.0:
                                if frame_times:
                                    avg_time = np.mean(frame_times)
                                    print(f"状态: {bgr_img.shape[1]}x{bgr_img.shape[0]} | 延迟: {avg_time:.1f}ms | FPS: {len(frame_times)/2:.1f}")
                                frame_times = []
                                last_print = current_time
                    
                    elapsed = time.time() - start_time
                    if elapsed < 0.033:
                        time.sleep(0.033 - elapsed)
                        
            except:
                time.sleep(0.1)
                
    except:
        pass

def main():
    print("=" * 60)
    print("OpenPilot 视频流服务器")
    print("访问: http://0.0.0.0:8888")
    print("=" * 60)
    
    cam_thread = threading.Thread(target=camera_thread, daemon=True)
    cam_thread.start()
    
    time.sleep(1)
    
    from werkzeug.serving import run_simple
    run_simple('0.0.0.0', 8888, app, threaded=True, processes=1, use_reloader=False)

if __name__ == "__main__":
    main()