#!/usr/bin/env python3
import time
import cereal.messaging as messaging

def test_locationd_jy62():
    # 订阅accelerometer和gyroscope消息
    sm = messaging.SubMaster(['accelerometer', 'gyroscope'])
    
    print("监听来自locationd的加速度计和陀螺仪消息...")
    print("确保locationd正在运行并支持JY62设备")
    
    start_time = time.time()
    accel_count = 0
    gyro_count = 0
    
    try:
        while time.time() - start_time < 10:  # 运行10秒钟
            sm.update()
            
            if sm.updated['accelerometer']:
                accel = sm['accelerometer']
                accel_count += 1
                print(f"加速度计 #{accel_count}: x={accel.acceleration.x:.3f}, "
                      f"y={accel.acceleration.y:.3f}, z={accel.acceleration.z:.3f}")
            
            if sm.updated['gyroscope']:
                gyro = sm['gyroscope']
                gyro_count += 1
                print(f"陀螺仪 #{gyro_count}: x={gyro.angularVelocity.x:.3f}, "
                      f"y={gyro.angularVelocity.y:.3f}, z={gyro.angularVelocity.z:.3f}")
            
            time.sleep(0.01)  # 10ms间隔
            
    except KeyboardInterrupt:
        print("\n测试被用户中断")
    except Exception as e:
        print(f"测试过程中发生错误: {e}")
    
    print(f"\n测试完成:")
    print(f"  收到加速度计消息: {accel_count}")
    print(f"  收到陀螺仪消息: {gyro_count}")

if __name__ == "__main__":
    test_locationd_jy62()