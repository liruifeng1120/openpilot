#!/usr/bin/env python3
"""
启用完整的 dashy 模式 (含视频流功能)
"""

from openpilot.common.params import Params

def main():
  print("启用完整的 dashy 模式 (含视频流功能)...")
  Params().put("dp_dev_dashy", "2")
  print("完成！可以访问 http://<设备IP>:5088 使用 dashy")

if __name__ == "__main__":
  main()
