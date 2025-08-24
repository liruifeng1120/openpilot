#!/usr/bin/env python3
import time
import subprocess
import os

def test_jy62_integration():
    """
    测试JY62设备与系统的集成
    """
    print("开始测试JY62设备集成")
    
    # 检查设备是否存在
    if not os.path.exists('/dev/ttyUSB0'):
        print("警告: /dev/ttyUSB0设备不存在，请检查JY62设备连接")
        return False
    
    # 检查locationd可执行文件
    locationd_path = '/home/liruifeng/ajouatom/selfdrive/locationd/locationd'
    if not os.path.exists(locationd_path):
        print("编译locationd...")
        result = subprocess.run([
            'scons', '--minimal', 'selfdrive/locationd/locationd'
        ], cwd='/home/liruifeng/ajouatom', capture_output=True, text=True)
        
        if result.returncode != 0:
            print(f"编译失败: {result.stderr}")
            return False
        else:
            print("编译成功")
    
    # 测试直接运行locationd
    print("测试直接运行locationd...")
    try:
        # 启动locationd进程（短暂运行）
        process = subprocess.Popen([
            locationd_path, 
            '--type=jy62', 
            '--device=/dev/ttyUSB0',
            '--baud=115200'
        ], cwd='/home/liruifeng/ajouatom', stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        
        # 等待几秒钟看是否有错误
        time.sleep(3)
        
        # 检查进程状态
        if process.poll() is None:
            # 进程仍在运行，说明启动成功
            print("locationd启动成功")
            process.terminate()
            process.wait()
            return True
        else:
            # 进程已退出，检查错误
            stdout, stderr = process.communicate()
            print(f"locationd启动失败:")
            print(f"stdout: {stdout.decode()}")
            print(f"stderr: {stderr.decode()}")
            return False
            
    except Exception as e:
        print(f"运行locationd时出错: {e}")
        return False

def check_system_manager_config():
    """
    检查系统管理器配置
    """
    print("检查系统管理器配置...")
    
    config_path = '/home/liruifeng/ajouatom/system/manager/process_config.py'
    if not os.path.exists(config_path):
        print("错误: 系统管理器配置文件不存在")
        return False
    
    # 检查配置文件中是否有正确的locationd配置
    with open(config_path, 'r') as f:
        content = f.read()
        
    if 'locationd.*--type=jy62' in content:
        print("✓ 系统管理器配置正确")
        return True
    else:
        print("✗ 系统管理器配置可能不正确")
        return False

if __name__ == "__main__":
    print("JY62设备集成测试")
    print("=" * 30)
    
    # 检查系统配置
    config_ok = check_system_manager_config()
    
    # 测试设备集成
    integration_ok = test_jy62_integration()
    
    print("\n测试结果:")
    print(f"系统配置: {'通过' if config_ok else '失败'}")
    print(f"设备集成: {'通过' if integration_ok else '失败'}")
    
    if config_ok and integration_ok:
        print("\n✓ JY62设备已正确集成到系统中")
        print("您可以使用桌面快捷方式启动系统")
    else:
        print("\n✗ JY62设备集成存在问题，请检查上述错误")