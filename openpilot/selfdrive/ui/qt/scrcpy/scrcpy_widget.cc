#include "selfdrive/ui/qt/scrcpy/scrcpy_widget.h"

#include <QPainter>
#include <QTransform>
#include <QMouseEvent>
#include <QThread>
#include <QTimer>
#include <QCursor>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <vector>
#include <algorithm>

#include <QPainterPath>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libyuv/convert_argb.h>
}

// ===== scrcpy 控制消息常量 (v3.3.4) =====
// 消息类型
enum ScrcpyControlType {
  TYPE_INJECT_KEYCODE = 0,
  TYPE_INJECT_TEXT = 1,
  TYPE_INJECT_TOUCH_EVENT = 2,
  TYPE_INJECT_SCROLL_EVENT = 3,
  TYPE_BACK_OR_SCREEN_ON = 4,
};

// MotionEvent action
enum MotionEventAction {
  ACTION_DOWN = 0,
  ACTION_UP = 1,
  ACTION_MOVE = 2,
};

// 触摸事件: 32 字节
// [type:1][action:1][pointer_id:8][x:4][y:4][w:2][h:2][pressure:2][action_button:4][buttons:4]
struct __attribute__((packed)) TouchEventMsg {
  uint8_t type;
  uint8_t action;
  uint64_t pointer_id;  // big-endian
  int32_t x;            // big-endian
  int32_t y;            // big-endian
  uint16_t width;       // big-endian
  uint16_t height;      // big-endian
  uint16_t pressure;    // big-endian
  uint32_t action_button; // big-endian
  uint32_t buttons;       // big-endian
};

// ===== 大端写入辅助 =====
static void writeBE64(uint8_t *p, uint64_t v) {
  for (int i = 7; i >= 0; i--) { p[i] = v & 0xff; v >>= 8; }
}
static void writeBE32(uint8_t *p, uint32_t v) {
  p[0] = (v >> 24) & 0xff; p[1] = (v >> 16) & 0xff;
  p[2] = (v >> 8) & 0xff;  p[3] = v & 0xff;
}
static void writeBE16(uint8_t *p, uint16_t v) {
  p[0] = (v >> 8) & 0xff; p[1] = v & 0xff;
}

// ===== 构造/析构 =====
ScrcpyWidget::ScrcpyWidget(QWidget *parent) : QWidget(parent) {
  // 透明背景 + 允许非矩形形状
  setAttribute(Qt::WA_TranslucentBackground);
  setFocusPolicy(Qt::StrongFocus);
  setMouseTracking(true);  // 鼠标悬停时也接收 move 事件, 用于切换光标
  setMinimumSize(120, 90);
  setCursor(Qt::ArrowCursor);

  // 解析 bundled adb 路径, 供 detectDevice() 轮询使用
  const char *logname = getenv("LOGNAME");
  std::string base = logname ? std::string("/home/") + logname + "/111/scrcpy-linux-x86_64-v3.3.4" : ".";
  adb_path_ = base + "/adb";
}

void ScrcpyWidget::setOpacity(int percent) {
  int clamped = std::max(0, std::min(100, percent));
  if (clamped != opacity_percent_) {
    opacity_percent_ = clamped;
    update();
  }
}

void ScrcpyWidget::setRotated(bool on) {
  if (maximized_) return;  // 最大化模式固定竖屏
  rotated_ = on;
  update();
}

ScrcpyWidget::~ScrcpyWidget() {
  stop();
}

// ===== 公开接口 =====
// start() 仅做并发保护并启动会话线程; 实际 setup 在 startImpl(),
// 视频循环在 videoLoop(), 资源清理在 cleanupResources() —— 全部在同一个
// video_thread_ 中完成, 避免多线程生命周期竞争。
bool ScrcpyWidget::start(const std::string &adb_path,
                         const std::string &server_jar,
                         int max_size, int bitrate) {
  if (running_ || starting_) return running_;
  // 保存配置供 runSession 使用
  cfg_adb_ = adb_path;
  cfg_jar_ = server_jar;
  cfg_max_size_ = max_size;
  cfg_bitrate_ = bitrate;
  stopping_ = false;
  starting_ = true;
  video_thread_ = std::thread(&ScrcpyWidget::runSession, this);
  return true;
}

// 会话线程入口: setup → 视频循环 → 清理
void ScrcpyWidget::runSession() {
  bool ok = startImpl(cfg_adb_, cfg_jar_, cfg_max_size_, cfg_bitrate_);
  if (ok) {
    videoLoop();
  }
  // 清理 (幂等): 关 socket / kill server / 释放 FFmpeg
  {
    std::lock_guard<std::mutex> lk(lifecycle_mtx_);
    cleanupResources();
  }
  running_ = false;
  starting_ = false;
  // 自然断开 (非 stop() 驱动) 时通知 UI; stop() 路径自己 emit
  if (!stopping_) {
    stopping_ = false;
    emit disconnected();
    fprintf(stderr, "[scrcpy] session ended (natural disconnect)\n");
  }
}

