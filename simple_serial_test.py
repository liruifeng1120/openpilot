#!/usr/bin/env python3
import time
import serial

def test_serial_communication():
    """
    直接测试串口通信以验证原始数据
    """
    print("=== 直接串口通信测试 ===")
    try:
        # 尝试连接到JY60设备
        ser = serial.Serial('/dev/ttyUSB0', 9600, 
                           bytesize=serial.EIGHTBITS,
                           parity=serial.PARITY_NONE,
                           stopbits=serial.STOPBITS_ONE,
                           timeout=2)
        print("成功连接到JY60设备")
        
        print("读取原始数据包:")
        packet_count = 0
        start_time = time.time()
        
        while time.time() - start_time < 5 and packet_count < 10:  # 读取5秒或10个数据包
            data = ser.read(14)  # JY60 packets are 14 bytes long
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
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    print("开始JY60设备串口通信测试")
    test_serial_communication()
    print("\n串口通信测试完成")