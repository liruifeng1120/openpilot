#!/usr/bin/env python3
import argparse
import sys
import select
import tty
import termios
import os

from typing import Any
from multiprocessing import Queue

from openpilot.tools.sim.bridge.metadrive.metadrive_bridge import MetaDriveBridge

def create_bridge(dual_camera, high_quality, config=None):
  queue: Any = Queue()

  simulator_bridge = MetaDriveBridge(dual_camera, high_quality, config=config)
  simulator_process = simulator_bridge.run(queue)

  return queue, simulator_process, simulator_bridge

def main():
  _, simulator_process, _ = create_bridge(True, False)
  simulator_process.join()

def parse_args(add_args=None):
  parser = argparse.ArgumentParser(description='Bridge between the simulator and openpilot.')
  parser.add_argument('--joystick', action='store_true')
  parser.add_argument('--high_quality', action='store_true')
  parser.add_argument('--dual_camera', action='store_true')
  parser.add_argument('--car_brand', default='BYD')
  parser.add_argument('--disable_debug', action='store_true', help='Disable debug output')

  return parser.parse_args(add_args)

if __name__ == "__main__":
  args = parse_args()

  # 设置环境变量来控制调试输出
  if args.disable_debug:
    os.environ['SIM_DISABLE_DEBUG'] = '1'
  else:
    # 默认情况下禁用调试输出
    os.environ['SIM_DISABLE_DEBUG'] = '1'

  config = {
    "vehicle_config": {"vehicle_model": args.car_brand},
  }

  queue, simulator_process, simulator_bridge = create_bridge(args.dual_camera, args.high_quality, config=config)

  if args.joystick:
    # start input poll for joystick
    from openpilot.tools.sim.lib.manual_ctrl import wheel_poll_thread

    wheel_poll_thread(queue)
  else:
    # start input poll for keyboard
    from openpilot.tools.sim.lib.keyboard_ctrl import keyboard_poll_thread

    # keyboard_poll_thread 会自己处理终端设置
    keyboard_poll_thread(queue)

  simulator_bridge.shutdown()

  simulator_process.join()