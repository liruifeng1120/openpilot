#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <opencv2/opencv.hpp>
#include <fstream>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>
#include <onnxruntime_cxx_api.h>
#include <cmath>
#include <chrono>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <algorithm>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include "../third_party/json11/json11.hpp"

std::vector<std::string> devices;
std::vector<int> camera_sign;

struct FrameData {
    cv::Mat frame;
    int cam_id;
};

std::mutex frame_mutex;
std::mutex lane_mutex;
std::vector<cv::Mat> shared_images;
std::atomic<bool> running(true);

// 每个摄像头一个队列
int cam_max_num = 2;
/*
constexpr int MAX_CAM = 2;
std::queue<FrameData> frame_queues[MAX_CAM];
std::mutex queue_mutexes[MAX_CAM];
std::condition_variable queue_conds[MAX_CAM];
*/
/*
std::vector<std::queue<FrameData>> frame_queues;
std::vector<std::mutex> queue_mutexes;
std::vector<std::condition_variable> queue_conds;
*/

constexpr int MAX_CAM = 8;
std::vector<std::queue<FrameData>> frame_queues(MAX_CAM);
// mutex 和 condition_variable 用 unique_ptr 包裹
std::vector<std::unique_ptr<std::mutex>> queue_mutexes(MAX_CAM);
std::vector<std::unique_ptr<std::condition_variable>> queue_conds(MAX_CAM);

#define YOLO_PIX 416

// ---------------- ROI ----------------
struct CameraROI {
    std::vector<cv::Point> polygon;
    int selected_idx = -1; // 拖动顶点索引
};
std::vector<CameraROI> camera_rois;
//std::vector<int> camera_sign;
std::vector<int> camera_car;
std::vector<int> lane_safe(2,-1);

// ---------------- ONNX Runtime ----------------
Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "YOLO");
Ort::Session* session = nullptr;
Ort::SessionOptions session_options;

void initialize_yolo() {
    try {
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        session_options.SetIntraOpNumThreads(1);
        const char* model_path = "yolov5s.onnx";
        session = new Ort::Session(env, model_path, session_options);
        std::cout << "YOLO model loaded successfully!" << std::endl;
    } catch (const Ort::Exception& e) {
        std::cerr << "Failed to load YOLO model: " << e.what() << std::endl;
        running = false;
    }
}

class LaneDebouncerSingleDirection {
public:
    LaneDebouncerSingleDirection(int window_size = 5) : max_len(window_size), status(true) {}

    // 每帧更新
    void update(bool current_safe) {
        if(status && !current_safe) {
            // 当前是安全状态，但检测到不安全 → 立即切换
            status = false;
            queue.clear(); // 清空滑动窗口
        } else if(!status && current_safe) {
            // 当前是不安全状态，检测到安全 → 放入滑动窗口
            if(queue.size() >= max_len) queue.pop_front();
            queue.push_back(true);

            // 窗口满且全部安全 → 切换为安全
            if(queue.size() == max_len && std::all_of(queue.begin(), queue.end(),
                                                      [](bool v){ return v; })) {
                status = true;
                queue.clear();
            }
        }
        // 当前安全状态且检测安全 → 不用操作
        // 当前不安全状态且检测不安全 → 不用操作
    }

    bool get_status() const { return status; }

private:
    int max_len;
    bool status;              // 当前稳定状态
    std::deque<bool> queue;   // 仅用于不安全 → 安全的滑动窗口
};

LaneDebouncerSingleDirection left_checker(12);
LaneDebouncerSingleDirection right_checker(12);