bool ScrcpyWidget::startImpl(const std::string &adb_path,
                         const std::string &server_jar,
                         int max_size, int bitrate) {
  if (running_) return true;
  stopping_ = false;

  // 路径解析优先级: 参数 > 默认值 (基于 $LOGNAME 拼路径)
  std::string adb = adb_path;
  std::string jar = server_jar;
  if (adb.empty() || jar.empty()) {
    const char *logname = getenv("LOGNAME");
    std::string base = logname ? std::string("/home/") + logname + "/111/scrcpy-linux-x86_64-v3.3.4" : ".";
    if (adb.empty()) adb = base + "/adb";
    if (jar.empty()) jar = base + "/scrcpy-server";
  }

  fprintf(stderr, "[scrcpy] starting: adb=%s jar=%s max_size=%d bitrate=%d\n",
         adb.c_str(), jar.c_str(), max_size, bitrate);
  adb_path_ = adb;

  // 1. adb forward
  if (!setupForward(adb)) {
    emit errorOccurred("adb forward 失败，请确认 adb 可用且手机已连接");
    return false;
  }

  // 2. push server jar
  if (!pushServer(adb, jar)) {
    emit errorOccurred("adb push scrcpy-server 失败");
    removeForward(adb);
    return false;
  }

  // 3. 启动 server (后台)
  if (!startServer(adb, max_size, bitrate)) {
    emit errorOccurred("启动 scrcpy server 失败");
    removeForward(adb);
    return false;
  }

  // 4. reverse tunnel 模式: listen 等待 server 连接
  fprintf(stderr, "[scrcpy] listening for video socket...\n");
  int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) {
    emit errorOccurred("创建 listen socket 失败");
    killServer();
    removeForward(adb);
    return false;
  }
  int reuse = 1;
  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
  struct sockaddr_in listen_addr;
  memset(&listen_addr, 0, sizeof(listen_addr));
  listen_addr.sin_family = AF_INET;
  listen_addr.sin_port = htons(LOCAL_PORT);
  inet_pton(AF_INET, "127.0.0.1", &listen_addr.sin_addr);
  if (bind(listen_fd, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) < 0 ||
      listen(listen_fd, 8) < 0) {
    fprintf(stderr, "[scrcpy] bind/listen 失败: %s\n", strerror(errno));
    ::close(listen_fd);
    killServer();
    removeForward(adb);
    return false;
  }

  // 等待 video socket 连接 (带超时, 避免 stop() 时无限阻塞)
  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(listen_fd, &rfds);
  struct timeval tv;
  tv.tv_sec = 15;
  tv.tv_usec = 0;
  int sel = select(listen_fd + 1, &rfds, nullptr, nullptr, &tv);
  if (sel <= 0) {
    emit errorOccurred("等待 video socket 连接超时");
    ::close(listen_fd);
    killServer();
    removeForward(adb);
    return false;
  }
  video_sock_ = accept(listen_fd, nullptr, nullptr);
  if (video_sock_ < 0) {
    emit errorOccurred("accept video socket 失败");
    ::close(listen_fd);
    killServer();
    removeForward(adb);
    return false;
  }
  fprintf(stderr, "[scrcpy] video socket accepted\n");

  // reverse 模式没有 dummy byte, 直接读设备名 (64 bytes)
  char device_name[DEVICE_NAME_LEN + 1] = {0};
  if (!recvAll(video_sock_, device_name, DEVICE_NAME_LEN)) {
    emit errorOccurred("读取设备名失败");
    ::close(listen_fd);
    killServer();
    removeForward(adb);
    video_sock_ = -1;
    return false;
  }
  fprintf(stderr, "[scrcpy] device: %s\n", device_name);

  // control socket 必须在 codec header 之前连接, 否则 server 不发视频数据
  fprintf(stderr, "[scrcpy] listening for control socket...\n");
  FD_ZERO(&rfds);
  FD_SET(listen_fd, &rfds);
  tv.tv_sec = 15;
  tv.tv_usec = 0;
  sel = select(listen_fd + 1, &rfds, nullptr, nullptr, &tv);
  if (sel <= 0) {
    fprintf(stderr, "[scrcpy] WARNING: control socket 等待超时, 反向控制不可用\n");
    control_sock_ = -1;
  } else {
    control_sock_ = accept(listen_fd, nullptr, nullptr);
    if (control_sock_ < 0) {
      fprintf(stderr, "[scrcpy] WARNING: control socket 连接失败, 反向控制不可用\n");
    } else {
      int flag = 1;
      setsockopt(control_sock_, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
      fprintf(stderr, "[scrcpy] control socket accepted\n");
    }
  }
  ::close(listen_fd);

  // 读 codec_id (4) + width (4) + height (4)
  uint8_t hdr[12];
  if (!recvAll(video_sock_, hdr, 12)) {
    emit errorOccurred("读取视频头信息失败");
    killServer();
    removeForward(adb);
    if (video_sock_ >= 0) { ::close(video_sock_); video_sock_ = -1; }
    if (control_sock_ >= 0) { ::close(control_sock_); control_sock_ = -1; }
    return false;
  }
  uint32_t codec_id = readBE32(hdr);
  video_w_ = (int)readBE32(hdr + 4);
  video_h_ = (int)readBE32(hdr + 8);

  if (codec_id == 0x68323634) {
    fprintf(stderr, "[scrcpy] codec: H.264, %dx%d\n", video_w_, video_h_);
  } else if (codec_id == 0x68323635) {
    fprintf(stderr, "[scrcpy] codec: H.265, %dx%d\n", video_w_, video_h_);
  } else {
    fprintf(stderr, "[scrcpy] unknown codec: 0x%08x\n", codec_id);
  }

  if (video_w_ <= 0 || video_h_ <= 0) {
    emit errorOccurred("无效的视频尺寸");
    killServer();
    removeForward(adb);
    if (video_sock_ >= 0) { ::close(video_sock_); video_sock_ = -1; }
    if (control_sock_ >= 0) { ::close(control_sock_); control_sock_ = -1; }
    return false;
  }

  // 5. 初始化 FFmpeg 解码器
  const AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_H264);
  if (!codec) {
    emit errorOccurred("找不到 H.264 解码器");
    killServer();
    removeForward(adb);
    if (video_sock_ >= 0) { ::close(video_sock_); video_sock_ = -1; }
    if (control_sock_ >= 0) { ::close(control_sock_); control_sock_ = -1; }
    return false;
  }
  decoder_ = avcodec_alloc_context3(codec);
  avframe_ = av_frame_alloc();
  avpacket_ = av_packet_alloc();
  if (!decoder_ || !avframe_ || !avpacket_) {
    emit errorOccurred("FFmpeg 初始化失败");
    killServer();
    removeForward(adb);
    if (video_sock_ >= 0) { ::close(video_sock_); video_sock_ = -1; }
    if (control_sock_ >= 0) { ::close(control_sock_); control_sock_ = -1; }
    return false;
  }
  if (avcodec_open2((AVCodecContext *)decoder_, codec, nullptr) < 0) {
    emit errorOccurred("FFmpeg 打开解码器失败");
    killServer();
    removeForward(adb);
    if (video_sock_ >= 0) { ::close(video_sock_); video_sock_ = -1; }
    if (control_sock_ >= 0) { ::close(control_sock_); control_sock_ = -1; }
    return false;
  }

  // 预分配 RGB 缓冲
  rgb_buf_size_ = video_w_ * video_h_ * 3;
  rgb_buf_ = (uint8_t *)malloc(rgb_buf_size_);
  if (!rgb_buf_) {
    emit errorOccurred("RGB 缓冲分配失败");
    killServer();
    removeForward(adb);
    if (video_sock_ >= 0) { ::close(video_sock_); video_sock_ = -1; }
    if (control_sock_ >= 0) { ::close(control_sock_); control_sock_ = -1; }
    return false;
  }

  // 6. 标记运行, UI 线程收到 connected 信号后显示窗口
  running_ = true;
  emit connected(video_w_, video_h_);
  fprintf(stderr, "[scrcpy] started successfully\n");
  return true;
}

