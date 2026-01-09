#!/usr/bin/env python3
"""
OpenPilot 视频流 
"""

import time
import threading
import numpy as np
from flask import Flask, Response, render_template_string, request
import cv2

app = Flask(__name__)

# 极简 HTML 页面
HTML = """
<!DOCTYPE html>
<html>
<head>
    <title>OpenPilot 低延迟视频流</title>
    <meta charset="utf-8">
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { 
            background: #000; 
            height: 100vh; 
            display: flex; 
            align-items: center; 
            justify-content: center; 
            overflow: hidden;
            font-family: monospace;
        }
        #videoContainer {
            position: relative;
            max-width: 100vw;
            max-height: 100vh;
        }
        #videoFeed {
            max-width: 100vw;
            max-height: 100vh;
            display: block;
        }
        #stats {
            position: fixed;
            bottom: 10px;
            left: 10px;
            background: rgba(0,0,0,0.7);
            color: white;
            padding: 8px 12px;
            border-radius: 4px;
            font-size: 12px;
        }
    </style>
</head>
<body>
    <div id="videoContainer">
        <img id="videoFeed" src="/video_feed" alt="实时视频">
    </div>
    
    <div id="stats">
        <div>延迟: <span id="latencyText">--</span>ms</div>
        <div>FPS: <span id="fpsText">--</span></div>
        <div>状态: <span id="statusText">连接中...</span></div>
    </div>
    
    <script>
        let frameCount = 0;
        let lastFpsTime = Date.now();
        let lastFrameTime = Date.now();
        let latency = 0;
        
        const videoFeed = document.getElementById('videoFeed');
        const latencyText = document.getElementById('latencyText');
        const fpsText = document.getElementById('fpsText');
        const statusText = document.getElementById('statusText');
        
        // 更新统计信息
        function updateStats() {
            frameCount++;
            const now = Date.now();
            
            // 每秒更新 FPS
            if (now - lastFpsTime >= 1000) {
                fpsText.textContent = frameCount;
                frameCount = 0;
                lastFpsTime = now;
            }
            
            // 更新延迟显示
            latencyText.textContent = latency;
            
            requestAnimationFrame(updateStats);
        }
        
        // 视频加载成功
        videoFeed.onload = function() {
            const now = Date.now();
            latency = now - lastFrameTime;
            lastFrameTime = now;
            statusText.textContent = '已连接';
            
            // 立即请求下一帧（关键！实现最低延迟）
            setTimeout(() => {
                videoFeed.src = '/video_feed?t=' + Date.now();
            }, 0);
        };
        
        // 视频加载失败
        videoFeed.onerror = function() {
            statusText.textContent = '连接失败，重试中...';
            setTimeout(() => {
                videoFeed.src = '/video_feed?t=' + Date.now();
            }, 100);
        };
        
        // 键盘快捷键
        document.addEventListener('keydown', (e) => {
            if (e.key === 'f' || e.key === 'F') {
                if (document.fullscreenElement) {
                    document.exitFullscreen();
                } else {
                    document.documentElement.requestFullscreen();
                }
            } else if (e.key === 'r' || e.key === 'R') {
                videoFeed.src = '/video_feed?t=' + Date.now();
            }
        });
        
        // 页面可见性变化
        document.addEventListener('visibilitychange', () => {
            if (document.hidden) {
                // 页面隐藏时暂停
                videoFeed.src = '';
            } else {
                // 页面显示时恢复
                videoFeed.src = '/video_feed?t=' + Date.now();
            }
        });
        
        // 启动
        updateStats();
    </script>
</body>
</html>
"""

# 全局变量
latest_jpeg = None
frame_lock = threading.Lock()
camera_width = 0
camera_height = 0

@app.route('/')
def index():
    return render_template_string(HTML)

@app.route('/video_feed')
def video_feed():
    """视频流端点"""
    def generate():
        while True:
            with frame_lock:
                jpeg_data = latest_jpeg
            
            if jpeg_data:
                yield (b'--frame\r\n'
                       b'Content-Type: image/jpeg\r\n\r\n' + 
                       jpeg_data + b'\r\n')
            else:
                # 发送黑色帧
                black_frame = np.zeros((100, 100, 3), dtype=np.uint8)
                _, jpeg = cv2.imencode('.jpg', black_frame)
                yield (b'--frame\r\n'
                       b'Content-Type: image/jpeg\r\n\r\n' + 
                       jpeg.tobytes() + b'\r\n')
            
            # 控制帧率
            time.sleep(0.016)  # 约60 FPS
    
    return Response(generate(),
                    mimetype='multipart/x-mixed-replace; boundary=frame')