// ---------------- YOLO 检测 ----------------
/*
std::vector<cv::Rect> detect_cars(cv::Mat& frame) {
    std::vector<cv::Rect> cars;

    int orig_w = frame.cols;
    int orig_h = frame.rows;

    // ---------------- Resize + letterbox + normalize ----------------
    float scale = std::min(float(YOLO_PIX) / orig_w, float(YOLO_PIX) / orig_h);
    int new_w = int(orig_w * scale);
    int new_h = int(orig_h * scale);
    int pad_x = (YOLO_PIX - new_w) / 2;
    int pad_y = (YOLO_PIX - new_h) / 2;

    cv::Mat resized_image;
    cv::resize(frame, resized_image, cv::Size(new_w, new_h));
    cv::Mat input_blob = cv::Mat::zeros(YOLO_PIX, YOLO_PIX, frame.type());
    resized_image.copyTo(input_blob(cv::Rect(pad_x, pad_y, new_w, new_h)));
    cv::cvtColor(input_blob, input_blob, cv::COLOR_BGR2RGB);
    input_blob.convertTo(input_blob, CV_32F, 1.0 / 255.0);

    // ---------------- ONNX Runtime ----------------
    std::vector<int64_t> input_shape = {1, 3, YOLO_PIX, YOLO_PIX}; // NCHW
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    // HWC -> CHW
    std::vector<float> blob_data(1*3*YOLO_PIX*YOLO_PIX);
    for(int c = 0; c < 3; c++)
        for(int y = 0; y < YOLO_PIX; y++)
            for(int x = 0; x < YOLO_PIX; x++)
                blob_data[c*YOLO_PIX*YOLO_PIX + y*YOLO_PIX + x] = input_blob.at<cv::Vec3f>(y, x)[c];

    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        memory_info,
        blob_data.data(),
        blob_data.size(),
        input_shape.data(),
        input_shape.size()
    );

    static std::vector<Ort::AllocatedStringPtr> input_names_ptrs;
    static std::vector<const char*> input_names;
    if(input_names.empty()) {
        size_t num_input_nodes = session->GetInputCount();
        for(size_t i=0;i<num_input_nodes;i++){
            auto name_ptr = session->GetInputNameAllocated(i, Ort::AllocatorWithDefaultOptions());
            input_names_ptrs.push_back(std::move(name_ptr));
            input_names.push_back(input_names_ptrs.back().get());
        }
    }

    static std::vector<Ort::AllocatedStringPtr> output_names_ptrs;
    static std::vector<const char*> output_names;
    if(output_names.empty()){
        size_t num_output_nodes = session->GetOutputCount();
        for(size_t i=0;i<num_output_nodes;i++){
            auto name_ptr = session->GetOutputNameAllocated(i, Ort::AllocatorWithDefaultOptions());
            output_names_ptrs.push_back(std::move(name_ptr));
            output_names.push_back(output_names_ptrs.back().get());
        }
    }

    auto output_tensors = session->Run(Ort::RunOptions{nullptr},
                                       input_names.data(), &input_tensor, 1,
                                       output_names.data(), output_names.size());

    float* output_data = output_tensors[0].GetTensorMutableData<float>();
    auto shape = output_tensors[0].GetTensorTypeAndShapeInfo().GetShape();
    int num_boxes = shape[1]; // 25200 for yolov5s 640, 10647 for yolov5s 416

    std::vector<cv::Rect> all_boxes;
    std::vector<float> scores;
    float conf_threshold = 0.1f;

    for(int i=0;i<num_boxes;i++){
        float x = output_data[i*85 + 0];
        float y = output_data[i*85 + 1];
        float w = output_data[i*85 + 2];
        float h = output_data[i*85 + 3];
        float conf = output_data[i*85 + 4];

        int best_class = -1;
        float best_prob = 0.0f;
        for(int c=0;c<80;c++){
            float prob = output_data[i*85 + 5 + c];
            if(prob > best_prob){
                best_prob = prob;
                best_class = c;
            }
        }

        float score = conf * best_prob;
        if(score > conf_threshold){ // 可调阈值
            if(best_class==0 || best_class==1 || best_class==2 || best_class==3 || best_class==5 || best_class==7){
                int left   = std::max(0, int((x - w/2 - pad_x)/scale));
                int top    = std::max(0, int((y - h/2 - pad_y)/scale));
                int right  = std::min(orig_w, int((x + w/2 - pad_x)/scale));
                int bottom = std::min(orig_h, int((y + h/2 - pad_y)/scale));

                all_boxes.push_back(cv::Rect(left, top, right-left, bottom-top));
                scores.push_back(score);
            }
        }
    }

    // ---------------- NMS ----------------
    std::vector<int> indices;
    float nms_threshold = 0.5f;
    cv::dnn::NMSBoxes(all_boxes, scores, conf_threshold, nms_threshold, indices);

    for(auto idx : indices)
        cars.push_back(all_boxes[idx]);

    return cars;
}
*/

