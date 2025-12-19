#!/usr/bin/env python3
import time
import cereal.messaging as messaging
from collections import defaultdict

def main():
    print("Starting Live Delay debugging...")

    # 订阅相关消息
    sm = messaging.SubMaster([
        'liveDelay', 'carState', 'carControl', 'carOutput',
        'liveLocationKalman', 'controlsState'
    ])

    with open("/data/debug/live_delay_analysis.log", "w") as f:
        f.write("Live Delay Analysis Started\n")
        f.write("=" * 50 + "\n")

    msg_counts = defaultdict(int)

    while True:
        sm.update()

        # 统计消息接收情况
        for service in sm.updated.keys():
            if sm.updated[service]:
                msg_counts[service] += 1

        # 每10秒输出一次状态
        if sm.frame % 1000 == 0:
            with open("/data/debug/live_delay_analysis.log", "a") as f:
                timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
                f.write(f"\n[{timestamp}] Status Report:\n")
                f.write(f"Frame: {sm.frame}\n")

                # Live Delay 状态
                if sm.valid['liveDelay']:
                    ld = sm['liveDelay']
                    f.write(f"LiveDelay - Valid: True, CalPerc: {ld.calPerc}%, "
                           f"LateralDelay: {ld.lateralDelay:.4f}s\n")
                else:
                    f.write(f"LiveDelay - Valid: False\n")

                # 车辆状态
                if sm.valid['carState']:
                    cs = sm['carState']
                    f.write(f"CarState - SteeringAngle: {cs.steeringAngleDeg:.2f}°, "
                           f"SteeringTorque: {cs.steeringTorque:.2f}, "
                           f"YawRate: {cs.yawRate:.4f}, VEgo: {cs.vEgo:.2f}\n")

                # 控制状态
                if sm.valid['controlsState']:
                    ctrl = sm['controlsState']
                    f.write(f"ControlsState - LatActive: {sm['carControl'].latActive if sm.valid['carControl'] else 'N/A'}\n")

                # 消息计数
                f.write("Message counts:\n")
                for service, count in msg_counts.items():
                    f.write(f"  {service}: {count}\n")

                f.write("-" * 30 + "\n")

        time.sleep(0.01)

if __name__ == "__main__":
    main()