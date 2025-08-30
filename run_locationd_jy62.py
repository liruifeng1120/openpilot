#!/usr/bin/env python3
"""
启动支持JY62设备的locationd服务
"""

import os
import sys
import subprocess
import time
import signal

def main():
    # 设置环境变量
    os.environ['IMU_TYPE'] = 'jy62'
    os.environ['IMU_DEVICE'] = '/dev/ttyUSB0'
    os.environ['IMU_BAUD'] = '9600'
    
    print("启动支持JY62设备的locationd服务...")
    print("环境变量设置:")
    print(f"  IMU_TYPE={os.environ['IMU_TYPE']}")
    print(f"  IMU_DEVICE={os.environ['IMU_DEVICE']}")
    print(f"  IMU_BAUD={os.environ['IMU_BAUD']}")
    print()
    
    # 构建命令
    cmd = [
        sys.executable,  # Python解释器
        'selfdrive/locationd/locationd.py',
        '--device', '/dev/ttyUSB0',
        '--baud', '115200',
        '--type', 'jy62'
    ]
    
    print("执行命令:")
    print(" ".join(cmd))
    print()
    
    try:
        # 启动locationd服务
        process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        print(f"locationd服务已启动，PID: {process.pid}")
        print("按Ctrl+C停止服务")
        print("-" * 50)
        
        # 实时显示输出
        while True:
            output = process.stdout.readline()
            if output:
                print(output.decode().strip())
            elif process.poll() is not None:
                break
                
            # 同时检查错误输出
            error = process.stderr.readline()
            if error:
                print("ERROR:", error.decode().strip())
                
    except KeyboardInterrupt:
        print("\n正在停止locationd服务...")
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
        print("locationd服务已停止")

if __name__ == "__main__":
    main()