struct DetectionResult {
    std::vector<cv::Rect> raw_boxes;   // 原始候选框（只经过 score 阈值过滤）
    std::vector<cv::Rect> final_boxes; // NMS 过滤后的框（更严格，用于判断）
};

// detect_cars 返回两个结果
DetectionResult detect_cars(cv::Mat& frame) {
    DetectionResult result;

    int orig_w = frame.cols;
    int orig_h = frame.rows;

    // ---------------- Resize + letterbox + normalize ----------------
    float scale = std::min(float(YOLO_PIX) / orig_w, float(YOLO_PIX) / orig_h);
    int new_w = int(orig_w * scale);
    int new_h = int(orig_h * scale);
    int pad_x = (YOLO_PIX - new_w) / 2;
    int pad_y = (YOLO_PIX - new_h) / 2;

    cv::Mat resized_image;
    cv::resize(frame, resized_image, cv::Size(new_w, new_h));
    cv::Mat input_blob = cv::Mat::zeros(YOLO_PIX, YOLO_PIX, frame.type());
    resized_image.copyTo(input_blob(cv::Rect(pad_x, pad_y, new_w, new_h)));
    cv::cvtColor(input_blob, input_blob, cv::COLOR_BGR2RGB);
    input_blob.convertTo(input_blob, CV_32F, 1.0 / 255.0);

    // ---------------- ONNX Runtime ----------------
    std::vector<int64_t> input_shape = {1, 3, YOLO_PIX, YOLO_PIX}; // NCHW
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    std::vector<float> blob_data(1*3*YOLO_PIX*YOLO_PIX);
    for(int c = 0; c < 3; c++)
        for(int y = 0; y < YOLO_PIX; y++)
            for(int x = 0; x < YOLO_PIX; x++)
                blob_data[c*YOLO_PIX*YOLO_PIX + y*YOLO_PIX + x] = input_blob.at<cv::Vec3f>(y, x)[c];

    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        memory_info,
        blob_data.data(),
        blob_data.size(),
        input_shape.data(),
        input_shape.size()
    );

    static std::vector<Ort::AllocatedStringPtr> input_names_ptrs;
    static std::vector<const char*> input_names;
    if(input_names.empty()) {
        size_t num_input_nodes = session->GetInputCount();
        for(size_t i=0;i<num_input_nodes;i++){
            auto name_ptr = session->GetInputNameAllocated(i, Ort::AllocatorWithDefaultOptions());
            input_names_ptrs.push_back(std::move(name_ptr));
            input_names.push_back(input_names_ptrs.back().get());
        }
    }

    static std::vector<Ort::AllocatedStringPtr> output_names_ptrs;
    static std::vector<const char*> output_names;
    if(output_names.empty()){
        size_t num_output_nodes = session->GetOutputCount();
        for(size_t i=0;i<num_output_nodes;i++){
            auto name_ptr = session->GetOutputNameAllocated(i, Ort::AllocatorWithDefaultOptions());
            output_names_ptrs.push_back(std::move(name_ptr));
            output_names.push_back(output_names_ptrs.back().get());
        }
    }

    auto output_tensors = session->Run(Ort::RunOptions{nullptr},
                                       input_names.data(), &input_tensor, 1,
                                       output_names.data(), output_names.size());

    float* output_data = output_tensors[0].GetTensorMutableData<float>();
    auto shape = output_tensors[0].GetTensorTypeAndShapeInfo().GetShape();
    int num_boxes = shape[1];

    std::vector<cv::Rect> all_boxes;
    std::vector<float> scores;

    float raw_conf_threshold = 0.1f;   // 宽松阈值 → 保证画框尽量多
    float nms_conf_threshold = 0.1f;   // 严格阈值 → 用于NMS
    float nms_threshold      = 0.5f;

    for(int i=0;i<num_boxes;i++){
        float x = output_data[i*85 + 0];
        float y = output_data[i*85 + 1];
        float w = output_data[i*85 + 2];
        float h = output_data[i*85 + 3];
        float conf = output_data[i*85 + 4];

        int best_class = -1;
        float best_prob = 0.0f;
        for(int c=0;c<80;c++){
            float prob = output_data[i*85 + 5 + c];
            if(prob > best_prob){
                best_prob = prob;
                best_class = c;
            }
        }

        float score = conf * best_prob;
        if(score > raw_conf_threshold){  // 用宽松阈值
            if(best_class==0 || best_class==1 || best_class==2 || best_class==3 || best_class==5 || best_class==7){
                int left   = std::max(0, int((x - w/2 - pad_x)/scale));
                int top    = std::max(0, int((y - h/2 - pad_y)/scale));
                int right  = std::min(orig_w, int((x + w/2 - pad_x)/scale));
                int bottom = std::min(orig_h, int((y + h/2 - pad_y)/scale));

                cv::Rect box(left, top, right-left, bottom-top);
                all_boxes.push_back(box);
                scores.push_back(score);
            }
        }
    }

    // 保存宽松的检测结果（仅用于画框）
    result.raw_boxes = all_boxes;

    // ---------------- NMS ----------------
    std::vector<int> indices;
    cv::dnn::NMSBoxes(all_boxes, scores, nms_conf_threshold, nms_threshold, indices);

    for(auto idx : indices)
        result.final_boxes.push_back(all_boxes[idx]);

    return result;
}


