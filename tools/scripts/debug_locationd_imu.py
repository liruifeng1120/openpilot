#!/usr/bin/env python3  
"""  
LocationD IMU数据调试脚本  
用于验证C++版本locationd中CAN信号yawRate备用功能  
  
使用方法: ./debug_locationd_imu.py "path/to/rlog.zst"  
"""  
  
import sys  
import argparse  
from collections import defaultdict  
from openpilot.tools.lib.logreader import LogReader  
  
def analyze_sensor_availability(lr):  
    """分析传感器数据可用性"""  
    print("=== 传感器数据可用性分析 ===")  
      
    sensor_counts = defaultdict(int)  
    required_inputs = ['carState', 'accelerometer', 'gyroscope', 'gpsLocationExternal', 'cameraOdometry', 'liveCalibration']  
      
    for msg in lr:  
        if msg.which() in required_inputs:  
            sensor_counts[msg.which()] += 1  
      
    print("LocationD需要的输入消息:")  
    for input_type in required_inputs:  
        count = sensor_counts[input_type]  
        status = "✓" if count > 0 else "✗"  
        print(f"  {status} {input_type}: {count}")  
      
    # 特别关注IMU传感器  
    imu_available = sensor_counts['gyroscope'] > 0 and sensor_counts['accelerometer'] > 10  
    print(f"\nIMU传感器状态: {'可用' if imu_available else '不可用'}")  
    print(f"  - 陀螺仪消息: {sensor_counts['gyroscope']}")  
    print(f"  - 加速度计消息: {sensor_counts['accelerometer']}")  
      
    return not imu_available  
  
def analyze_can_yawrate_data(lr):  
    """分析CAN信号中的yawRate数据"""  
    print("\n=== CAN信号yawRate数据分析 ===")  
      
    car_state_msgs = [msg for msg in lr if msg.which() == 'carState']  
    if not car_state_msgs:  
        print("错误: 没有找到carState消息")  
        return None  
      
    print(f"找到 {len(car_state_msgs)} 条carState消息")  
      
    # 分析前10条消息  
    print("\n前10条carState消息:")  
    yaw_rates = []  
    speeds = []  
      
    for i, msg in enumerate(car_state_msgs[:10]):  
        yaw_rate = msg.carState.yawRate  
        speed = msg.carState.vEgo  
        yaw_rates.append(yaw_rate)  
        speeds.append(speed)  
        print(f"  {i}: Speed={speed:.2f} m/s, YawRate={yaw_rate:.6f} rad/s")  
      
    # 统计分析  
    all_yaw_rates = [msg.carState.yawRate for msg in car_state_msgs]  
    non_zero_count = sum(1 for rate in all_yaw_rates if abs(rate) > 0.001)  
      
    print(f"\n统计信息:")  
    print(f"  - 总消息数: {len(all_yaw_rates)}")  
    print(f"  - 非零yawRate消息: {non_zero_count} ({non_zero_count/len(all_yaw_rates)*100:.1f}%)")  
    print(f"  - yawRate范围: {min(all_yaw_rates):.6f} 到 {max(all_yaw_rates):.6f}")  
      
    return {  
        'has_valid_data': non_zero_count > 0,  
        'sample_yaw_rates': yaw_rates[:10],  
        'sample_speeds': speeds[:10]  
    }  
  
