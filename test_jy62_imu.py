#!/usr/bin/env python3
"""
测试JY62 IMU设备的脚本
用于验证设备连接和数据读取是否正常工作
"""
import time
import cereal.messaging as messaging

def test_jy62_imu():
    print("开始测试JY62 IMU设备")
    print("确保设备已连接到正确的串口设备（如 /dev/ttyUSB0 或 /dev/ttyACM0），并检查波特率是否匹配（如 115200）")
    print("按 Ctrl+C 停止测试")
    
    # 订阅IMU消息
    sm = messaging.SubMaster(['accelerometer', 'gyroscope'])
    
    print("\n时间\t\t加速度计(X\tY\tZ)\t\t陀螺仪(X\tY\tZ)")
    print("-" * 70)
    
    try:
        while True:
            sm.update()
            
            timestamp = time.strftime('%H:%M:%S')
            
            # 处理加速度计数据
            if sm.updated['accelerometer']:
                accel = sm['accelerometer']
                ax = accel.acceleration.v[0]
                ay = accel.acceleration.v[1]
                az = accel.acceleration.v[2]
                accel_str = f"{ax:6.2f}\t{ay:6.2f}\t{az:6.2f}"
            else:
                accel_str = "  -  \t  -  \t  -  "
                
            # 处理陀螺仪数据
            if sm.updated['gyroscope']:
                gyro = sm['gyroscope']
                gx = gyro.angularVelocity.v[0]
                gy = gyro.angularVelocity.v[1]
                gz = gyro.angularVelocity.v[2]
                gyro_str = f"{gx:6.2f}\t{gy:6.2f}\t{gz:6.2f}"
            else:
                gyro_str = "  -  \t  -  \t  -  "
                
            print(f"{timestamp}\t{accel_str}\t\t{gyro_str}")
            
            time.sleep(0.1)
            
    except KeyboardInterrupt:
        print("\n测试已停止")
    except Exception as e:
        print(f"测试过程中发生错误: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    test_jy62_imu()