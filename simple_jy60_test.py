#!/usr/bin/env python3
import serial
import time
import sys

def simple_jy60_test():
    try:
        # 尝试连接到JY60设备
        ser = serial.Serial('/dev/ttyUSB0', 9600, timeout=2)
        print("Connected to JY60 device on /dev/ttyUSB0")
        
        print("Reading data for 5 seconds...")
        start_time = time.time()
        packet_count = 0
        
        while time.time() - start_time < 5:
            # 读取14字节的数据包
            data = ser.read(14)
            if len(data) == 14:
                packet_count += 1
                # 显示原始字节数据
                hex_data = ' '.join(f'{b:02x}' for b in data)
                print(f"Packet {packet_count}: {hex_data}")
                
                # 检查协议头
                if data[0] == 0x55:
                    packet_type = data[1]
                    if packet_type == 0x51:
                        print("  -> Accelerometer data")
                    elif packet_type == 0x52:
                        print("  -> Gyroscope data")
                
                # 如果收到足够多的数据包就停止
                if packet_count >= 5:
                    break
        
        ser.close()
        print(f"Test completed. Received {packet_count} packets.")
        
    except Exception as e:
        print(f"Error: {e}")
        return False
    
    return True

if __name__ == "__main__":
    simple_jy60_test()