#!/bin/bash
# AMD GPU硬件加速摄像头解码设置脚本

set -e

echo "=== AMD GPU摄像头硬件解码设置 ==="

# 检测系统
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$ID
    VERSION=$VERSION_ID
else
    echo "无法检测系统版本"
    exit 1
fi

echo "检测到系统: $OS $VERSION"

# 安装基本依赖和VA-API支持
echo "安装VA-API和多媒体支持..."
sudo apt-get update
sudo apt-get install -y \
    vainfo \
    libva2 \
    libva-dev \
    libva-drm2 \
    libva-x11-2 \
    libva-wayland2 \
    ffmpeg \
    libavcodec-dev \
    libavformat-dev \
    libavutil-dev \
    libavdevice-dev

# Python依赖
echo "安装Python多媒体依赖..."
pip3 install av opencv-python numpy psutil 2>/dev/null || {
    echo "使用系统包管理器安装Python依赖..."
    sudo apt-get install -y python3-opencv python3-numpy python3-psutil
    pip3 install av || echo "警告: av模块安装失败，请手动安装"
}

# 检查现有AMD环境
echo "检查现有AMD环境..."
if lspci | grep -i "vga.*amd\|vga.*ati\|3d.*amd\|3d.*ati" > /dev/null; then
    echo "✓ 检测到AMD显卡"

    # 检查OpenCL
    if command -v clinfo > /dev/null; then
        echo "✓ OpenCL工具已安装"
    else
        echo "⚠ 未找到clinfo，如需要请安装: sudo apt-get install clinfo"
    fi

    # 检查Mesa驱动
    if [ -f "/usr/lib/x86_64-linux-gnu/dri/radeonsi_dri.so" ] || [ -f "/usr/lib/dri/radeonsi_dri.so" ]; then
        echo "✓ Mesa Radeon驱动已安装"
    else
        echo "⚠ 未检测到Mesa Radeon驱动，如需要请安装: sudo apt-get install mesa-va-drivers"
    fi
else
    echo "警告: 未检测到AMD显卡，硬件加速可能不可用"
fi

# 检查设备权限
echo "检查硬件设备权限..."
for device in /dev/dri/renderD128 /dev/dri/renderD129 /dev/dri/card0; do
    if [ -e "$device" ]; then
        if [ -r "$device" ] && [ -w "$device" ]; then
            echo "✓ 设备可访问: $device"
        else
            echo "⚠ 设备需要权限: $device"
            NEED_PERMISSION=1
        fi
    fi
done

if [ "$NEED_PERMISSION" = "1" ]; then
    echo "设置用户权限..."
    sudo usermod -a -G video $USER
    sudo usermod -a -G render $USER
    echo "✓ 已添加用户到video和render组"
    echo "⚠ 请重新登录或运行 'newgrp video' 使权限生效"
else
    echo "✓ 设备权限正常"
fi

# 测试VA-API
echo "测试VA-API..."
if command -v vainfo > /dev/null; then
    echo "VA-API信息:"
    timeout 10 vainfo | head -20 || echo "VA-API测试超时或失败"
else
    echo "vainfo命令不可用"
fi

# 创建AMD GPU测试脚本
cat > test_amd_gpu.py << 'EOF'
#!/usr/bin/env python3
"""AMD GPU摄像头解码测试脚本"""

import subprocess
import os
import sys

def test_vaapi():
    """测试VA-API"""
    try:
        result = subprocess.run(['vainfo'], capture_output=True, text=True, timeout=5)
        if result.returncode == 0:
            output = result.stdout.lower()
            if any(keyword in output for keyword in ['amd', 'radeon', 'radv']):
                if 'mjpeg' in output or 'motion jpeg' in output:
                    print("✓ AMD VA-API MJPEG解码支持确认")
                    return True
                else:
                    print("✗ AMD驱动不支持MJPEG硬件解码")
            else:
                print("✗ 未检测到AMD VA-API驱动")
        else:
            print("✗ VA-API不可用")
        return False
    except (FileNotFoundError, subprocess.TimeoutExpired):
        print("✗ vainfo命令不可用或超时")
        return False

def test_opencv():
    """测试OpenCV"""
    try:
        import cv2
        cap = cv2.VideoCapture(0)
        if cap.isOpened():
            ret, frame = cap.read()
            cap.release()
            if ret:
                print("✓ OpenCV摄像头访问正常")
                return True
        print("✗ OpenCV摄像头访问失败")
        return False
    except ImportError:
        print("✗ OpenCV未安装")
        return False