// ---------------- ROI 判断 ----------------
bool is_in_roi(const cv::Rect& box, const std::vector<cv::Point>& polygon) {
    if (polygon.empty()) return true;
    int cx = box.x + box.width / 2;
    int cy = box.y + box.height;
    return cv::pointPolygonTest(polygon, cv::Point(cx, cy), false) >= 0;
}

// ---------------- 摄像头捕获线程 ----------------
#if 0
void capture_from_camera(const std::string& device, int cam_id) {
    int fd = open(device.c_str(), O_RDWR);
    if (fd == -1) { std::cerr << "Failed to open " << device << std::endl; return; }

    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = 640;
    fmt.fmt.pix.height = 480;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    if (ioctl(fd, VIDIOC_S_FMT, &fmt) == -1) { std::cerr << "Failed set fmt " << device << std::endl; close(fd); return; }

    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = 4; req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1) { std::cerr << "Reqbuf failed " << device << std::endl; close(fd); return; }

    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; buf.memory = V4L2_MEMORY_MMAP; buf.index = 0;
    if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) { std::cerr << "Querybuf failed " << device << std::endl; close(fd); return; }

    void* buffer = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
    if (buffer == MAP_FAILED) { std::cerr << "mmap failed " << device << std::endl; close(fd); return; }

    if (ioctl(fd, VIDIOC_STREAMON, &buf.type) == -1) { std::cerr << "streamon failed " << device << std::endl; close(fd); return; }

    while (running) {
        if (ioctl(fd, VIDIOC_QBUF, &buf) == -1 || ioctl(fd, VIDIOC_DQBUF, &buf) == -1) continue;
        unsigned char* data = (unsigned char*)buffer;
        if (data[0] != 0xFF || data[1] != 0xD8) continue;

        std::vector<uchar> jpeg_data(data, data + buf.bytesused);
        cv::Mat img = cv::imdecode(jpeg_data, cv::IMREAD_COLOR);
        if (img.empty()) continue;
        
        // ---------------- YOLO 推理 + ROI ----------------
        //detect_objects_debug(img); //TEST
        std::vector<cv::Rect> cars = detect_cars(img);
        std::vector<cv::Rect> cars_in_roi;
#if 1
        if(cam_id < camera_rois.size()){
            for(auto& car : cars)
                if(is_in_roi(car, camera_rois[cam_id].polygon)){
                    cars_in_roi.push_back(car);
                }
                
            //判断摄像头框选范围是否有车
            if(cars_in_roi.size() > 0){
                if(camera_car[cam_id] == 0){
                    std::cout << "camera" + std::to_string(cam_id) + " lane unsafe!" << std::endl;
                }
                camera_car[cam_id] = 1;
            }
            else{
                if(camera_car[cam_id] == 1){
                    std::cout << "camera" + std::to_string(cam_id) + " lane safe!" << std::endl;
                }
                camera_car[cam_id] = 0;
            }
        } else {
            cars_in_roi = cars;
        }
#else
        cars_in_roi = cars;
#endif

        // 绘制 ROI
        if(cam_id < camera_rois.size() && !camera_rois[cam_id].polygon.empty())
            cv::polylines(img, std::vector<std::vector<cv::Point>>{camera_rois[cam_id].polygon}, true, cv::Scalar(0,255,0), 2);

        // 绘制检测框
        for(auto& car : cars_in_roi)
            cv::rectangle(img, car, cv::Scalar(0,0,255), 2);

        // 更新共享图像
        {
            std::lock_guard<std::mutex> lock(frame_mutex);
            if (cam_id >= shared_images.size()) shared_images.resize(cam_id + 1);
            shared_images[cam_id] = img.clone();
        }
    }
    ioctl(fd, VIDIOC_STREAMOFF, &buf.type);
    munmap(buffer, buf.length);
    close(fd);
}
#endif