void ScrcpyWidget::stop() {
  // 标记停止, 关闭 socket 让 videoLoop 退出, join 会话线程, 清理资源
  stopping_ = true;
  running_ = false;

  // 关闭 socket 让 videoLoop 退出
  if (video_sock_ >= 0) { ::shutdown(video_sock_, SHUT_RDWR); ::close(video_sock_); video_sock_ = -1; }
  {
    std::lock_guard<std::mutex> lk(control_mtx_);
    if (control_sock_ >= 0) { ::close(control_sock_); control_sock_ = -1; }
  }

  if (video_thread_.joinable()) video_thread_.join();

  {
    std::lock_guard<std::mutex> lk(lifecycle_mtx_);
    cleanupResources();
  }
  stopping_ = false;

  emit disconnected();
  fprintf(stderr, "[scrcpy] stopped\n");
}

// 释放 socket / server 进程 / FFmpeg 资源 (幂等)
// 调用方需持有 lifecycle_mtx_ (runSession / stop 已加锁)
void ScrcpyWidget::cleanupResources() {
  if (video_sock_ >= 0) { ::shutdown(video_sock_, SHUT_RDWR); ::close(video_sock_); video_sock_ = -1; }
  {
    std::lock_guard<std::mutex> lk(control_mtx_);
    if (control_sock_ >= 0) { ::close(control_sock_); control_sock_ = -1; }
  }
  killServer();
  removeForward(adb_path_);

  if (decoder_) { avcodec_free_context((AVCodecContext **)&decoder_); decoder_ = nullptr; }
  if (avframe_) { av_frame_free((AVFrame **)&avframe_); avframe_ = nullptr; }
  if (avpacket_) { av_packet_free((AVPacket **)&avpacket_); avpacket_ = nullptr; }
  if (rgb_buf_) { free(rgb_buf_); rgb_buf_ = nullptr; rgb_buf_size_ = 0; }
}

