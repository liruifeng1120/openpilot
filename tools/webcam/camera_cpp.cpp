#include "camera_cpp.h"
#include <iostream>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <fcntl.h>
#include <chrono>

CameraCpp::CameraCpp(const CameraConfig& config)
    : config_(config), fd_(-1), initialized_(false), streaming_(false) {
}

CameraCpp::~CameraCpp() {
    stop_streaming();
    cleanup();
}

bool CameraCpp::initialize() {
    if (initialized_) {
        return true;
    }

    std::cout << "初始化C++摄像头驱动..." << std::endl;
    std::cout << "设备: " << config_.device << std::endl;
    std::cout << "分辨率: " << config_.width << "x" << config_.height << std::endl;
    std::cout << "帧率: " << config_.fps << " fps" << std::endl;

    if (!open_device()) {
        log_error("无法打开摄像头设备");
        return false;
    }

    if (!check_device_caps()) {
        log_error("设备不支持所需功能");
        cleanup();
        return false;
    }

    if (!init_device()) {
        log_error("设备初始化失败");
        cleanup();
        return false;
    }

    if (!init_mmap()) {
        log_error("内存映射初始化失败");
        cleanup();
        return false;
    }

    initialized_ = true;
    std::cout << "C++摄像头驱动初始化成功" << std::endl;
    return true;
}

bool CameraCpp::start_streaming() {
    if (!initialized_) {
        log_error("摄像头未初始化");
        return false;
    }

    if (streaming_) {
        return true;
    }

    if (!start_capturing()) {
        log_error("启动采集失败");
        return false;
    }

    streaming_ = true;
    std::cout << "开始摄像头流模式" << std::endl;
    return true;
}

void CameraCpp::stop_streaming() {
    if (!streaming_) {
        return;
    }

    stop_capturing();
    streaming_ = false;
    std::cout << "停止摄像头流模式" << std::endl;
}

// 同步读取帧（类似OpenCV接口）
CameraCpp::Frame CameraCpp::read_frame() {
    Frame frame;
    read(frame);
    return frame;
}

bool CameraCpp::read(Frame& frame) {
    if (!streaming_) {
        frame.valid = false;
        return false;
    }

    // 等待帧可用
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd_, &fds);

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    int r = select(fd_ + 1, &fds, NULL, NULL, &timeout);

    if (r == -1) {
        if (errno != EINTR) {
            log_error("select失败: " + errno_string());
        }
        frame.valid = false;
        return false;
    }

    if (r == 0) {
        // 超时
        frame.valid = false;
        return false;
    }

    if (!FD_ISSET(fd_, &fds)) {
        frame.valid = false;
        return false;
    }

    // 读取帧
    struct v4l2_buffer buf = {};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd_, VIDIOC_DQBUF, &buf) == -1) {
        if (errno == EAGAIN) {
            frame.valid = false;
            return false;
        }
        log_error("出队缓冲区失败: " + errno_string());
        frame.valid = false;
        return false;
    }

    if (buf.index >= buffers_.size()) {
        log_error("无效的缓冲区索引");
        frame.valid = false;
        return false;
    }

    // 复制帧数据
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>
        (std::chrono::steady_clock::now().time_since_epoch()).count();

    frame.data.assign(
        static_cast<uint8_t*>(buffers_[buf.index].start),
        static_cast<uint8_t*>(buffers_[buf.index].start) + buf.bytesused
    );
    frame.timestamp = timestamp;
    frame.width = config_.width;
    frame.height = config_.height;
    frame.valid = true;

    // 重新入队缓冲区
    if (ioctl(fd_, VIDIOC_QBUF, &buf) == -1) {
        log_error("重新入队缓冲区失败: " + errno_string());
        return false;
    }

    return true;
}

bool CameraCpp::open_device() {
    fd_ = open(config_.device.c_str(), O_RDWR | O_NONBLOCK);

    if (fd_ == -1) {
        log_error("无法打开设备 " + config_.device + ": " + errno_string());
        return false;
    }

    return true;
}

bool CameraCpp::check_device_caps() {
    struct v4l2_capability cap;

    if (ioctl(fd_, VIDIOC_QUERYCAP, &cap) == -1) {
        log_error("查询设备能力失败: " + errno_string());
        return false;
    }

    std::cout << "设备信息:" << std::endl;
    std::cout << "  驱动: " << cap.driver << std::endl;
    std::cout << "  设备: " << cap.card << std::endl;
    std::cout << "  总线: " << cap.bus_info << std::endl;

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        log_error("设备不支持视频采集");
        return false;
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        log_error("设备不支持流模式");
        return false;
    }

    return true;
}