void capture_from_camera(const std::string& device, int cam_id) {
    int fd = open(device.c_str(), O_RDWR);
    if (fd == -1) { std::cerr << "Failed to open " << device << std::endl; return; }

    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = 640;
    fmt.fmt.pix.height = 480;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    if (ioctl(fd, VIDIOC_S_FMT, &fmt) == -1) { std::cerr << "Failed set fmt " << device << std::endl; close(fd); return; }

    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = 4; req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1) { std::cerr << "Reqbuf failed " << device << std::endl; close(fd); return; }

    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; buf.memory = V4L2_MEMORY_MMAP; buf.index = 0;
    if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) { std::cerr << "Querybuf failed " << device << std::endl; close(fd); return; }

    void* buffer = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
    if (buffer == MAP_FAILED) { std::cerr << "mmap failed " << device << std::endl; close(fd); return; }

    if (ioctl(fd, VIDIOC_STREAMON, &buf.type) == -1) { std::cerr << "streamon failed " << device << std::endl; close(fd); return; }

    while (running) {
        if (ioctl(fd, VIDIOC_QBUF, &buf) == -1 || ioctl(fd, VIDIOC_DQBUF, &buf) == -1) continue;
        unsigned char* data = (unsigned char*)buffer;
        if (data[0] != 0xFF || data[1] != 0xD8) continue;

        std::vector<uchar> jpeg_data(data, data + buf.bytesused);
        cv::Mat img = cv::imdecode(jpeg_data, cv::IMREAD_COLOR);
        if (img.empty()) continue;

        {
            std::lock_guard<std::mutex> lock(*queue_mutexes[cam_id]);
            frame_queues[cam_id].push({img, cam_id});
        }
        queue_conds[cam_id]->notify_one();
    }
    
    ioctl(fd, VIDIOC_STREAMOFF, &buf.type);
    munmap(buffer, buf.length);
    close(fd);
}

