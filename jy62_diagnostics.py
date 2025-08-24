#!/usr/bin/env python3
"""
JY62 IMU设备诊断脚本
用于全面检查JY62设备的串口通信和系统消息发布情况
"""

import serial
import time
import struct
import math
import cereal.messaging as messaging

def test_jy62_serial():
    """
    测试JY62设备的串口通信
    """
    print("=" * 60)
    print("JY62 IMU 串口通信测试")
    print("=" * 60)
    
    device_path = '/dev/ttyUSB0'
    
    try:
        # 连接设备
        ser = serial.Serial(device_path, 115200, 
                           bytesize=serial.EIGHTBITS,
                           parity=serial.PARITY_NONE,
                           stopbits=serial.STOPBITS_ONE,
                           timeout=1)
        print(f"✅ 成功连接到设备: {device_path}")
        print(f"   波特率: {ser.baudrate}")
        print(f"   数据位: {ser.bytesize}")
        print(f"   校验位: {ser.parity}")
        print(f"   停止位: {ser.stopbits}")
        
        print("\n开始读取数据包...")
        print("-" * 60)
        
        packet_buffer = bytearray()
        valid_packets = 0
        total_attempts = 0
        packet_types = {}
        
        start_time = time.time()
        while time.time() - start_time < 10:  # 测试10秒
            total_attempts += 1
            data = ser.read(32)
            if data:
                packet_buffer.extend(data)
                
                # 查找完整数据包
                while len(packet_buffer) >= 11:
                    # 查找包头
                    header_index = -1
                    for i in range(min(16, len(packet_buffer))):  # 限制搜索范围
                        if packet_buffer[i] == 0x55:
                            header_index = i
                            break
                    
                    if header_index == -1:
                        # 没有找到包头，清空缓冲区
                        packet_buffer = packet_buffer[-10:] if len(packet_buffer) > 10 else bytearray()
                        break
                    
                    if header_index > 0:
                        # 移除包头前的无效数据
                        packet_buffer = packet_buffer[header_index:]
                    
                    if len(packet_buffer) < 11:
                        # 数据不足，等待更多数据
                        break
                    
                    # 提取数据包
                    packet = packet_buffer[:11]
                    
                    # 验证校验和
                    checksum = sum(packet[:10]) & 0xFF
                    if checksum == packet[10]:
                        valid_packets += 1
                        packet_type = packet[1]
                        packet_types[packet_type] = packet_types.get(packet_type, 0) + 1
                        
                        print(f"[{valid_packets:2d}] 包类型: 0x{packet_type:02x}", end="")
                        
                        if packet_type == 0x51:  # 加速度
                            print(" (加速度数据)")
                            ax_raw = struct.unpack('<h', packet[2:4])[0]
                            ay_raw = struct.unpack('<h', packet[4:6])[0]
                            az_raw = struct.unpack('<h', packet[6:8])[0]
                            
                            # 转换为m/s² (±16g量程)
                            ax = (ax_raw / 32768.0) * 16.0 * 9.8
                            ay = (ay_raw / 32768.0) * 16.0 * 9.8
                            az = (az_raw / 32768.0) * 16.0 * 9.8
                            
                            print(f"     AX: {ax_raw:6d} ({ax:7.3f} m/s²)")
                            print(f"     AY: {ay_raw:6d} ({ay:7.3f} m/s²)")
                            print(f"     AZ: {az_raw:6d} ({az:7.3f} m/s²)")
                            
                        elif packet_type == 0x52:  # 角速度
                            print(" (角速度数据)")
                            wx_raw = struct.unpack('<h', packet[2:4])[0]
                            wy_raw = struct.unpack('<h', packet[4:6])[0]
                            wz_raw = struct.unpack('<h', packet[6:8])[0]
                            
                            # 转换为rad/s (±2000°/s量程)
                            wx = (wx_raw / 32768.0) * 2000.0 * math.pi / 180.0
                            wy = (wy_raw / 32768.0) * 2000.0 * math.pi / 180.0
                            wz = (wz_raw / 32768.0) * 2000.0 * math.pi / 180.0
                            
                            print(f"     WX: {wx_raw:6d} ({wx:7.3f} rad/s)")
                            print(f"     WY: {wy_raw:6d} ({wy:7.3f} rad/s)")
                            print(f"     WZ: {wz_raw:6d} ({wz:7.3f} rad/s)")
                            
                        elif packet_type == 0x53:  # 角度
                            print(" (角度数据)")
                            roll_raw = struct.unpack('<h', packet[2:4])[0]
                            pitch_raw = struct.unpack('<h', packet[4:6])[0]
                            yaw_raw = struct.unpack('<h', packet[6:8])[0]
                            
                            # 转换为度 (±180°量程)
                            roll = (roll_raw / 32768.0) * 180.0
                            pitch = (pitch_raw / 32768.0) * 180.0
                            yaw = (yaw_raw / 32768.0) * 180.0
                            
                            print(f"     Roll:  {roll_raw:6d} ({roll:7.2f} °)")
                            print(f"     Pitch: {pitch_raw:6d} ({pitch:7.2f} °)")
                            print(f"     Yaw:   {yaw_raw:6d} ({yaw:7.2f} °)")
                            
                        else:
                            print(f" (未知类型)")
                            print(f"     数据: {' '.join(f'{b:02x}' for b in packet[2:8])}")
                        
                        # 移除已处理的数据包
                        packet_buffer = packet_buffer[11:]
                    else:
                        # 校验和错误，移除包头继续查找
                        packet_buffer = packet_buffer[1:]
            
            time.sleep(0.01)
        
        ser.close()
        
        print("-" * 60)
        print("串口通信测试结果:")
        print(f"  总尝试次数: {total_attempts}")
        print(f"  有效数据包: {valid_packets}")
        print(f"  数据包类型统计:")
        for ptype, count in sorted(packet_types.items()):
            type_names = {0x51: "加速度", 0x52: "角速度", 0x53: "角度"}
            name = type_names.get(ptype, "未知")
            print(f"    0x{ptype:02x} ({name}): {count} 个")
            
        if valid_packets > 0:
            print("✅ 串口通信正常")
            return True
        else:
            print("❌ 未检测到有效数据包")
            return False
            
    except Exception as e:
        print(f"❌ 串口测试失败: {e}")
        return False

