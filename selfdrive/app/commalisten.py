#!/usr/bin/env python3
import json
import socket
import sys
import threading
import time
from datetime import datetime
import argparse
import traceback

# 处理Windows环境
try:
    import curses
except ImportError:
    try:
        # Windows平台上尝试导入windows-curses
        import windows_curses as curses
    except ImportError:
        print("无法导入curses模块。在Windows上需要安装windows-curses包。")
        print("请运行: pip install windows-curses")
        sys.exit(1)

class CommaListener:
    def __init__(self, port=8088):
        """初始化接收器"""
        self.port = port
        self.data = {}
        self.last_update = 0
        self.running = True
        self.device_ip = None

    def start_listening(self):
        """启动UDP监听"""
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        try:
            sock.bind(('0.0.0.0', self.port))
            print(f"正在监听端口 {self.port} 的广播数据...")

            while self.running:
                try:
                    data, addr = sock.recvfrom(4096)
                    self.device_ip = addr[0]
                    try:
                        self.data = json.loads(data.decode('utf-8'))
                        self.last_update = time.time()
                    except json.JSONDecodeError:
                        print(f"接收到无效的JSON数据: {data[:100]}...")
                except Exception as e:
                    print(f"接收数据时出错: {e}")
        except Exception as e:
            print(f"无法绑定到端口 {self.port}: {e}")
        finally:
            sock.close()

