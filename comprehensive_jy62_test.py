#!/usr/bin/env python3
import time
import cereal.messaging as messaging
import numpy as np

def test_sensor_data_accuracy():
    """
    测试传感器数据的准确性和稳定性
    """
    # 订阅accelerometer和gyroscope消息
    sm = messaging.SubMaster(['accelerometer', 'gyroscope'])
    
    print("开始测试传感器数据准确性和稳定性...")
    print("确保JY62设备已连接且locationd正在运行")
    
    # 收集数据
    accel_data = []
    gyro_data = []
    
    start_time = time.time()
    try:
        while time.time() - start_time < 10:  # 收集10秒数据
            sm.update()
            
            if sm.updated['accelerometer']:
                accel = sm['accelerometer']
                accel_values = [
                    accel.acceleration.v[0],
                    accel.acceleration.v[1], 
                    accel.acceleration.v[2]
                ]
                accel_data.append(accel_values)
                print(f"加速度计数据: x={accel_values[0]:.3f}, y={accel_values[1]:.3f}, z={accel_values[2]:.3f}")
            
            if sm.updated['gyroscope']:
                gyro = sm['gyroscope']
                gyro_values = [
                    gyro.angularVelocity.v[0],
                    gyro.angularVelocity.v[1],
                    gyro.angularVelocity.v[2]
                ]
                gyro_data.append(gyro_values)
                print(f"陀螺仪数据: x={gyro_values[0]:.3f}, y={gyro_values[1]:.3f}, z={gyro_values[2]:.3f}")
            
            time.sleep(0.01)  # 10ms间隔
            
    except KeyboardInterrupt:
        print("\n测试被用户中断")
    except Exception as e:
        print(f"测试过程中发生错误: {e}")
        import traceback
        traceback.print_exc()
    
    # 分析数据
    print("\n=== 测试结果分析 ===")
    if accel_data:
        accel_array = np.array(accel_data)
        print(f"加速度计数据统计 (共{len(accel_data)}个样本):")
        print(f"  X轴: 均值={np.mean(accel_array[:, 0]):.3f}, 标准差={np.std(accel_array[:, 0]):.3f}")
        print(f"  Y轴: 均值={np.mean(accel_array[:, 1]):.3f}, 标准差={np.std(accel_array[:, 1]):.3f}")
        print(f"  Z轴: 均值={np.mean(accel_array[:, 2]):.3f}, 标准差={np.std(accel_array[:, 2]):.3f}")
    else:
        print("未收到加速度计数据")
    
    if gyro_data:
        gyro_array = np.array(gyro_data)
        print(f"陀螺仪数据统计 (共{len(gyro_data)}个样本):")
        print(f"  X轴: 均值={np.mean(gyro_array[:, 0]):.3f}, 标准差={np.std(gyro_array[:, 0]):.3f}")
        print(f"  Y轴: 均值={np.mean(gyro_array[:, 1]):.3f}, 标准差={np.std(gyro_array[:, 1]):.3f}")
        print(f"  Z轴: 均值={np.mean(gyro_array[:, 2]):.3f}, 标准差={np.std(gyro_array[:, 2]):.3f}")
    else:
        print("未收到陀螺仪数据")
    
    # 验证重力加速度（静止时Z轴应接近9.8m/s²）
    if accel_data:
        mean_z = np.mean(accel_array[:, 2])
        gravity_error = abs(abs(mean_z) - 9.8) / 9.8 * 100
        print(f"\n重力加速度验证:")
        print(f"  Z轴平均值: {mean_z:.3f} m/s²")
        print(f"  与标准重力加速度误差: {gravity_error:.1f}%")
        if gravity_error < 10:  # 误差小于10%认为正常
            print("  ✓ 重力加速度测量正常")
        else:
            print("  ✗ 重力加速度测量异常")

def test_direct_serial_communication():
    """
    直接测试串口通信以验证原始数据
    """
    import serial
    
    print("\n=== 直接串口通信测试 ===")
    try:
        # 尝试连接到JY62设备
        ser = serial.Serial('/dev/ttyUSB0', 115200, 
                           bytesize=serial.EIGHTBITS,
                           parity=serial.PARITY_NONE,
                           stopbits=serial.STOPBITS_ONE,
                           timeout=2)
        print("成功连接到JY62设备")
        
        print("读取原始数据包:")
        packet_count = 0
        start_time = time.time()
        
        while time.time() - start_time < 5 and packet_count < 10:  # 读取5秒或10个数据包
            data = ser.read(14)  # JY62 packets are 14 bytes long
            if len(data) == 14:
                packet_count += 1
                # 显示原始字节数据
                hex_data = ' '.join(f'{b:02x}' for b in data)
                print(f"数据包 {packet_count}: {hex_data}")
                
                # 检查协议头
                if data[0] == 0x55:
                    packet_type = data[1]
                    if packet_type == 0x51:
                        print("  -> 加速度数据包")
                        # 解析加速度数据
                        ax = int.from_bytes(data[2:4], byteorder='little', signed=True)
                        ay = int.from_bytes(data[4:6], byteorder='little', signed=True)
                        az = int.from_bytes(data[6:8], byteorder='little', signed=True)
                        print(f"     AX: {ax}, AY: {ay}, AZ: {az}")
                        
                        # 转换为物理单位 (g)
                        ax_g = ax / 32768.0 * 16.0
                        ay_g = ay / 32768.0 * 16.0
                        az_g = az / 32768.0 * 16.0
                        print(f"     AX: {ax_g:.3f}g, AY: {ay_g:.3f}g, AZ: {az_g:.3f}g")
                    elif packet_type == 0x52:
                        print("  -> 角速度数据包")
                        # 解析角速度数据
                        wx = int.from_bytes(data[2:4], byteorder='little', signed=True)
                        wy = int.from_bytes(data[4:6], byteorder='little', signed=True)
                        wz = int.from_bytes(data[6:8], byteorder='little', signed=True)
                        print(f"     WX: {wx}, WY: {wy}, WZ: {wz}")
                        
                        # 转换为物理单位 (°/s)
                        wx_dps = wx / 32768.0 * 2000.0
                        wy_dps = wy / 32768.0 * 2000.0
                        wz_dps = wz / 32768.0 * 2000.0
                        print(f"     WX: {wx_dps:.3f}°/s, WY: {wy_dps:.3f}°/s, WZ: {wz_dps:.3f}°/s")
                    else:
                        print(f"  -> 未知数据包类型: 0x{packet_type:02x}")
                else:
                    print(f"  -> 无效协议头: 0x{data[0]:02x}")
        
        ser.close()
        print(f"总共接收到 {packet_count} 个数据包")
        
    except Exception as e:
        print(f"串口通信测试错误: {e}")

if __name__ == "__main__":
    print("开始JY62设备综合测试")
    
    # 先直接测试串口通信
    test_direct_serial_communication()
    
    # 再测试通过locationd处理的数据
    test_sensor_data_accuracy()
    
    print("\n综合测试完成")