#!/bin/bash

# 监控JY60设备数据的脚本

echo "监控JY60设备数据"
echo "按 Ctrl+C 停止监控"

# 检查是否安装了必要工具
if ! command -v python3 &> /dev/null; then
    echo "错误: 未找到python3"
    exit 1
fi

# 运行监控脚本
PYTHONPATH=/home/liruifeng/ajouatom python3 -c "
import time
import cereal.messaging as messaging

def monitor_sensors():
    # 订阅传感器消息
    sm = messaging.SubMaster(['accelerometer', 'gyroscope'])
    
    print('开始监控传感器数据...')
    print('时间\t\t\t加速度计(X\tY\tZ)\t\t陀螺仪(X\tY\tZ)')
    print('-' * 80)
    
    try:
        while True:
            sm.update()
            
            timestamp = time.strftime('%H:%M:%S')
            
            if sm.updated['accelerometer']:
                accel = sm['accelerometer']
                ax = accel.acceleration.v[0]
                ay = accel.acceleration.v[1]
                az = accel.acceleration.v[2]
                accel_str = f'{ax:6.2f}\t{ay:6.2f}\t{az:6.2f}'
            else:
                accel_str = '  -  \t  -  \t  -  '
                
            if sm.updated['gyroscope']:
                gyro = sm['gyroscope']
                gx = gyro.angularVelocity.v[0]
                gy = gyro.angularVelocity.v[1]
                gz = gyro.angularVelocity.v[2]
                gyro_str = f'{gx:6.2f}\t{gy:6.2f}\t{gz:6.2f}'
            else:
                gyro_str = '  -  \t  -  \t  -  '
                
            print(f'{timestamp}\t\t{accel_str}\t\t{gyro_str}')
            
            time.sleep(0.1)  # 100ms更新间隔
            
    except KeyboardInterrupt:
        print('\\n监控已停止')
    except Exception as e:
        print(f'监控过程中发生错误: {e}')

if __name__ == '__main__':
    monitor_sensors()
"