// 推理线程函数
void inference_thread(int cam_id) {
    while (running) {
        FrameData data;
        {
            std::unique_lock<std::mutex> lock(*queue_mutexes[cam_id]);
            queue_conds[cam_id]->wait(lock, [&] { return !frame_queues[cam_id].empty() || !running; });
            if (!running) break;
            data = frame_queues[cam_id].back();   // 只取最新帧
            while (!frame_queues[cam_id].empty()) frame_queues[cam_id].pop();
        }

        cv::Mat img = data.frame;

        // ---------------- YOLO 推理 + ROI ----------------
        //std::vector<cv::Rect> cars = detect_cars(img);
        auto result = detect_cars(img);
        std::vector<cv::Rect> cars = result.raw_boxes;
        std::vector<cv::Rect> cars_box = result.final_boxes;
        std::vector<cv::Rect> cars_in_roi;
        std::vector<cv::Rect> cars_box_in_roi;

        if(cam_id < camera_rois.size()){
            for(auto& car : cars) {
                if(is_in_roi(car, camera_rois[cam_id].polygon)) {
                    cars_in_roi.push_back(car);
                }
            }

            if(!cars_in_roi.empty()) {
                std::lock_guard<std::mutex> lock(lane_mutex);
                if(camera_car[cam_id] == 0){
                    //std::cout << "camera" + std::to_string(cam_id) + " lane unsafe!" << std::endl;
                }
                camera_car[cam_id] = 1;
            } else {
                std::lock_guard<std::mutex> lock(lane_mutex);
                if(camera_car[cam_id] == 1){
                    //std::cout << "camera" + std::to_string(cam_id) + " lane safe!" << std::endl;
                }
                camera_car[cam_id] = 0;
            }
        } else {
            cars_in_roi = cars;
        }
        
        if(cam_id < camera_rois.size()){
            for(auto& car : cars_box) {
                if(is_in_roi(car, camera_rois[cam_id].polygon)) {
                    cars_box_in_roi.push_back(car);
                }
            }
        }

        // 绘制 ROI
        if(cam_id < camera_rois.size() && !camera_rois[cam_id].polygon.empty())
            cv::polylines(img, std::vector<std::vector<cv::Point>>{camera_rois[cam_id].polygon}, true, cv::Scalar(0,255,0), 2);

        // 绘制检测框
        for(auto& car : cars_box_in_roi)
            cv::rectangle(img, car, cv::Scalar(0,0,255), 2);

        // 更新共享图像
        {
            std::lock_guard<std::mutex> lock(frame_mutex);
            if (cam_id >= shared_images.size()) shared_images.resize(cam_id + 1);
            shared_images[cam_id] = img.clone();
        }
    }
}

// ---------------- 保存/加载 ROI ----------------
std::mutex roi_file_mutex;

void save_rois_threadsafe(const std::string& filename){
    std::lock_guard<std::mutex> lock(roi_file_mutex);
    std::ofstream ofs(filename);
    for(auto& roi: camera_rois){
        for(auto& pt: roi.polygon) ofs << pt.x << " " << pt.y << " ";
        ofs << "\n";
    }
}

void save_rois(const std::string& filename){
    std::ofstream ofs(filename);
    for(auto& roi: camera_rois){
        for(auto& pt: roi.polygon) ofs<<pt.x<<" "<<pt.y<<" ";
        ofs<<"\n";
    }
}
void load_rois(const std::string& filename){
    std::ifstream ifs(filename);
    if(!ifs.is_open()) return;
    camera_rois.clear();
    std::string line;
    while(std::getline(ifs,line)){
        CameraROI roi;
        std::istringstream iss(line);
        int x,y;
        while(iss>>x>>y) roi.polygon.push_back(cv::Point(x,y));
        camera_rois.push_back(roi);
    }
}