// ===== 自动设备检测 =====
void ScrcpyWidget::setAutoConnect(bool on) {
  auto_connect_ = on;
  if (on) {
    if (!auto_timer_) {
      auto_timer_ = new QTimer(this);
      auto_timer_->setInterval(2000);  // 2s 轮询
      QObject::connect(auto_timer_, &QTimer::timeout, this, &ScrcpyWidget::onAutoTimer);
    }
    auto_timer_->start();
    // 立即检测一次
    QTimer::singleShot(0, this, [this]() { onAutoTimer(); });
  } else {
    if (auto_timer_) auto_timer_->stop();
  }
}

// 解析 "adb devices" 输出, 判断是否有处于 device 状态的设备
bool ScrcpyWidget::detectDevice() {
  std::string out;
  std::string cmd = adb_path_ + " devices";
  // quiet: 不打印轮询日志
  if (!runCmd(cmd, &out, true)) return false;
  std::istringstream iss(out);
  std::string line;
  while (std::getline(iss, line)) {
    if (line.find("List of devices") != std::string::npos) continue;
    if (line.empty()) continue;
    // 格式: <serial>\tdevice  (offline/unauthorized 不算)
    if (line.find("\tdevice") != std::string::npos) {
      return true;
    }
  }
  return false;
}

void ScrcpyWidget::onAutoTimer() {
  if (running_ || starting_ || stopping_ || detecting_) return;
  // 回收上一轮自然结束的会话线程 (此时 running_ 已为 false)
  if (video_thread_.joinable()) {
    video_thread_.join();
  }
  if (!auto_connect_ || stopping_) return;

  // 在工作线程执行 adb devices (避免每 2s 阻塞 UI 线程), 检测到设备后
  // 回到 UI 线程启动会话 (保证 video_thread_ 只在 UI 线程修改/回收)。
  detecting_ = true;
  std::thread([this]() {
    bool found = detectDevice();
    detecting_ = false;
    if (!found) return;
    QTimer::singleShot(0, this, [this]() {
      if (running_ || starting_ || stopping_ || !auto_connect_) return;
      fprintf(stderr, "[scrcpy] 检测到安卓设备, 自动启动投屏\n");
      starting_ = true;
      stopping_ = false;
      video_thread_ = std::thread(&ScrcpyWidget::runSession, this);
    });
  }).detach();
}

// ===== 视频接收/解码线程 =====
void ScrcpyWidget::videoLoop() {
  AVCodecContext *dec = (AVCodecContext *)decoder_;
  AVFrame *frm = (AVFrame *)avframe_;
  AVPacket *pkt = (AVPacket *)avpacket_;

  while (running_ && !stopping_) {
    // 读 12 字节 meta header: pts+flags(8) + size(4)
    uint8_t meta[12];
    if (!recvAll(video_sock_, meta, 12)) {
      if (!stopping_) fprintf(stderr, "[scrcpy] video socket 断开\n");
      break;
    }

    uint64_t pts_flags = readBE64(meta);
    uint32_t pkt_size = readBE32(meta + 8);

    if (pkt_size == 0 || pkt_size > 8 * 1024 * 1024) {
      fprintf(stderr, "[scrcpy] 异常包大小: %u, 跳过\n", pkt_size);
      continue;
    }

    // 读包数据
    std::vector<uint8_t> buf(pkt_size);
    if (!recvAll(video_sock_, buf.data(), pkt_size)) {
      if (!stopping_) fprintf(stderr, "[scrcpy] 读取包数据失败\n");
      break;
    }

    // 喂给解码器
    pkt->data = buf.data();
    pkt->size = (int)pkt_size;
    pkt->pts = (pts_flags & ((1ULL << 62) - 1));  // 低 62 位 = PTS
    if (pts_flags & (1ULL << 63)) {
      pkt->pts = AV_NOPTS_VALUE;  // config packet (SPS/PPS)
    }
    if (pts_flags & (1ULL << 62)) {
      pkt->flags |= AV_PKT_FLAG_KEY;
    } else {
      pkt->flags &= ~AV_PKT_FLAG_KEY;
    }

    int ret = avcodec_send_packet(dec, pkt);
    if (ret < 0 && ret != AVERROR(EAGAIN)) {
      // 解码错误, 继续下一帧
      continue;
    }

    // 接收解码后的帧
    while (true) {
      ret = avcodec_receive_frame(dec, frm);
      if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
      if (ret < 0) break;

      // 检测分辨率变化 (手机旋转)
      if (frm->width != video_w_ || frm->height != video_h_) {
        if (frm->width > 0 && frm->height > 0) {
          fprintf(stderr, "[scrcpy] resolution changed: %dx%d -> %dx%d\n",
                  (int)video_w_, (int)video_h_, frm->width, frm->height);
          video_w_ = frm->width;
          video_h_ = frm->height;
          free(rgb_buf_);
          rgb_buf_size_ = video_w_ * video_h_ * 3;
          rgb_buf_ = (uint8_t *)malloc(rgb_buf_size_);
          if (!rgb_buf_) {
            fprintf(stderr, "[scrcpy] realloc rgb_buf failed\n");
            break;
          }
          emit resolutionChanged(video_w_, video_h_);
        }
      }

      // I420 (YUV420P) → BGR24 via libyuv (I420ToRAW 输出 BGR, 匹配 QImage::Format_RGB888)
      if (frm->format == AV_PIX_FMT_YUV420P && rgb_buf_) {
        libyuv::I420ToRAW(
          frm->data[0], frm->linesize[0],
          frm->data[1], frm->linesize[1],
          frm->data[2], frm->linesize[2],
          rgb_buf_, video_w_ * 3,
          video_w_, video_h_);

        // 深拷贝到 QImage
        QImage img(rgb_buf_, video_w_, video_h_, video_w_ * 3, QImage::Format_RGB888);
        QImage copy = img.copy();
        {
          std::lock_guard<std::mutex> lk(frame_mtx_);
          frame_ = copy;
        }
        update();  // 触发 paintEvent (线程安全)
      }
    }
  }
}