class UI:
    def __init__(self, listener):
        """初始化UI"""
        self.listener = listener
        self.stdscr = None
        self.running = True
        self.current_page = 0
        self.pages = ["车辆信息", "设备信息", "位置信息", "扩展车辆信息", "原始数据"]
        self.max_pages = len(self.pages)

        # 颜色配对
        self.COLOR_NORMAL = 1
        self.COLOR_HIGHLIGHT = 2
        self.COLOR_STATUS_OK = 3
        self.COLOR_STATUS_WARNING = 4
        self.COLOR_STATUS_ERROR = 5
        self.COLOR_HEADER = 6
        self.COLOR_DATA = 7
        self.COLOR_TITLE = 8

        # 布局参数
        self.max_height = 0
        self.max_width = 0

    def run(self):
        """运行UI"""
        curses.wrapper(self.main)

    def main(self, stdscr):
        """主UI循环"""
        self.stdscr = stdscr
        self.init_colors()

        curses.curs_set(0)  # 隐藏光标
        self.stdscr.timeout(100)  # 设置getch非阻塞超时

        while self.running:
            self.stdscr.clear()
            self.max_height, self.max_width = self.stdscr.getmaxyx()

            # 处理按键
            self.handle_input()

            # 绘制界面
            self.draw_header()
            self.draw_status()

            # 根据当前页面绘制内容
            if self.current_page == 0:
                self.draw_car_info()
            elif self.current_page == 1:
                self.draw_device_info()
            elif self.current_page == 2:
                self.draw_location_info()
            elif self.current_page == 3:
                self.draw_extended_car_info()
            elif self.current_page == 4:
                self.draw_raw_data()

            self.draw_footer()

            self.stdscr.refresh()
            time.sleep(0.1)

    def init_colors(self):
        """初始化颜色配对"""
        curses.start_color()
        curses.use_default_colors()
        curses.init_pair(self.COLOR_NORMAL, curses.COLOR_WHITE, -1)
        curses.init_pair(self.COLOR_HIGHLIGHT, curses.COLOR_BLACK, curses.COLOR_WHITE)
        curses.init_pair(self.COLOR_STATUS_OK, curses.COLOR_GREEN, -1)
        curses.init_pair(self.COLOR_STATUS_WARNING, curses.COLOR_YELLOW, -1)
        curses.init_pair(self.COLOR_STATUS_ERROR, curses.COLOR_RED, -1)
        curses.init_pair(self.COLOR_HEADER, curses.COLOR_CYAN, -1)
        curses.init_pair(self.COLOR_DATA, curses.COLOR_GREEN, -1)
        curses.init_pair(self.COLOR_TITLE, curses.COLOR_MAGENTA, -1)

    def handle_input(self):
        """处理按键"""
        key = self.stdscr.getch()
        if key == ord('q'):
            self.running = False
        elif key == ord('n') or key == curses.KEY_RIGHT:
            self.current_page = (self.current_page + 1) % self.max_pages
        elif key == ord('p') or key == curses.KEY_LEFT:
            self.current_page = (self.current_page - 1) % self.max_pages

    def draw_header(self):
        """绘制顶部标题栏"""
        title = "CommaAssist 数据监视器"
        x = max(0, (self.max_width - len(title)) // 2)
        self.stdscr.attron(curses.color_pair(self.COLOR_HEADER) | curses.A_BOLD)
        self.safe_addstr(0, x, title)
        self.stdscr.attroff(curses.color_pair(self.COLOR_HEADER) | curses.A_BOLD)

        # 绘制页面标签
        self.safe_addstr(1, 0, " " * (self.max_width - 1), curses.color_pair(self.COLOR_HIGHLIGHT))
        x = 2
        for i, page in enumerate(self.pages):
            if i == self.current_page:
                self.safe_addstr(1, x, f" {page} ", curses.color_pair(self.COLOR_HIGHLIGHT) | curses.A_BOLD)
            else:
                self.safe_addstr(1, x, f" {page} ", curses.color_pair(self.COLOR_NORMAL))
            x += len(page) + 3

    def draw_status(self):
        """绘制状态栏"""
        current_time = time.time()
        status_line = 2

        if not self.listener.data:
            self.safe_addstr(status_line, 0, "等待数据...", curses.color_pair(self.COLOR_STATUS_WARNING))
            return

        time_diff = current_time - self.listener.last_update
        if time_diff > 5:
            self.safe_addstr(status_line, 0, f"数据已过期! 上次更新: {datetime.fromtimestamp(self.listener.last_update).strftime('%H:%M:%S')}",
                           curses.color_pair(self.COLOR_STATUS_ERROR))
        else:
            self.safe_addstr(status_line, 0, f"已连接到 {self.listener.device_ip} - 最后更新: {datetime.fromtimestamp(self.listener.last_update).strftime('%H:%M:%S')}",
                           curses.color_pair(self.COLOR_STATUS_OK))

    def draw_footer(self):
        """绘制底部控制栏"""
        footer = "操作: [q]退出 [←/p]上一页 [→/n]下一页"
        y = self.max_height - 1
        x = max(0, (self.max_width - len(footer)) // 2)

        # 防止超出屏幕边界
        try:
            # 修复：确保不超出屏幕边界
            if y > 0 and self.max_width > 0:
                # 使用safe_addstr方法避免边界问题
                self.safe_addstr(y, 0, " " * (self.max_width - 1), curses.color_pair(self.COLOR_HIGHLIGHT))
                self.safe_addstr(y, x, footer[:self.max_width - x - 1], curses.color_pair(self.COLOR_HIGHLIGHT))
        except curses.error:
            # 忽略curses边界错误
            pass

    def safe_addstr(self, y, x, text, attr=0):
        """安全地添加字符串，避免边界问题"""
        try:
            # 确保y和x是有效坐标
            height, width = self.stdscr.getmaxyx()
            if y < 0 or y >= height or x < 0:
                return

            # 计算可以显示的最大长度
            max_len = min(len(text), width - x - 1)
            if max_len <= 0:
                return

            # 确保不会写入最后一个字符位置
            self.stdscr.addstr(y, x, text[:max_len], attr)
        except curses.error:
            # 捕获并忽略curses错误
            pass

    def draw_car_info(self):
        """绘制车辆信息"""
        if not self.listener.data or 'car' not in self.listener.data:
            self.draw_no_data("车辆数据不可用")
            return

        car = self.listener.data.get('car', {})

        self.draw_section_title("基本车辆信息", 4)

        # 速度和档位
        data = [
            ("当前速度", f"{car.get('speed', 0):.1f} km/h"),
            ("巡航速度", f"{car.get('cruise_speed', 0):.1f} km/h"),
            ("档位", car.get('gear_shifter', 'Unknown')),
            ("车门状态", "打开" if car.get('door_open', False) else "关闭"),
        ]
        self.draw_data_section(data, 5, 0, self.max_width // 2)

        # 方向盘和踏板
        data = [
            ("方向盘角度", f"{car.get('steering_angle', 0):.1f}°"),
            ("转向力矩", f"{car.get('steering_torque', 0):.1f} Nm"),
            ("制动踏板", "已踩下" if car.get('brake_pressed', False) else "释放"),
            ("油门踏板", "已踩下" if car.get('gas_pressed', False) else "释放"),
        ]
        self.draw_data_section(data, 5, self.max_width // 2, self.max_width // 2)

        # 转向灯状态
        self.draw_section_title("信号灯状态", 10)
        left_blinker = car.get('left_blinker', False)
        right_blinker = car.get('right_blinker', False)

        blinker_status = "无"
        if left_blinker and right_blinker:
            blinker_status = "双闪"
        elif left_blinker:
            blinker_status = "左转"
        elif right_blinker:
            blinker_status = "右转"

        self.safe_addstr(11, 2, f"转向灯: {blinker_status}", curses.color_pair(self.COLOR_DATA))

    def draw_device_info(self):
        """绘制设备信息"""
        if not self.listener.data or 'device' not in self.listener.data:
            self.draw_no_data("设备数据不可用")
            return

        device = self.listener.data.get('device', {})

        self.draw_section_title("设备状态", 4)

        # 设备基本信息
        data = [
            ("IP地址", device.get('ip', 'Unknown')),
            ("内存使用", f"{device.get('mem_usage', 0):.1f}%"),
            ("CPU温度", f"{device.get('cpu_temp', 0):.1f}°C"),
            ("可用空间", f"{device.get('free_space', 0):.1f}%"),
        ]
        self.draw_data_section(data, 5, 0, self.max_width // 2)

        # 电池信息
        battery = device.get('battery', {})
        data = [
            ("电池电量", f"{battery.get('percent', 0)}%"),
            ("电池电压", f"{battery.get('voltage', 0):.2f}V"),
            ("电池电流", f"{battery.get('status', 0):.2f}A"),
            ("充电状态", "充电中" if not battery.get('charging', False) else "未充电"),
        ]
        self.draw_data_section(data, 5, self.max_width // 2, self.max_width // 2)

    def draw_location_info(self):
        """绘制位置信息"""
        if not self.listener.data or 'location' not in self.listener.data:
            self.draw_no_data("位置数据不可用")
            return

        location = self.listener.data.get('location', {})

        if not location.get('gps_valid', False):
            self.draw_section_title("GPS状态", 4)
            self.safe_addstr(5, 2, "GPS信号无效或未获取", curses.color_pair(self.COLOR_STATUS_ERROR))
            return

        self.draw_section_title("GPS位置", 4)

        # GPS基本信息
        data = [
            ("纬度", f"{location.get('latitude', 0):.6f}"),
            ("经度", f"{location.get('longitude', 0):.6f}"),
            ("海拔", f"{location.get('altitude', 0):.1f}m"),
            ("GPS精度", f"{location.get('accuracy', 0):.1f}m"),
        ]
        self.draw_data_section(data, 5, 0, self.max_width // 2)

        # 运动信息
        data = [
            ("方向", f"{location.get('bearing', 0):.1f}°"),
            ("GPS速度", f"{location.get('speed', 0):.1f}km/h"),
        ]
        self.draw_data_section(data, 5, self.max_width // 2, self.max_width // 2)

        # 导航信息
        if 'navigation' in self.listener.data:
            nav = self.listener.data.get('navigation', {})

            self.draw_section_title("导航信息", 10)

            # 格式化距离
            dist = nav.get('distance_remaining', 0)
            if dist > 1000:
                dist_str = f"{dist/1000:.1f}km"
            else:
                dist_str = f"{dist:.0f}m"

            # 格式化时间
            time_sec = nav.get('time_remaining', 0)
            minutes = int(time_sec // 60)
            seconds = int(time_sec % 60)
            time_str = f"{minutes}分{seconds}秒"

            data = [
                ("剩余距离", dist_str),
                ("剩余时间", time_str),
                ("道路限速", f"{nav.get('speed_limit', 0):.1f}km/h"),
            ]
            self.draw_data_section(data, 11, 0, self.max_width // 2)

            if nav.get('maneuver_distance', 0) > 0:
                data = [
                    ("下一动作", nav.get('maneuver_text', '')),
                    ("动作距离", f"{nav.get('maneuver_distance', 0)}m"),
                ]
                self.draw_data_section(data, 11, self.max_width // 2, self.max_width // 2)

    def draw_extended_car_info(self):
        """绘制扩展车辆信息"""
        if not self.listener.data or 'car_info' not in self.listener.data:
            self.draw_no_data("扩展车辆数据不可用")
            return

        car_info = self.listener.data.get('car_info', {})

        # 绘制基本信息
        if 'basic' in car_info:
            basic = car_info.get('basic', {})
            self.draw_section_title("车辆基本信息", 4)

            data = [
                ("车型", basic.get('car_model', 'Unknown')),
                ("车辆指纹", basic.get('fingerprint', 'Unknown')),
                ("重量", basic.get('weight', 'Unknown')),
                ("轴距", basic.get('wheelbase', 'Unknown')),
                ("转向比", basic.get('steering_ratio', 'Unknown')),
            ]
            self.draw_data_section(data, 5, 0, self.max_width)

        # 绘制详细车辆信息
        if 'details' not in car_info:
            return

        details = car_info.get('details', {})
        row = 10

        # 巡航控制信息
        if 'cruise' in details:
            cruise = details.get('cruise', {})
            self.draw_section_title("巡航控制", row)
            row += 1

            data = [
                ("巡航状态", "开启" if cruise.get('enabled', False) else "关闭"),
                ("自适应巡航", "可用" if cruise.get('available', False) else "不可用"),
                ("设定速度", f"{cruise.get('speed', 0):.1f}km/h"),
            ]

            if 'gap' in cruise:
                data.append(("跟车距离", str(cruise.get('gap', 0))))

            self.draw_data_section(data, row, 0, self.max_width)
            row += len(data) + 1

        # 车轮速度
        if 'wheel_speeds' in details:
            ws = details.get('wheel_speeds', {})
            self.draw_section_title("车轮速度", row)
            row += 1

            data = [
                ("左前", f"{ws.get('fl', 0):.1f}km/h"),
                ("右前", f"{ws.get('fr', 0):.1f}km/h"),
                ("左后", f"{ws.get('rl', 0):.1f}km/h"),
                ("右后", f"{ws.get('rr', 0):.1f}km/h"),
            ]
            self.draw_data_section(data, row, 0, self.max_width // 2)
            row += len(data) + 1

        # 安全系统
        if 'safety_systems' in details and details['safety_systems']:
            ss = details.get('safety_systems', {})
            self.draw_section_title("安全系统", row)
            row += 1

            data = []
            if 'esp_disabled' in ss:
                data.append(("ESP状态", "禁用" if ss.get('esp_disabled', False) else "正常"))
            if 'abs_active' in ss:
                data.append(("ABS状态", "激活" if ss.get('abs_active', False) else "正常"))
            if 'tcs_active' in ss:
                data.append(("牵引力控制", "激活" if ss.get('tcs_active', False) else "正常"))
            if 'collision_warning' in ss:
                data.append(("碰撞警告", "警告" if ss.get('collision_warning', False) else "正常"))

            if data:
                self.draw_data_section(data, row, 0, self.max_width // 2)
                row += len(data) + 1

        # 盲点检测
        if 'blind_spot' in details and details['blind_spot']:
            bs = details.get('blind_spot', {})
            self.draw_section_title("盲点监测", row)
            row += 1

            data = []
            if 'left' in bs:
                data.append(("左侧", "检测到车辆" if bs.get('left', False) else "无车辆"))
            if 'right' in bs:
                data.append(("右侧", "检测到车辆" if bs.get('right', False) else "无车辆"))

            if data:
                self.draw_data_section(data, row, 0, self.max_width // 2)
                row += len(data) + 1

        # 其他信息
        if 'other' in details and details['other']:
            other = details.get('other', {})
            self.draw_section_title("其他信息", row)
            row += 1

            data = []
            if 'outside_temp' in other:
                data.append(("车外温度", f"{other.get('outside_temp', 0):.1f}°C"))
            if 'fuel_range' in other:
                data.append(("续航里程", f"{other.get('fuel_range', 0):.1f}km"))
            if 'odometer' in other:
                data.append(("里程表", f"{other.get('odometer', 0):.1f}km"))
            if 'fuel_consumption' in other:
                data.append(("油耗", f"{other.get('fuel_consumption', 0):.1f}L/100km"))

            if data:
                self.draw_data_section(data, row, 0, self.max_width // 2)

    def draw_raw_data(self):
        """绘制原始数据"""
        if not self.listener.data:
            self.draw_no_data("没有数据")
            return

        self.draw_section_title("原始JSON数据", 4)

        try:
            json_str = json.dumps(self.listener.data, indent=2)
            lines = json_str.split('\n')

            for i, line in enumerate(lines):
                if 4 + i + 1 >= self.max_height - 1:  # 保留底部状态栏
                    self.safe_addstr(4 + i, 0, "... (内容过多无法完全显示)", curses.color_pair(self.COLOR_STATUS_WARNING))
                    break
                if len(line) > self.max_width:
                    line = line[:self.max_width - 3] + "..."
                self.safe_addstr(4 + i + 1, 0, line)
        except Exception as e:
            self.safe_addstr(5, 0, f"无法显示JSON数据: {e}", curses.color_pair(self.COLOR_STATUS_ERROR))

    def draw_no_data(self, message):
        """绘制无数据消息"""
        y = self.max_height // 2
        x = max(0, (self.max_width - len(message)) // 2)
        self.safe_addstr(y, x, message, curses.color_pair(self.COLOR_STATUS_WARNING))

    def draw_section_title(self, title, row):
        """绘制区域标题"""
        self.safe_addstr(row, 0, title, curses.color_pair(self.COLOR_TITLE) | curses.A_BOLD)

    def draw_data_section(self, data_list, start_row, start_col, width):
        """绘制数据区域"""
        for i, (label, value) in enumerate(data_list):
            row = start_row + i

            if row >= self.max_height - 1:  # 避免超出屏幕底部
                break

            self.safe_addstr(row, start_col + 2, f"{label}: ", curses.color_pair(self.COLOR_NORMAL))
            try:
                self.stdscr.addstr(value, curses.color_pair(self.COLOR_DATA))
            except curses.error:
                pass

def main():
    """主函数"""
    parser = argparse.ArgumentParser(description='CommaAssist 终端监控器')
    parser.add_argument('-p', '--port', type=int, default=8088, help='监听端口(默认: 8088)')
    args = parser.parse_args()

    # 创建并启动监听器
    listener = CommaListener(port=args.port)
    listener_thread = threading.Thread(target=listener.start_listening)
    listener_thread.daemon = True
    listener_thread.start()

    # 创建并运行UI
    ui = UI(listener)
    try:
        ui.run()
    except KeyboardInterrupt:
        pass
    finally:
        listener.running = False
        print("正在退出...")

if __name__ == "__main__":
    main()