// ---------------- ROI 鼠标回调 ----------------
void mouse_callback(int event, int x, int y, int flags, void* userdata) {
    int cam_id = *reinterpret_cast<int*>(userdata); // 获取窗口对应的摄像头ID
    if (cam_id < 0 || cam_id >= camera_rois.size()) return;
    auto& roi = camera_rois[cam_id];

    auto distance = [](cv::Point a, cv::Point b) { return std::sqrt((a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y)); };
    const int select_radius = 10;
    
    static std::chrono::steady_clock::time_point last_save_time = std::chrono::steady_clock::now();
    bool changed = false;

    if (event == cv::EVENT_LBUTTONDOWN) {
        // 选择最近顶点拖动
        roi.selected_idx = -1;
        for (int i = 0; i < roi.polygon.size(); i++)
            if (distance(roi.polygon[i], cv::Point(x, y)) < select_radius) {
                roi.selected_idx = i;
                return;
            }
        // 没有选中顶点，新增顶点
        roi.polygon.push_back(cv::Point(x, y));
        changed = true;
    }
    else if (event == cv::EVENT_MOUSEMOVE) {
        if (roi.selected_idx != -1 && (flags & cv::EVENT_FLAG_LBUTTON)) {
            roi.polygon[roi.selected_idx] = cv::Point(x, y);
            changed = true;
        }
    }
    else if (event == cv::EVENT_LBUTTONUP) {
        roi.selected_idx = -1;
    }
    else if (event == cv::EVENT_RBUTTONDOWN) {
        // 删除最近顶点
        int idx = -1;
        float min_dist = select_radius;
        for (int i = 0; i < roi.polygon.size(); i++) {
            float d = distance(roi.polygon[i], cv::Point(x, y));
            if (d < min_dist) { min_dist = d; idx = i; }
        }
        if (idx != -1) {
            roi.polygon.erase(roi.polygon.begin() + idx);
            changed = true;
        }
    }
    
    // 如果有修改，实时保存
    //if (changed) save_rois_threadsafe("rois.txt");
    // 只每 0.5 秒保存一次
    if (changed) {
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_save_time).count() > 500) {
            save_rois_threadsafe("rois.txt");
            last_save_time = now;
        }
    }
}

// ---------------- 显示线程 ----------------
void display_loop() {
    while (running) {
        {
            std::lock_guard<std::mutex> lock(frame_mutex);
            for (int i = 0; i < shared_images.size(); ++i) {
                if (shared_images[i].empty()) continue;
                
                std::string window_name = "Camera " + std::to_string(i);
                cv::imshow(window_name, shared_images[i]);

                static std::vector<int> cam_ids;
                if(cam_ids.size() < shared_images.size()) cam_ids.resize(shared_images.size());
                cam_ids[i] = i;

                cv::setMouseCallback(window_name, mouse_callback, &cam_ids[i]);
            }
        }

        if (cv::waitKey(1) == 27) { // ESC
            std::cout << "display_loop end" << std::endl;
            running = false;
        }
    }
    cv::destroyAllWindows();
}

// 线程函数
void lane_check_thread() {
    while (running) {
        std::vector<int> lane_safe_tmp(2, 1); // 2 个元素，初始值都是 1

        int land_id = 0;

        // 上锁保护共享数据
        {
            std::lock_guard<std::mutex> lock(lane_mutex);

            for(int cam_id = 0; cam_id < camera_car.size(); cam_id++){
                if(camera_sign[cam_id] >= lane_safe_tmp.size()){
                    break;
                }
                land_id = camera_sign[cam_id];

                if(camera_car[land_id] > 0){
                    lane_safe_tmp[land_id] = 0;
                }
            }
        }

        // 更新消抖
        left_checker.update(lane_safe_tmp[0]);
        right_checker.update(lane_safe_tmp[1]);

        bool left_status = left_checker.get_status();
        bool right_status = right_checker.get_status();

        // 更新 lane_safe 并打印
        {
            std::lock_guard<std::mutex> lock(lane_mutex);

            if(lane_safe[0] != left_status){
                lane_safe[0] = left_status;
                std::cout << (lane_safe[0] ? "left lane safe!" : "left lane unsafe!") << std::endl;
            }

            if(lane_safe[1] != right_status){
                lane_safe[1] = right_status;
                std::cout << (lane_safe[1] ? "right lane safe!" : "right lane unsafe!") << std::endl;
            }
        }

        // 控制线程循环频率，例如 40ms 一次
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
    }
}

