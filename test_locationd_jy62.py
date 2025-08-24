#!/usr/bin/env python3
import time
import cereal.messaging as messaging

def test_locationd_jy62():
    # 订阅accelerometer和gyroscope消息
    sm = messaging.SubMaster(['accelerometer', 'gyroscope'])
    
    print("Listening for accelerometer and gyroscope messages from locationd...")
    print("Make sure locationd is running with JY62 support")
    
    start_time = time.time()
    accel_count = 0
    gyro_count = 0
    
    try:
        while time.time() - start_time < 10:  # 运行10秒钟
            sm.update()
            
            if sm.updated['accelerometer']:
                accel = sm['accelerometer']
                accel_count += 1
                print(f"Accelerometer #{accel_count}: x={accel.acceleration.x:.3f}, "
                      f"y={accel.acceleration.y:.3f}, z={accel.acceleration.z:.3f}")
            
            if sm.updated['gyroscope']:
                gyro = sm['gyroscope']
                gyro_count += 1
                print(f"Gyroscope #{gyro_count}: x={gyro.angularVelocity.x:.3f}, "
                      f"y={gyro.angularVelocity.y:.3f}, z={gyro.angularVelocity.z:.3f}")
            
            time.sleep(0.01)  # 10ms间隔
            
    except KeyboardInterrupt:
        print("\nTest interrupted by user")
    except Exception as e:
        print(f"Error occurred: {e}")
    
    print(f"\nTest completed:")
    print(f"  Accelerometer messages received: {accel_count}")
    print(f"  Gyroscope messages received: {gyro_count}")

if __name__ == "__main__":
    test_locationd_jy62()