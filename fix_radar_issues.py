#!/usr/bin/env python3
import re
import os

def fix_radar_problems():
    print("开始修复雷达误识别问题...")
    
    radar_file = "selfdrive/controls/radard.py"
    
    # 备份文件
    backup_file = radar_file + ".backup"
    if not os.path.exists(backup_file):
        os.system(f"cp {radar_file} {backup_file}")
        print(f"已备份: {backup_file}")
    
    # 读取文件
    with open(radar_file, 'r') as f:
        content = f.read()
    
    # 查找雷达处理逻辑，添加严格过滤
    # 寻找处理雷达点的代码部分
    if "radar_points" in content and "leadOne" in content:
        print("找到雷达处理逻辑...")
        
        # 在雷达点处理前添加过滤逻辑
        pattern = r'(radar_points\s*=\s*radar\.update\(can_strings\))'
        replacement = r'''# === 严格过滤修复误识别 ===
        radar_points = radar.update(can_strings)
        radar_points = self._apply_strict_filtering(radar_points)'''
        
        content = re.sub(pattern, replacement, content)
        
        # 添加过滤方法到类中
        class_pattern = r'(class\s+\w+.*?:)(\s*def\s+__init__.*?)(?=def\s+\w+)'
        
        filter_method = '''
    def _apply_strict_filtering(self, radar_points):
        """严格过滤雷达点，修复误识别和急刹"""
        if not radar_points:
            return []
            
        filtered_points = []
        for point in radar_points:
            # 距离过滤：5-80米范围
            if not (5.0 <= point.dRel <= 80.0):
                continue
                
            # 速度过滤：避免静态物体
            if abs(point.vRel) < 0.5:  # 几乎静止的目标
                continue
                
            # 横向位置过滤：避免路边物体
            if abs(point.yRel) > 2.5:
                continue
                
            # 只保留高质量目标
            filtered_points.append(point)
            
        # 按距离排序，只返回最近的两个目标
        filtered_points.sort(key=lambda x: x.dRel)
        return filtered_points[:2]
'''
        
        # 在类的第一个方法前插入过滤方法
        if 'class RadarD' in content:
            content = content.replace('class RadarD', 'class RadarD' + filter_method)
        else:
            # 找到第一个类定义
            class_match = re.search(r'(class\s+\w+.*?:)', content)
            if class_match:
                class_def = class_match.group(1)
                content = content.replace(class_def, class_def + filter_method)
    
    # 写入修改后的文件
    with open(radar_file, 'w') as f:
        f.write(content)
    
    print("雷达误识别修复完成！")
    print("修改内容：")
    print("- 距离范围: 5-80米")
    print("- 过滤静态物体 (速度<0.5m/s)")
    print("- 横向范围: ±2.5米")
    print("- 只保留最近2个目标")

if __name__ == "__main__":
    fix_radar_problems()