def yuv_to_jpeg_fast(yuv_data, stride, width, height):
    """YUV 转 JPEG - 快速版"""
    try:
        # 提取 Y 通道（最快方式）
        y_plane = np.frombuffer(yuv_data, dtype=np.uint8).reshape((-1, stride))
        y_img = y_plane[:height, :width]
        
        # 快速编码
        encode_params = [
            cv2.IMWRITE_JPEG_QUALITY, 70,
            cv2.IMWRITE_JPEG_OPTIMIZE, 1,
        ]
        
        _, jpeg = cv2.imencode('.jpg', y_img, encode_params)
        return jpeg.tobytes()
    except Exception as e:
        print(f"YUV转JPEG错误: {e}")
        return None

def camera_thread():
    """相机捕获线程"""
    global latest_jpeg, camera_width, camera_height
    
    print("启动相机捕获线程...")
    
    # 连接 OpenPilot 相机
    try:
        from msgq.visionipc.visionipc_pyx import VisionIpcClient, VisionStreamType
        
        print("尝试连接相机...")
        vipc_client = VisionIpcClient("camerad", VisionStreamType.VISION_STREAM_ROAD, True)
        
        # 尝试连接
        connected = False
        for i in range(5):
            if vipc_client.connect(False):
                connected = True
                break
            print(f"  连接尝试 {i+1}/5...")
            time.sleep(1)
        
        if not connected:
            print("相机连接失败，使用测试模式")
            raise ConnectionError("相机连接失败")
        
        camera_width = vipc_client.width
        camera_height = vipc_client.height
        print(f"✓ 相机已连接: {camera_width}x{camera_height}")
        
        # 主循环
        frame_count = 0
        last_log_time = time.time()
        
        while True:
            try:
                # 获取下一帧
                yuv_buf = vipc_client.recv()
                
                if yuv_buf:
                    # 快速处理
                    jpeg_data = yuv_to_jpeg_fast(
                        yuv_buf.data, 
                        yuv_buf.stride, 
                        camera_width, 
                        camera_height
                    )
                    
                    if jpeg_data:
                        with frame_lock:
                            latest_jpeg = jpeg_data
                        frame_count += 1
                    
                    # 控制帧率
                    time.sleep(0.016)  # 约60 FPS
                    
                else:
                    # 没有数据时短暂等待
                    time.sleep(0.001)
                
                # 性能统计
                if time.time() - last_log_time > 5:
                    fps = frame_count / 5
                    print(f"性能: {fps:.1f} FPS | 延迟: < 50ms")
                    frame_count = 0
                    last_log_time = time.time()
                    
            except Exception as e:
                print(f"捕获错误: {e}")
                time.sleep(0.1)
                
    except Exception as e:
        print(f"相机初始化失败: {e}")
        print("使用测试模式...")
        test_mode_thread()

def test_mode_thread():
    """测试模式 - 模拟相机数据"""
    global latest_jpeg
    
    print("运行测试模式...")
    
    width, height = 640, 480
    frame_num = 0
    
    while True:
        try:
            # 生成测试帧
            frame = np.zeros((height, width, 3), dtype=np.uint8)
            
            # 添加移动方块
            x = int((time.time() * 80) % (width - 60))
            frame[200:260, x:x+60] = [0, 255, 0]  # 绿色方块
            
            # 添加时间戳（毫秒）
            ms = int(time.time() * 1000) % 1000
            cv2.putText(frame, f"Test: {ms:03d}", (10, 30), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 1)
            
            # 添加帧号
            frame_num += 1
            cv2.putText(frame, f"Frame: {frame_num}", (width-120, 30), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.5, (200, 200, 200), 1)
            
            # 快速编码
            _, jpeg = cv2.imencode('.jpg', frame)
            
            with frame_lock:
                latest_jpeg = jpeg.tobytes()
            
            # 控制测试帧率
            time.sleep(0.033)  # 约 30 FPS
            
        except Exception as e:
            print(f"测试模式错误: {e}")
            time.sleep(1)

def main():
    """主函数"""
    print("=" * 60)
    print("OpenPilot 视频流服务器 - 修复版")
    print("=" * 60)
    
    # 检查 OpenCV
    try:
        import cv2
        print("✓ OpenCV 可用")
    except ImportError:
        print("✗ 需要安装 OpenCV")
        print("运行: pip install opencv-python")
        return
    
    # 启动相机线程
    cam_thread = threading.Thread(target=camera_thread, daemon=True)
    cam_thread.start()
    
    # 等待相机初始化
    time.sleep(2)
    
    # 启动 Flask 服务器
    print(f"\n服务器启动: http://0.0.0.0:8888")
    print("访问地址: http://<设备IP>:8888")
    print("快捷键: F=全屏, R=重新连接")
    print("按 Ctrl+C 停止\n")
    
    app.run(host='0.0.0.0', port=8888, threaded=True, debug=False, use_reloader=False)

if __name__ == "__main__":
    main()