def test_devices():
    """测试设备文件"""
    devices = ['/dev/dri/renderD128', '/dev/dri/renderD129', '/dev/dri/card0']
    found = False
    for device in devices:
        if os.path.exists(device) and os.access(device, os.R_OK | os.W_OK):
            print(f"✓ 设备可访问: {device}")
            found = True
        elif os.path.exists(device):
            print(f"⚠ 设备存在但无权限: {device}")
        else:
            print(f"✗ 设备不存在: {device}")
    return found

def test_camera_device():
    """测试摄像头设备"""
    for i in range(5):
        device = f"/dev/video{i}"
        if os.path.exists(device):
            print(f"✓ 发现摄像头设备: {device}")
            return i
    print("✗ 未找到摄像头设备")
    return None

def main():
    print("=== AMD GPU摄像头解码测试 ===")

    vaapi_ok = test_vaapi()
    opencv_ok = test_opencv()
    device_ok = test_devices()
    camera_id = test_camera_device()

    print("\n配置建议:")
    if vaapi_ok and device_ok and opencv_ok:
        print("✓ AMD GPU解码环境完整，推荐配置:")
        print("export USE_AMD_GPU=1")
        print("export CAMERA_BACKEND=amd_optimized")
        print("export CAMERA_WIDTH=2592")
        print("export CAMERA_HEIGHT=1944")
        print("export CAMERA_FPS=20.0")
        print("export LIBVA_DRIVER_NAME=radeonsi")

        if camera_id is not None:
            print(f"export ROAD_CAM={camera_id}")

        print("\n启动命令:")
        print("cd /path/to/ajouatom")
        print("python3 tools/webcam/camerad.py")

    else:
        print("⚠ AMD GPU解码环境不完整，建议使用CPU模式:")
        print("export USE_AMD_GPU=0")
        print("export CAMERA_BACKEND=cpu")

    print("\n性能测试命令:")
    if camera_id is not None:
        print(f"python3 tools/webcam/benchmark_camera.py --camera {camera_id} --duration 20")
    else:
        print("python3 tools/webcam/benchmark_camera.py --camera 0 --duration 20")

if __name__ == "__main__":
    main()
EOF

chmod +x test_amd_gpu.py

# 创建启动脚本
cat > start_amd_camerad.sh << 'EOF'
#!/bin/bash
# AMD GPU摄像头启动脚本

# 设置AMD GPU优化环境变量
export USE_AMD_GPU=1
export CAMERA_BACKEND=amd_optimized
export CAMERA_WIDTH=2592
export CAMERA_HEIGHT=1944
export CAMERA_FPS=20.0
export LIBVA_DRIVER_NAME=radeonsi

# AMD性能优化
export AMD_DISABLE_PERFCOUNTERS=1
export MESA_GL_VERSION_OVERRIDE=4.6
export RADV_PERFTEST=aco

# 检查摄像头设备
if [ ! -e "/dev/video0" ]; then
    echo "错误: 未找到摄像头设备 /dev/video0"
    exit 1
fi

# 检查AMD设备权限
if [ ! -r "/dev/dri/renderD128" ] || [ ! -w "/dev/dri/renderD128" ]; then
    echo "错误: 无法访问AMD设备，请检查用户权限"
    echo "运行: sudo usermod -a -G video,render $USER && newgrp video"
    exit 1
fi

echo "启动AMD GPU摄像头解码..."
echo "配置: ${CAMERA_WIDTH}x${CAMERA_HEIGHT}@${CAMERA_FPS}fps"
echo "后端: ${CAMERA_BACKEND}"

cd "$(dirname "$0")"
python3 tools/webcam/camerad.py
EOF

chmod +x start_amd_camerad.sh

echo ""
echo "=== 配置完成 ==="
echo "检测结果:"
echo "  AMD显卡: $(lspci | grep -i amd | wc -l)个设备"
echo "  DRI设备: $(ls /dev/dri/ 2>/dev/null | wc -l)个文件"
echo "  VA-API: $(command -v vainfo >/dev/null && echo '可用' || echo '不可用')"
echo "  OpenCL: $(command -v clinfo >/dev/null && echo '可用' || echo '不可用')"
echo ""
echo "下一步:"
echo "1. 测试环境: ./test_amd_gpu.py"
echo "2. 启动摄像头: ./start_amd_camerad.sh"
echo "3. 性能测试: python3 tools/webcam/benchmark_camera.py --test amd"
echo ""
echo "环境变量快速设置:"
echo "  export USE_AMD_GPU=1 CAMERA_BACKEND=amd_optimized LIBVA_DRIVER_NAME=radeonsi"