#!/usr/bin/env python3
"""
验证LDW功能是否已被禁用的脚本
"""

import sys
import os
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from openpilot.common.params import Params
from openpilot.selfdrive.controls.lib.ldw import LaneDepartureWarning

def check_ldw_status():
    """检查LDW状态"""
    print("=== LDW功能禁用状态检查 ===\n")

    # 1. 检查参数设置
    params = Params()
    is_ldw_enabled = params.get_bool("IsLdwEnabled")
    print(f"1. 参数检查:")
    print(f"   IsLdwEnabled: {is_ldw_enabled}")

    if not is_ldw_enabled:
        print("   ✓ LDW已通过参数禁用")
    else:
        print("   ✗ LDW参数仍然启用")

    # 2. 检查LDW类功能
    print(f"\n2. LDW类功能检查:")
    ldw = LaneDepartureWarning()

    # 模拟正常的更新调用（空的模拟数据）
    class MockData:
        leftBlinker = False
        rightBlinker = False
        vEgo = 50  # 50 m/s，足够高的速度

    class MockModel:
        class Meta:
            desirePrediction = [0.8, 0.1, 0.1]  # 模拟高车道偏离概率
        meta = Meta()
        laneLineProbs = [0.9, 0.9, 0.9, 0.9]  # 模拟清晰的车道线
        class LaneLines:
            y = [2.0]  # 模拟接近的车道线
        laneLines = [LaneLines(), LaneLines(), LaneLines(), LaneLines()]

    class MockCC:
        latActive = False

    # 执行更新
    ldw.update(100, MockModel(), MockData(), MockCC())

    print(f"   左侧车道偏离警告: {ldw.left}")
    print(f"   右侧车道偏离警告: {ldw.right}")
    print(f"   总体警告状态: {ldw.warning}")

    if not ldw.warning:
        print("   ✓ LDW类功能已禁用")
    else:
        print("   ✗ LDW类功能仍然激活")

    # 3. 总结
    print(f"\n3. 总结:")
    if not is_ldw_enabled and not ldw.warning:
        print("   ✓ LDW功能已完全禁用")
        return True
    else:
        print("   ✗ LDW功能未完全禁用，请检查配置")
        return False

if __name__ == "__main__":
    success = check_ldw_status()
    sys.exit(0 if success else 1)