bool CameraCpp::set_format() {
    struct v4l2_format fmt = {};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = config_.width;
    fmt.fmt.pix.height = config_.height;

    if (config_.mjpeg_mode) {
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
        std::cout << "使用MJPEG格式" << std::endl;
    } else {
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
        std::cout << "使用YUYV格式" << std::endl;
    }

    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if (ioctl(fd_, VIDIOC_S_FMT, &fmt) == -1) {
        log_error("设置格式失败: " + errno_string());
        return false;
    }

    // 验证设置的格式
    if (fmt.fmt.pix.width != static_cast<__u32>(config_.width) ||
        fmt.fmt.pix.height != static_cast<__u32>(config_.height)) {
        std::cout << "警告: 请求的分辨率 " << config_.width << "x" << config_.height
                  << " 被调整为 " << fmt.fmt.pix.width << "x" << fmt.fmt.pix.height << std::endl;
        config_.width = fmt.fmt.pix.width;
        config_.height = fmt.fmt.pix.height;
    }

    return true;
}

bool CameraCpp::init_device() {
    if (!set_format()) {
        return false;
    }

    // 设置帧率
    struct v4l2_streamparm parm = {};
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    parm.parm.capture.timeperframe.numerator = 1;
    parm.parm.capture.timeperframe.denominator = config_.fps;

    if (ioctl(fd_, VIDIOC_S_PARM, &parm) == -1) {
        std::cout << "警告: 设置帧率失败，使用默认帧率: " << errno_string() << std::endl;
    } else {
        // 验证帧率设置
        int actual_fps = parm.parm.capture.timeperframe.denominator /
                        parm.parm.capture.timeperframe.numerator;
        if (actual_fps != config_.fps) {
            std::cout << "警告: 请求的帧率 " << config_.fps
                      << " fps被调整为 " << actual_fps << " fps" << std::endl;
        }
    }

    return true;
}

bool CameraCpp::request_buffers() {
    struct v4l2_requestbuffers req = {};
    req.count = config_.buffer_count;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd_, VIDIOC_REQBUFS, &req) == -1) {
        log_error("请求缓冲区失败: " + errno_string());
        return false;
    }

    if (req.count < 2) {
        log_error("缓冲区数量不足");
        return false;
    }

    std::cout << "分配了 " << req.count << " 个缓冲区" << std::endl;
    return true;
}

bool CameraCpp::map_buffers() {
    buffers_.resize(config_.buffer_count);

    for (int i = 0; i < config_.buffer_count; ++i) {
        struct v4l2_buffer buf = {};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (ioctl(fd_, VIDIOC_QUERYBUF, &buf) == -1) {
            log_error("查询缓冲区失败: " + errno_string());
            return false;
        }

        buffers_[i].length = buf.length;
        buffers_[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE,
                                MAP_SHARED, fd_, buf.m.offset);

        if (buffers_[i].start == MAP_FAILED) {
            log_error("内存映射失败: " + errno_string());
            return false;
        }
    }

    return true;
}

bool CameraCpp::init_mmap() {
    if (!request_buffers()) {
        return false;
    }

    if (!map_buffers()) {
        return false;
    }

    return true;
}

bool CameraCpp::start_capturing() {
    // 将所有缓冲区加入队列
    for (size_t i = 0; i < buffers_.size(); ++i) {
        struct v4l2_buffer buf = {};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (ioctl(fd_, VIDIOC_QBUF, &buf) == -1) {
            log_error("缓冲区入队失败: " + errno_string());
            return false;
        }
    }

    // 开始流
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd_, VIDIOC_STREAMON, &type) == -1) {
        log_error("开始流失败: " + errno_string());
        return false;
    }

    return true;
}

void CameraCpp::stop_capturing() {
    if (fd_ == -1) {
        return;
    }

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd_, VIDIOC_STREAMOFF, &type) == -1) {
        log_error("停止流失败: " + errno_string());
    }
}

void CameraCpp::cleanup() {
    if (fd_ != -1) {
        // 取消映射缓冲区
        for (auto& buffer : buffers_) {
            if (buffer.start != MAP_FAILED) {
                munmap(buffer.start, buffer.length);
            }
        }
        buffers_.clear();

        close(fd_);
        fd_ = -1;
    }

    initialized_ = false;
}

std::string CameraCpp::get_device_info() {
    if (fd_ == -1) {
        return "设备未打开";
    }

    struct v4l2_capability cap;
    if (ioctl(fd_, VIDIOC_QUERYCAP, &cap) == -1) {
        return "无法获取设备信息";
    }

    return std::string("驱动: ") + reinterpret_cast<char*>(cap.driver) +
           ", 设备: " + reinterpret_cast<char*>(cap.card) +
           ", 总线: " + reinterpret_cast<char*>(cap.bus_info);
}

bool CameraCpp::test_connection() {
    if (!open_device()) {
        return false;
    }

    bool result = check_device_caps();

    if (fd_ != -1) {
        close(fd_);
        fd_ = -1;
    }

    return result;
}

void CameraCpp::log_error(const std::string& msg) {
    std::cerr << "[CameraCpp错误] " << msg << std::endl;
}

std::string CameraCpp::errno_string() {
    return std::string(strerror(errno)) + " (errno: " + std::to_string(errno) + ")";
}