// ===== 绘制 =====
void ScrcpyWidget::paintEvent(QPaintEvent *) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing, true);

  const int radius = 20;
  const QRect r = rect();

  // 创建圆角路径 (用于裁剪和边框)
  QPainterPath roundPath;
  roundPath.addRoundedRect(r, radius, radius);

  // === 1. 设置圆角裁剪, 后续所有绘制都在圆角内 ===
  p.setClipPath(roundPath);

  // 背景: 半透明深色 (非纯黑, 带一点质感) — 仅填充视频区域, 留出透明标题栏
  QRect video_area(0, TITLE_BAR_H, width(), height() - TITLE_BAR_H);
  p.fillRect(video_area, QColor(10, 11, 13, 225));

  // 视频画面
  QImage f;
  {
    std::lock_guard<std::mutex> lk(frame_mtx_);
    f = frame_;
  }

  QRect video_rect(0, TITLE_BAR_H, width(), height() - TITLE_BAR_H);

  if (!f.isNull()) {
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    QRect display_rect = videoDisplayRect();
    if (rotated_ && f.width() > 0 && f.height() > 0) {
      QTransform t;
      t.translate(display_rect.center().x(), display_rect.center().y());
      t.rotate(90.0);
      p.setTransform(t);
      QRect r2(-display_rect.height() / 2, -display_rect.width() / 2,
               display_rect.height(), display_rect.width());
      p.drawImage(r2, f);
      p.resetTransform();
    } else {
      p.drawImage(display_rect, f);
    }
    // 透明度叠加 (非纯黑, 用深色半透明模拟模糊感)
    if (opacity_percent_ < 100) {
      int alpha = (100 - opacity_percent_) * 200 / 100;
      p.fillRect(video_rect, QColor(8, 8, 10, alpha));
    }
  } else if (running_) {
    p.setPen(QColor(200, 200, 200, 180));
    p.drawText(video_rect, Qt::AlignCenter, "连接中...");
  }

  // === 3. 标题栏 (40px, 无文字, 纯深色 + subtle 渐变) ===
  {
    QRect title_rect(0, 0, width(), TITLE_BAR_H);
    QLinearGradient titleGrad(0, 0, 0, TITLE_BAR_H);
    titleGrad.setColorAt(0, Qt::transparent);
    titleGrad.setColorAt(1, Qt::transparent);
    p.fillRect(title_rect, titleGrad);

    // 标题栏底部分隔线 (极淡)
    p.setPen(QPen(QColor(80, 80, 80, 60), 1));
    p.drawLine(0, TITLE_BAR_H, width(), TITLE_BAR_H);

    // 拖动提示: 顶部中央一个 subtle 的小横条 (暗示可拖动)
    int barW = 36, barH = 3;
    int barX = (width() - barW) / 2;
    int barY = (TITLE_BAR_H - barH) / 2;
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(120, 120, 120, 100));
    p.drawRoundedRect(barX, barY, barW, barH, barH / 2, barH / 2);
  }

  // === 4. 内阴影 (Inset Shadow, 四边边缘渐变) ===
  // 顶部内阴影
  {
    QLinearGradient grad(0, 0, 0, 20);
    grad.setColorAt(0, QColor(0, 0, 0, 76));   // 约 30% 透明度
    grad.setColorAt(1, QColor(0, 0, 0, 0));
    p.fillRect(0, 0, width(), 20, grad);
  }
  // 底部内阴影
  {
    QLinearGradient grad(0, height() - 20, 0, height());
    grad.setColorAt(0, QColor(0, 0, 0, 0));
    grad.setColorAt(1, QColor(0, 0, 0, 76));
    p.fillRect(0, height() - 20, width(), 20, grad);
  }
  // 左侧内阴影
  {
    QLinearGradient grad(0, 0, 20, 0);
    grad.setColorAt(0, QColor(0, 0, 0, 76));
    grad.setColorAt(1, QColor(0, 0, 0, 0));
    p.fillRect(0, 0, 20, height(), grad);
  }
  // 右侧内阴影
  {
    QLinearGradient grad(width() - 20, 0, width(), 0);
    grad.setColorAt(0, QColor(0, 0, 0, 0));
    grad.setColorAt(1, QColor(0, 0, 0, 76));
    p.fillRect(width() - 20, 0, 20, height(), grad);
  }

  // === 5. 取消裁剪, 绘制边框 (暗金色 1px, 呼应 HUD 黄色) ===
  p.setClipping(false);
  p.setPen(QPen(QColor(255, 255, 255, 160), 1.5)); // 白色边框, 约 63% 透明度
  p.setBrush(Qt::NoBrush);
  p.drawRoundedRect(r.adjusted(0, 0, -1, -1), radius, radius);

  // === 6. 右下角缩放手柄 (半透明三角形) ===
  {
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(120, 120, 120, 140));
    int gx = width() - RESIZE_GRIP;
    int gy = height() - RESIZE_GRIP;
    QPolygon tri;
    tri << QPoint(width() - 4, gy + 4)
        << QPoint(width() - 4, height() - 4)
        << QPoint(gx + 4, height() - 4);
    p.drawPolygon(tri);
  }
}

