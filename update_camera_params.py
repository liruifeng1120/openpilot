#!/usr/bin/env python3

import re
import sys
import os

def update_file(file_path, pattern, replacement):
    """
    更新文件中的指定模式
    """
    if not os.path.exists(file_path):
        print(f"文件 {file_path} 不存在")
        return False

    try:
        with open(file_path, 'r') as f:
            content = f.read()

        # 使用正则表达式替换
        updated_content = re.sub(pattern, replacement, content, flags=re.MULTILINE)

        # 如果内容有变化，则写入文件
        if updated_content != content:
            with open(file_path, 'w') as f:
                f.write(updated_content)
            print(f"已更新文件: {file_path}")
            return True
        else:
            print(f"文件 {file_path} 无需更新")
            return False
    except Exception as e:
        print(f"更新文件 {file_path} 时出错: {e}")
        return False

def main():
    # 定义要修改的文件和对应的修改规则
    modifications = [
        {
            "file": "common/transformations/camera.py",# 这是文件路径
        #    这一行是替换前的参数
            "pattern": r"_ar_ox_fisheye = CameraConfig\(2592, 1944, 567\.0\)",
        #    这一行是替换后的参数
            "replacement": "_ar_ox_fisheye = CameraConfig(2592, 1944, 865.0)"
        },
        {
            "file": "selfdrive/ui/ui.h",# 这是文件路径
        #    这一行是替换前的参数
            "pattern": r"const Eigen::Matrix3f ECAM_INTRINSIC_MATRIX = \(Eigen::Matrix3f\(\) <<\s*\n\s*567\.0, 0\.0, 2592\.0 / 2,\s*\n\s*0\.0, 567\.0, 1944\.0 / 2,\s*\n\s*0\.0, 0\.0, 1\.0\)\.finished\(\);",
        #    这一行是替换后的参数
            "replacement": "const Eigen::Matrix3f FCAM_INTRINSIC_MATRIX = (Eigen::Matrix3f() <<\n  865.0, 0.0, 2592.0 / 2,\n  0.0, 865.0, 1944.0 / 2,\n  0.0, 0.0, 1.0).finished();"
        }
    ]

    # 获取项目根目录
    project_root = os.path.dirname(os.path.abspath(__file__))

    updated_count = 0
    for mod in modifications:
        file_path = os.path.join(project_root, mod["file"])
        pattern = mod["pattern"]
        replacement = mod["replacement"]

        if update_file(file_path, pattern, replacement):
            updated_count += 1

    print(f"\n完成！共更新了 {updated_count} 个文件。")

    # 检查 tools/webcam/camera.py 是否需要更新
    webcam_camera_path = os.path.join(project_root, "tools/webcam/camera.py")
    if os.path.exists(webcam_camera_path):
        print(f"\n注意：请手动检查 {webcam_camera_path} 文件，因为其中未找到需要修改的参数。")
    else:
        print(f"\n注意：{webcam_camera_path} 文件不存在。")

if __name__ == "__main__":
    main()