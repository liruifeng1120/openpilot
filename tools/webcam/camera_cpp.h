#ifndef CAMERA_CPP_H
#define CAMERA_CPP_H

#include <string>
#include <vector>
#include <linux/videodev2.h>

class CameraCpp {
public:
    struct CameraConfig {
        int width = 2592;
        int height = 1944;
        int fps = 20;
        std::string device = "/dev/video0";
        int buffer_count = 4;
        bool mjpeg_mode = true;
    };

    struct Frame {
        std::vector<uint8_t> data;
        uint64_t timestamp;
        int width;
        int height;
        bool valid;

        Frame() : timestamp(0), width(0), height(0), valid(false) {}
    };

private:
    struct Buffer {
        void* start;
        size_t length;
    };

    CameraConfig config_;
    int fd_;
    std::vector<Buffer> buffers_;
    bool initialized_;
    bool streaming_;

    // 私有方法
    bool open_device();
    bool init_device();
    bool init_mmap();
    bool start_capturing();
    void stop_capturing();
    bool read_frame_sync();
    void cleanup();

    // 设备能力检查
    bool check_device_caps();
    bool set_format();
    bool request_buffers();
    bool map_buffers();

    // 错误处理
    void log_error(const std::string& msg);
    std::string errno_string();

public:
    CameraCpp(const CameraConfig& config = CameraConfig{});
    ~CameraCpp();

    // 禁止拷贝
    CameraCpp(const CameraCpp&) = delete;
    CameraCpp& operator=(const CameraCpp&) = delete;

    // 初始化和控制
    bool initialize();
    bool start_streaming();
    void stop_streaming();
    bool is_streaming() const { return streaming_; }
    bool is_initialized() const { return initialized_; }

    // 同步帧读取（类似OpenCV接口）
    Frame read_frame();
    bool read(Frame& frame);

    // 配置获取
    const CameraConfig& get_config() const { return config_; }

    // 状态信息
    std::string get_device_info();
    bool test_connection();
};

#endif // CAMERA_CPP_H