// ===== 窗口拖动 / 缩放 辅助 =====
bool ScrcpyWidget::inTitleBar(int x, int y) const {
  return y < TITLE_BAR_H;
}

bool ScrcpyWidget::inResizeGrip(int x, int y) const {
  return x >= width() - RESIZE_GRIP && y >= height() - RESIZE_GRIP;
}

void ScrcpyWidget::updateHoverCursor(int x, int y) {
  if (inResizeGrip(x, y)) {
    setCursor(Qt::SizeFDiagCursor);
  } else if (inTitleBar(x, y)) {
    setCursor(Qt::SizeAllCursor);
  } else {
    setCursor(Qt::ArrowCursor);
  }
}

// ===== 触摸/鼠标事件 =====
// 视频在 widget 内的实际绘制矩形 (保持手机画面比例, 居中)
QRect ScrcpyWidget::videoDisplayRect() const {
  QRect video_rect(0, TITLE_BAR_H, width(), height() - TITLE_BAR_H);
  if (video_w_ <= 0 || video_h_ <= 0 || video_rect.width() <= 0 || video_rect.height() <= 0) {
    return video_rect;
  }
  int disp_w = rotated_ ? video_h_ : video_w_;
  int disp_h = rotated_ ? video_w_ : video_h_;
  double scale = std::min((double)video_rect.width() / disp_w,
                          (double)video_rect.height() / disp_h);
  int dw = (int)(disp_w * scale + 0.5);
  int dh = (int)(disp_h * scale + 0.5);
  int dx = video_rect.x() + (video_rect.width() - dw) / 2;
  int dy = video_rect.y() + (video_rect.height() - dh) / 2;
  return QRect(dx, dy, dw, dh);
}

void ScrcpyWidget::mapToScreen(int wx, int wy, int *sx, int *sy) {
  QRect dr = videoDisplayRect();
  if (dr.width() > 0 && dr.height() > 0) {
    int rx = (wx - dr.x()) * (rotated_ ? video_h_ : video_w_) / dr.width();
    int ry = (wy - dr.y()) * (rotated_ ? video_w_ : video_h_) / dr.height();
    if (rotated_) {
      *sx = video_w_ - ry;
      *sy = rx;
    } else {
      *sx = rx;
      *sy = ry;
    }
  } else {
    *sx = wx; *sy = wy;
  }
}

void ScrcpyWidget::sendTouch(int action, int x, int y) {
  TouchEventMsg msg;
  memset(&msg, 0, sizeof(msg));
  msg.type = TYPE_INJECT_TOUCH_EVENT;
  msg.action = (uint8_t)action;

  // pointer_id = 0 (虚拟手指), 大端
  uint64_t pid = 0;
  writeBE64((uint8_t *)&msg.pointer_id, pid);

  // 坐标大端
  uint32_t bx = (uint32_t)x;
  uint32_t by = (uint32_t)y;
  writeBE32((uint8_t *)&msg.x, bx);
  writeBE32((uint8_t *)&msg.y, by);
  writeBE16((uint8_t *)&msg.width, (uint16_t)video_w_);
  writeBE16((uint8_t *)&msg.height, (uint16_t)video_h_);

  // pressure: down/move = 0xFFFF (1.0), up = 0
  uint16_t pressure = (action == ACTION_UP) ? 0 : 0xFFFF;
  writeBE16((uint8_t *)&msg.pressure, pressure);

  // action_button, buttons = 0
  writeBE32((uint8_t *)&msg.action_button, 0);
  writeBE32((uint8_t *)&msg.buttons, 0);

  sendToControl(&msg, sizeof(msg));
}

void ScrcpyWidget::sendToControl(const void *data, size_t len) {
  std::lock_guard<std::mutex> lk(control_mtx_);
  if (control_sock_ < 0) return;
  ssize_t n = send(control_sock_, data, len, MSG_NOSIGNAL);
  if (n < 0) {
    fprintf(stderr, "[scrcpy] control send 失败: %s\n", strerror(errno));
  }
}