def test_system_messages():
    """
    测试系统消息发布情况
    """
    print("\n" + "=" * 60)
    print("JY62 IMU 系统消息测试")
    print("=" * 60)
    
    print("订阅 accelerometer 和 gyroscope 消息...")
    
    try:
        # 创建订阅者
        sm = messaging.SubMaster(['accelerometer', 'gyroscope'])
        
        print("开始接收消息 (10秒)...")
        print("-" * 60)
        
        accel_count = 0
        gyro_count = 0
        start_time = time.time()
        
        while time.time() - start_time < 10:
            sm.update()
            
            if sm.updated['accelerometer']:
                accel = sm['accelerometer']
                accel_count += 1
                if accel_count <= 5:  # 只显示前5个
                    if hasattr(accel.acceleration, 'v'):
                        values = accel.acceleration.v
                    else:
                        values = [accel.acceleration.x, accel.acceleration.y, accel.acceleration.z]
                    print(f"[{accel_count:2d}] 加速度: [{values[0]:8.3f}, {values[1]:8.3f}, {values[2]:8.3f}] m/s²")
            
            if sm.updated['gyroscope']:
                gyro = sm['gyroscope']
                gyro_count += 1
                if gyro_count <= 5:  # 只显示前5个
                    if hasattr(gyro.angularVelocity, 'v'):
                        values = gyro.angularVelocity.v
                    else:
                        values = [gyro.angularVelocity.x, gyro.angularVelocity.y, gyro.angularVelocity.z]
                    print(f"[{gyro_count:2d}] 角速度:  [{values[0]:8.3f}, {values[1]:8.3f}, {values[2]:8.3f}] rad/s")
            
            time.sleep(0.01)
        
        print("-" * 60)
        print("系统消息测试结果:")
        print(f"  接收到加速度消息: {accel_count} 个")
        print(f"  接收到角速度消息: {gyro_count} 个")
        
        if accel_count > 0 and gyro_count > 0:
            print("✅ 系统消息发布正常")
            return True
        else:
            print("❌ 系统消息发布异常")
            return False
            
    except Exception as e:
        print(f"❌ 系统消息测试失败: {e}")
        return False

def main():
    """
    主函数
    """
    print("JY62 IMU 设备诊断工具")
    print("此工具将检查JY62设备的串口通信和系统消息发布情况")
    
    serial_ok = test_jy62_serial()
    messages_ok = test_system_messages()
    
    print("\n" + "=" * 60)
    print("诊断总结")
    print("=" * 60)
    
    if serial_ok and messages_ok:
        print("✅ JY62 IMU设备工作正常")
        print("   - 串口通信正常")
        print("   - 系统消息发布正常")
        print("   - 数据通过accelerometer和gyroscope消息类型正确发布")
    elif serial_ok:
        print("⚠ JY62 IMU设备部分工作正常")
        print("   - 串口通信正常")
        print("   - 但系统消息发布异常")
        print("   可能原因:")
        print("   - locationd服务未运行")
        print("   - JY62设备支持未正确启用")
    elif messages_ok:
        print("⚠ JY62 IMU设备部分工作正常")
        print("   - 系统消息发布正常")
        print("   - 但串口通信异常")
        print("   可能原因:")
        print("   - 设备连接问题")
        print("   - 波特率或数据格式设置错误")
    else:
        print("❌ JY62 IMU设备工作异常")
        print("   - 串口通信和系统消息均异常")
        print("   建议检查:")
        print("   - 设备连接")
        print("   - 设备权限")
        print("   - locationd服务配置")

if __name__ == "__main__":
    main()