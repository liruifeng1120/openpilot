#pragma once

#include <QImage>
#include <QWidget>
#include <QTimer>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

// 自包含的 scrcpy 投屏 Widget：手机画面 → OP 设备 UI 显示 + 反向触摸控制
//
// 工作流程：
//   1. adb forward tcp:27183 tcp:27183
//   2. adb push scrcpy-server.jar → 手机 /data/local/tmp/
//   3. adb shell app_process 启动 com.genymobile.scrcpy.Server (后台)
//   4. 连 video socket (27183) → 读握手 → 读 codec/宽高 → 循环读 H.264 包解码
//   5. 连 control socket (27183) → 鼠标/触摸事件转 scrcpy 控制消息发回手机
//
// 交互:
//   - 顶部标题栏: 拖动移动窗口; 双击 → 发 toggleRequested() 请求隐藏
//     拖动标题栏到屏幕左/右边缘 (≤EDGE_THRESHOLD) → 最大化占满上下; 拖回还原
//   - 右下角缩放手柄: 拖动改变窗口大小
//   - 其余区域: 触摸反向控制手机
//   - setAutoConnect(true): 定时轮询 adb devices, 检测到安卓设备自动启动投屏,
//     断开后自动隐藏并继续轮询 (开机未连接时保持隐藏)
//
// 协议参考: scrcpy v3.3.4
class ScrcpyWidget : public QWidget {
  Q_OBJECT

public:
  explicit ScrcpyWidget(QWidget *parent = nullptr);
  ~ScrcpyWidget() override;

  // 启动投屏
  //   adb_path:    adb 路径, 空则默认 /home/$LOGNAME/111/scrcpy-linux-x86_64-v3.3.4/adb
  //   server_jar:  scrcpy-server jar 路径, 空则默认同目录下的 scrcpy-server
  //   max_size:    视频最大边长 (像素), 0=原始分辨率
  //   bitrate:     视频码率 (bps)
  bool start(const std::string &adb_path = "",
             const std::string &server_jar = "",
             int max_size = 1280, int bitrate = 8000000);

  // 停止投屏, 关闭 socket, kill server 进程
  void stop();

  // 自动检测安卓设备连接: 开启后定时轮询 adb devices,
  // 检测到设备自动 start(), 断开后自动隐藏并继续轮询。
  void setAutoConnect(bool on);
  bool autoConnect() const { return auto_connect_; }

  bool isRunning() const { return running_; }
  int videoWidth() const { return video_w_; }
  int videoHeight() const { return video_h_; }
  // 标题栏高度 (像素), 供外部按"视频区比例"精确计算窗口几何
  int titleBarHeight() const { return TITLE_BAR_H; }

  // 透明度 & 横屏旋转控制 (线程安全, paintEvent 读取)
  void setOpacity(int percent);
  int opacity() const { return opacity_percent_; }
  void setRotated(bool on);
  bool rotated() const { return rotated_; }

signals:
  void connected(int width, int height);
  void disconnected();
  void errorOccurred(const QString &msg);
  // 双击标题栏请求切换显示/隐藏 (由父窗口决定如何处理)
  void toggleRequested();
  // 手机屏幕旋转/分辨率变化时发出, 父窗口需重新计算 widget 尺寸
  void resolutionChanged(int width, int height);
  // 拖动到屏幕边缘触发最大化/还原
  void maximizeToggled(bool maximized);

protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void mouseDoubleClickEvent(QMouseEvent *e) override;

private:
  // 视频接收/解码线程
  void videoLoop();

  // 会话线程入口: startImpl → videoLoop → cleanupResources (在 video_thread_ 中运行)
  void runSession();

  // start() 的实际实现 (start() 负责并发保护)
  bool startImpl(const std::string &adb_path, const std::string &server_jar,
                 int max_size, int bitrate);

  // 释放 socket / server 进程 / FFmpeg 资源 (幂等, 可在 videoLoop 退出时调用)
  void cleanupResources();

  // adb 辅助
  static bool runCmd(const std::string &cmd, std::string *out = nullptr, bool quiet = false);
  bool setupForward(const std::string &adb_path);
  bool pushServer(const std::string &adb_path, const std::string &server_jar);
  bool startServer(const std::string &adb_path, int max_size, int bitrate);
  void killServer();
  void removeForward(const std::string &adb_path);

  // socket
  static int connectSocket(int port, int retries = 100, int retry_delay_ms = 100);
  static bool recvAll(int fd, void *buf, size_t len);

  // 大端读取
  static uint64_t readBE64(const uint8_t *p);
  static uint32_t readBE32(const uint8_t *p);

  // 发送控制消息
  void sendTouch(int action, int x, int y);
  void sendToControl(const void *data, size_t len);

  // 将 widget 坐标映射到手机屏幕坐标
  void mapToScreen(int widget_x, int widget_y, int *screen_x, int *screen_y);

  // 计算视频在 widget 内的实际绘制矩形 (保持手机画面比例, 居中, 不拉伸)
  QRect videoDisplayRect() const;

  // 窗口拖动 / 缩放
  enum DragMode { None = 0, Move, Resize };
  bool inTitleBar(int x, int y) const;
  bool inResizeGrip(int x, int y) const;
  void updateHoverCursor(int x, int y);

  // 自动设备检测
  bool detectDevice();
  void onAutoTimer();

  std::thread video_thread_;
  std::atomic<bool> running_{false};
  std::atomic<bool> stopping_{false};
  std::atomic<bool> starting_{false};  // 防止并发 start
  std::atomic<bool> detecting_{false}; // 正在执行 adb devices 检测

  int video_sock_ = -1;
  int control_sock_ = -1;
  std::mutex control_mtx_;
  std::mutex frame_mtx_;
  std::mutex lifecycle_mtx_;  // 保护 start/cleanup 串行化, 避免自动重连与清理竞争
  QImage frame_;

  int video_w_ = 0;
  int video_h_ = 0;
  int opacity_percent_ = 100;
  bool rotated_ = false;

  std::string adb_path_ = "adb";  // 保存 adb 路径, stop() 时用
  char scid_str_[16] = {0};       // scrcpy session id, 用于 reverse localabstract 名字

  // 会话配置 (由 start() 保存, runSession 使用)
  std::string cfg_adb_;
  std::string cfg_jar_;
  int cfg_max_size_ = 1280;
  int cfg_bitrate_ = 8000000;

  static constexpr int LOCAL_PORT = 27183;
  static constexpr int DEVICE_NAME_LEN = 64;
  pid_t server_pid_ = -1;
  bool forward_set_ = false;

  // FFmpeg 解码器 (void* 避免头文件依赖 FFmpeg)
  void *decoder_ = nullptr;    // AVCodecContext*
  void *avframe_ = nullptr;    // AVFrame*
  void *avpacket_ = nullptr;   // AVPacket*
  uint8_t *rgb_buf_ = nullptr; // RGB24 输出缓冲
  int rgb_buf_size_ = 0;

  // 拖动/缩放状态
  DragMode drag_mode_ = None;
  QPoint drag_last_pos_;
  QRect drag_start_geom_;

  // 最大化显示 (拖到屏幕边缘触发)
  bool maximized_ = false;
  QRect saved_geo_;
  static constexpr int EDGE_THRESHOLD = 20;

  // 自动连接
  bool auto_connect_ = false;
  QTimer *auto_timer_ = nullptr;

  // 标题栏 & 缩放手柄尺寸
  static constexpr int TITLE_BAR_H = 28;
  static constexpr int RESIZE_GRIP = 22;
};