void ScrcpyWidget::mousePressEvent(QMouseEvent *e) {
  if (e->button() != Qt::LeftButton) return;
  int x = e->x(), y = e->y();

  // 1. 缩放手柄 (最大化时禁止缩放)
  if (inResizeGrip(x, y)) {
    if (maximized_) return;
    drag_mode_ = Resize;
    drag_last_pos_ = e->globalPos();
    drag_start_geom_ = geometry();
    return;
  }
  // 2. 标题栏 → 拖动移动
  if (inTitleBar(x, y)) {
    drag_mode_ = Move;
    drag_last_pos_ = e->globalPos();
    if (maximized_) {
      // 最大化状态下拖标题栏: 直接还原到 saved_geo_ (原来左下角小窗的位置和大小)
      // 然后从 saved_geo_ 开始跟随鼠标拖动
      maximized_ = false;
      setGeometry(saved_geo_);
      drag_start_geom_ = saved_geo_;
      emit maximizeToggled(false);
    } else {
      drag_start_geom_ = geometry();
    }
    return;
  }
  // 3. 视频区域 → 触摸反向控制
  if (!running_ || control_sock_ < 0) return;
  int sx, sy;
  mapToScreen(x, y, &sx, &sy);
  sendTouch(ACTION_DOWN, sx, sy);
}

void ScrcpyWidget::mouseMoveEvent(QMouseEvent *e) {
  int x = e->x(), y = e->y();

  if (drag_mode_ == Move) {
    QPoint delta = e->globalPos() - drag_last_pos_;
    QRect g = drag_start_geom_.translated(delta);
    // 限制在父窗口范围内 (保留标题栏可见)
    if (parentWidget()) {
      g.moveLeft(std::max(0, std::min(g.left(), (int)parentWidget()->width() - 40)));
      g.moveTop(std::max(0, std::min(g.top(), (int)parentWidget()->height() - TITLE_BAR_H)));
    }
    setGeometry(g);
    return;
  }
  if (drag_mode_ == Resize) {
    QPoint delta = e->globalPos() - drag_last_pos_;
    int nw = std::max(minimumWidth(), drag_start_geom_.width() + delta.x());
    int nh = std::max(minimumHeight(), drag_start_geom_.height() + delta.y());
    setGeometry(drag_start_geom_.left(), drag_start_geom_.top(), nw, nh);
    return;
  }

  // 未拖动: 更新光标 + 触摸移动
  updateHoverCursor(x, y);
  if (!running_ || control_sock_ < 0) return;
  if (!(e->buttons() & Qt::LeftButton)) return;  // 只在按下时移动
  // 拖动标题栏/手柄时已 return, 这里是视频区域
  int sx, sy;
  mapToScreen(x, y, &sx, &sy);
  sendTouch(ACTION_MOVE, sx, sy);
}

void ScrcpyWidget::mouseReleaseEvent(QMouseEvent *e) {
  if (e->button() != Qt::LeftButton) return;
  if (drag_mode_ == Move) {
    drag_mode_ = None;
    // 拖动到屏幕边缘 → 最大化显示 (仅非最大化时触发)
    if (!maximized_) {
      QWidget *p = parentWidget();
      if (p) {
        QRect g = geometry();
        bool near_edge = g.left() <= EDGE_THRESHOLD || p->width() - g.right() <= EDGE_THRESHOLD;
        if (near_edge && video_w_ > 0 && video_h_ > 0) {
          saved_geo_ = g;
          maximized_ = true;
          const int border = 5;
          int mh = p->height() - border * 2;
          int mw = mh * video_w_ / video_h_;
          mw = std::min(mw, p->width() - border * 2);
          // 始终吸附到屏幕左侧, 避免跑到画面中间
          int mx = border;
          rotated_ = false;  // 最大化后固定竖屏
          setGeometry(mx, border, mw, mh);
          raise();
          emit maximizeToggled(true);
          return;
        }
      }
    }
    return;
  }
  if (drag_mode_ == Resize) {
    drag_mode_ = None;
    return;
  }
  // 视频区域 → 触摸抬起
  if (!running_ || control_sock_ < 0) return;
  int sx, sy;
  mapToScreen(e->x(), e->y(), &sx, &sy);
  sendTouch(ACTION_UP, sx, sy);
}

void ScrcpyWidget::mouseDoubleClickEvent(QMouseEvent *e) {
  if (e->button() != Qt::LeftButton) return;
  // 不管是最大化还是小窗模式, 双击都请求隐藏;
  // 投屏保持运行, 双击主界面空白即可恢复显示
  emit toggleRequested();
}

