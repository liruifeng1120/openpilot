#!/usr/bin/env python3
import serial
import time
import os

def test_jy60_binary():
    device_path = '/dev/ttyUSB0'
    
    # 检查设备是否存在
    if not os.path.exists(device_path):
        print(f"Device {device_path} does not exist")
        return
    
    # 检查设备权限
    if not os.access(device_path, os.R_OK):
        print(f"No read permission for {device_path}")
        print("Try running: sudo usermod -a -G dialout $USER")
        print("Then log out and log back in, or reboot the system")
        return
    
    try:
        # 尝试连接到JY60设备
        ser = serial.Serial(device_path, 9600, 
                           bytesize=serial.EIGHTBITS,
                           parity=serial.PARITY_NONE,
                           stopbits=serial.STOPBITS_ONE,
                           timeout=1)
        print(f"Connected to JY60 device on {device_path}")
        print(f"Baudrate: {ser.baudrate}")
        print(f"Bytesize: {ser.bytesize}")
        print(f"Parity: {ser.parity}")
        print(f"Stopbits: {ser.stopbits}")
        
        # 读取几行数据
        print("Reading binary data from JY60 device:")
        packet_count = 0
        start_time = time.time()
        
        while time.time() - start_time < 5:  # 读取5秒钟
            data = ser.read(14)  # JY60 packets are 14 bytes long
            if data and len(data) == 14:
                packet_count += 1
                # 显示原始字节数据
                hex_data = ' '.join(f'{b:02x}' for b in data)
                print(f"[{packet_count}] RAW: {hex_data}")
                
                # 检查协议头
                if data[0] == 0x55:
                    packet_type = data[1]
                    if packet_type == 0x51:
                        print(f"  -> Accelerometer packet")
                        # 解析加速度数据
                        ax = int.from_bytes(data[2:4], byteorder='little', signed=True)
                        ay = int.from_bytes(data[4:6], byteorder='little', signed=True)
                        az = int.from_bytes(data[6:8], byteorder='little', signed=True)
                        print(f"     AX: {ax}, AY: {ay}, AZ: {az}")
                    elif packet_type == 0x52:
                        print(f"  -> Gyroscope packet")
                        # 解析角速度数据
                        wx = int.from_bytes(data[2:4], byteorder='little', signed=True)
                        wy = int.from_bytes(data[4:6], byteorder='little', signed=True)
                        wz = int.from_bytes(data[6:8], byteorder='little', signed=True)
                        print(f"     WX: {wx}, WY: {wy}, WZ: {wz}")
                    else:
                        print(f"  -> Unknown packet type: 0x{packet_type:02x}")
                else:
                    print(f"  -> Invalid header: 0x{data[0]:02x}")
                
                if packet_count >= 10:
                    break
                    
            time.sleep(0.01)
        
        ser.close()
        print(f"Total packets received: {packet_count}")
        
    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    test_jy60_binary()