def analyze_locationd_output(lr):  
    """分析locationd输出数据"""  
    print("\n=== LocationD输出数据分析 ===")  
      
    location_msgs = [msg for msg in lr if msg.which() == 'liveLocationKalman']  
    if not location_msgs:  
        print("错误: 没有找到liveLocationKalman消息")  
        return None  
      
    print(f"找到 {len(location_msgs)} 条liveLocationKalman消息")  
      
    # 分析前10条消息  
    print("\n前10条liveLocationKalman消息:")  
    sensors_ok_count = 0  
    inputs_ok_count = 0  
    valid_yaw_rates = []  
      
    for i, msg in enumerate(location_msgs[:10]):  
        try:  
            sensors_ok = msg.liveLocationKalman.sensorsOK  
            inputs_ok = msg.liveLocationKalman.inputsOK  
              
            # 正确访问yawRate  
            angular_velocity = msg.liveLocationKalman.angularVelocityDevice  
            yaw_rate = angular_velocity.value[2] if hasattr(angular_velocity, 'value') and len(angular_velocity.value) > 2 else 0.0  
            valid = angular_velocity.valid if hasattr(angular_velocity, 'valid') else False  
              
            if sensors_ok:  
                sensors_ok_count += 1  
            if inputs_ok:  
                inputs_ok_count += 1  
              
            valid_yaw_rates.append(yaw_rate)  
              
            print(f"  {i}: SensorsOK={sensors_ok}, InputsOK={inputs_ok}, YawRate={yaw_rate:.6f}, Valid={valid}")  
              
        except Exception as e:  
            print(f"  {i}: 解析错误 - {e}")  
      
    # 统计分析  
    all_yaw_rates = []  
    all_sensors_ok = []  
    all_inputs_ok = []  
      
    for msg in location_msgs:  
        try:  
            all_sensors_ok.append(msg.liveLocationKalman.sensorsOK)  
            all_inputs_ok.append(msg.liveLocationKalman.inputsOK)  
              
            angular_velocity = msg.liveLocationKalman.angularVelocityDevice  
            yaw_rate = angular_velocity.value[2] if hasattr(angular_velocity, 'value') and len(angular_velocity.value) > 2 else 0.0  
            all_yaw_rates.append(yaw_rate)  
        except:  
            continue  
      
    non_zero_output_count = sum(1 for rate in all_yaw_rates if abs(rate) > 0.001)  
      
    print(f"\n统计信息:")  
    print(f"  - SensorsOK=True: {sum(all_sensors_ok)}/{len(all_sensors_ok)} ({sum(all_sensors_ok)/len(all_sensors_ok)*100:.1f}%)")  
    print(f"  - InputsOK=True: {sum(all_inputs_ok)}/{len(all_inputs_ok)} ({sum(all_inputs_ok)/len(all_inputs_ok)*100:.1f}%)")  
    print(f"  - 非零yawRate输出: {non_zero_output_count}/{len(all_yaw_rates)} ({non_zero_output_count/len(all_yaw_rates)*100:.1f}%)")  
    print(f"  - 输出yawRate范围: {min(all_yaw_rates):.6f} 到 {max(all_yaw_rates):.6f}")  
      
    return {  
        'sensors_ok_ratio': sum(all_sensors_ok)/len(all_sensors_ok) if all_sensors_ok else 0,  
        'inputs_ok_ratio': sum(all_inputs_ok)/len(all_inputs_ok) if all_inputs_ok else 0,  
        'non_zero_output_ratio': non_zero_output_count/len(all_yaw_rates) if all_yaw_rates else 0,  
        'output_yaw_rates': all_yaw_rates  
    }  
  
def verify_can_backup_function(can_data, locationd_data, imu_unavailable):  
    """验证CAN信号备用功能"""  
    print("\n=== CAN信号备用功能验证 ===")  
      
    if not can_data or not locationd_data:  
        print("错误: 缺少必要的数据进行验证")  
        return  
      
    print(f"IMU传感器不可用: {'是' if imu_unavailable else '否'}")  
    print(f"CAN信号包含有效yawRate: {'是' if can_data['has_valid_data'] else '否'}")  
    print(f"LocationD输出非零yawRate比例: {locationd_data['non_zero_output_ratio']*100:.1f}%")  
      
    # 验证逻辑  
    if imu_unavailable and can_data['has_valid_data']:  
        if locationd_data['non_zero_output_ratio'] > 0.1:  # 超过10%的输出有非零值  
            print("\n✓ CAN信号备用功能可能正常工作")  
            print("  - IMU传感器不可用时，系统使用了CAN信号数据")  
        else:  
            print("\n✗ CAN信号备用功能可能未正常工作")  
            print("  - 尽管CAN信号包含有效数据，但locationd输出全为零")  
            print("  - 建议检查以下方面:")  
            print("    1. handle_car_state函数中的CAN信号处理逻辑")  
            print("    2. IMU超时检查机制的时间同步")  
            print("    3. 陀螺仪偏差检查和交叉验证条件")  
            print("    4. 数据坐标系转换（注意负号）")  
    elif not imu_unavailable:  
        print("\n- IMU传感器可用，应优先使用硬件传感器数据")  
    else:  
        print("\n- CAN信号中没有有效的yawRate数据")  
  
def main():  
    parser = argparse.ArgumentParser(description='LocationD IMU数据调试脚本')  
    parser.add_argument('log_file', help='rlog.zst文件路径')  
    args = parser.parse_args()  
      
    try:  
        print(f"正在加载日志文件: {args.log_file}")  
        lr = LogReader(args.log_file)  
          
        # 分析传感器可用性  
        imu_unavailable = analyze_sensor_availability(lr)  
          
        # 重新加载LogReader（因为迭代器已消耗）  
        lr = LogReader(args.log_file)  
          
        # 分析CAN信号数据  
        can_data = analyze_can_yawrate_data(lr)  
          
        # 重新加载LogReader  
        lr = LogReader(args.log_file)  
          
        # 分析locationd输出  
        locationd_data = analyze_locationd_output(lr)  
          
        # 验证CAN备用功能  
        verify_can_backup_function(can_data, locationd_data, imu_unavailable)  
          
        print("\n=== 调试完成 ===")  
          
    except Exception as e:  
        print(f"错误: {e}")  
        sys.exit(1)  
  
if __name__ == "__main__":  
    main()