// ===== adb 辅助 =====
bool ScrcpyWidget::runCmd(const std::string &cmd, std::string *out, bool quiet) {
  if (!quiet) fprintf(stderr, "[scrcpy] cmd: %s\n", cmd.c_str());
  FILE *fp = popen(cmd.c_str(), "r");
  if (!fp) {
    if (!quiet) fprintf(stderr, "[scrcpy] popen 失败\n");
    return false;
  }
  char buf[512];
  while (fgets(buf, sizeof(buf), fp)) {
    if (out) out->append(buf);
    if (!quiet) fprintf(stderr, "[scrcpy]   %s", buf);
  }
  int status = pclose(fp);
  bool ok = WIFEXITED(status) && WEXITSTATUS(status) == 0;
  if (!ok && !quiet) fprintf(stderr, "[scrcpy] cmd 失败, exit=%d\n", WEXITSTATUS(status));
  return ok;
}

bool ScrcpyWidget::setupForward(const std::string &adb_path) {
  // reverse tunnel: scrcpy 用 localabstract:scrcpy_XXXX 映射到本地 27183
  // scid 和 reverse 名字用同一个值
  snprintf(scid_str_, sizeof(scid_str_), "%08x", (uint32_t)time(nullptr));
  std::string cmd = adb_path + " reverse localabstract:scrcpy_" + scid_str_ +
                    " tcp:" + std::to_string(LOCAL_PORT);
  bool ok = runCmd(cmd);
  if (ok) forward_set_ = true;
  return ok;
}

void ScrcpyWidget::removeForward(const std::string &adb_path) {
  if (!forward_set_) return;
  forward_set_ = false;
  std::string cmd = adb_path + " reverse --remove localabstract:scrcpy_" + scid_str_;
  runCmd(cmd, nullptr, true);
}

bool ScrcpyWidget::pushServer(const std::string &adb_path, const std::string &server_jar) {
  // 检查文件是否存在
  if (access(server_jar.c_str(), R_OK) != 0) {
    fprintf(stderr, "[scrcpy] server jar 不存在: %s\n", server_jar.c_str());
    return false;
  }
  std::string cmd = adb_path + " push \"" + server_jar + "\" /data/local/tmp/scrcpy-server.jar";
  return runCmd(cmd);
}

bool ScrcpyWidget::startServer(const std::string &adb_path, int max_size, int bitrate) {
  // reverse tunnel 模式: scid 和 reverse localabstract 名字一致
  std::string shell_cmd =
    "CLASSPATH=/data/local/tmp/scrcpy-server.jar app_process / com.genymobile.scrcpy.Server 3.3.4"
    " scid=" + std::string(scid_str_) + " log_level=info audio=false";
  if (max_size > 0) {
    shell_cmd += " max_size=" + std::to_string(max_size);
  }
  if (bitrate > 0) {
    shell_cmd += " video_bit_rate=" + std::to_string(bitrate);
  }

  // 用 adb shell 后台启动: fork + exec
  // adb shell 命令会阻塞, 所以放到子进程
  std::string full_cmd = adb_path + " shell \"" + shell_cmd + "\"";

  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    return false;
  }
  if (pid == 0) {
    // 子进程
    setsid();
    // 重定向 stdin/out/err 到 /dev/null
    int devnull = open("/dev/null", O_RDWR);
    if (devnull >= 0) {
      dup2(devnull, 0);
      dup2(devnull, 1);
      dup2(devnull, 2);
      if (devnull > 2) ::close(devnull);
    }
    // 用 system 执行 (子进程里安全)
    execl("/bin/sh", "sh", "-c", full_cmd.c_str(), (char *)nullptr);
    _exit(127);
  }
  server_pid_ = pid;
  fprintf(stderr, "[scrcpy] server started, pid=%d\n", (int)pid);
  return true;
}

void ScrcpyWidget::killServer() {
  if (server_pid_ > 0) {
    kill(server_pid_, SIGTERM);
    int status;
    // 等待最多 2 秒
    for (int i = 0; i < 20; i++) {
      if (waitpid(server_pid_, &status, WNOHANG) != 0) break;
      usleep(100000);  // 100ms
    }
    if (waitpid(server_pid_, &status, WNOHANG) == 0) {
      kill(server_pid_, SIGKILL);
      waitpid(server_pid_, &status, 0);
    }
    server_pid_ = -1;
  }
}

// ===== socket 辅助 =====
int ScrcpyWidget::connectSocket(int port, int retries, int retry_delay_ms) {
  for (int i = 0; i < retries; i++) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { usleep(retry_delay_ms * 1000); continue; }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (::connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
      return fd;
    }
    ::close(fd);
    usleep(retry_delay_ms * 1000);
  }
  return -1;
}

bool ScrcpyWidget::recvAll(int fd, void *buf, size_t len) {
  uint8_t *p = (uint8_t *)buf;
  size_t got = 0;
  while (got < len) {
    ssize_t n = recv(fd, p + got, len - got, 0);
    if (n <= 0) return false;
    got += n;
  }
  return true;
}

uint64_t ScrcpyWidget::readBE64(const uint8_t *p) {
  uint64_t v = 0;
  for (int i = 0; i < 8; i++) { v = (v << 8) | p[i]; }
  return v;
}

uint32_t ScrcpyWidget::readBE32(const uint8_t *p) {
  return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
         ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}