bool load_camera_config(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open config file: " << filename << std::endl;
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    
    std::string err;
    auto json = json11::Json::parse(content, err);
    if (!err.empty()) {
        std::cerr << "JSON parse error: " << err << std::endl;
        return false;
    }

    if (!json["cameras"].is_array()) {
        std::cerr << "Invalid config: 'cameras' must be an array" << std::endl;
        return false;
    }

    devices.clear();
    camera_sign.clear();

    for (const auto &cam : json["cameras"].array_items()) {
        devices.push_back(cam["device"].string_value());
        camera_sign.push_back(cam["sign"].int_value());
    }

    return true;
}

// ---------------- main ----------------
int main(){
    initialize_yolo();

    //std::vector<std::string> devices={"/dev/video5","/dev/video8"};
    //std::vector<std::string> devices={"/dev/v4l/by-path/pci-0000:00:14.0-usbv2-0:7.1:1.0-video-index0",
    //                                  "/dev/v4l/by-path/pci-0000:00:14.0-usbv2-0:7.4.1:1.0-video-index0"};
    if (!load_camera_config("camera_config.json")) {
        return -1;
    }

    std::cout << "Loaded " << devices.size() << " cameras" << std::endl;
    for (size_t i = 0; i < devices.size(); i++) {
        std::cout << "Camera " << i << ": " << devices[i]
                  << ", sign=" << camera_sign[i] << std::endl;
    }
    
    cam_max_num = devices.size();
    
    // 初始化指针
    for (size_t i = 0; i < cam_max_num; i++) {
        queue_mutexes[i] = std::make_unique<std::mutex>();
        queue_conds[i] = std::make_unique<std::condition_variable>();
    }
    
    //摄像头方向，0为左边，1为右边
    //camera_sign.resize(cam_max_num);
    //camera_sign[0] = 0;
    //camera_sign[1] = 1;
    
    //摄像头车辆状态，0无车，1有车
    camera_car.resize(cam_max_num);
    for(int i=0; i<camera_car.size();i++){
      camera_car[i] = 0;
    }
    
    //车道是否安全，0不安全，1安全
    //lane_safe.resize(2);
    lane_safe[0] = -1;
    lane_safe[1] = -1;
    
    // 初始化 ROI
    camera_rois.resize(cam_max_num);
    for(int i=0; i<cam_max_num;i++){
      camera_rois[i].polygon = {cv::Point(100,50),cv::Point(540,50),cv::Point(540,430),cv::Point(100,430)};
    }

    load_rois("rois.txt");

    std::vector<std::thread> threads;
    for (int i = 0; i < cam_max_num; i++) {
        threads.emplace_back(capture_from_camera, devices[i], i);
        threads.emplace_back(inference_thread, i);
    }

    std::thread lane_thread(lane_check_thread);
    std::thread display_thread(display_loop);

    //for (auto& t : threads) t.join();
    display_thread.join();
    
    std::cout << "display_loop exit" << std::endl;

    // 通知所有等待条件变量的推理线程
    for(int i=0; i<MAX_CAM; i++)
        queue_conds[i]->notify_all();
        
    std::cout << "queue_conds notify_all" << std::endl;

    // 等待摄像头采集 + 推理线程退出
    for(auto& t : threads)
        if(t.joinable()) t.join();
        
    std::cout << "camera thread exit" << std::endl;

    // 等待车道检查线程退出
    if(lane_thread.joinable()) lane_thread.join();
    
    std::cout << "lane_thread exit" << std::endl;

    //save_rois("rois.txt");

    return 0;
}

