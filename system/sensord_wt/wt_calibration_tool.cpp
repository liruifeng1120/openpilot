// wt_calibration_tool.cpp
#include <iostream>
#include <unistd.h>
#include <string>
#include <chrono>
#include <thread>
#include <cmath>
#include "sensors/wit_c_sdk.h"
#include "sensors/REG.h"
#include "sensors/serial.h"
#include "common/swaglog.h"
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct SensorData {
    double accel_x, accel_y, accel_z;
    double gyro_x, gyro_y, gyro_z;
    double mag_x, mag_y, mag_z;

    SensorData() : accel_x(0), accel_y(0), accel_z(0),
                   gyro_x(0), gyro_y(0), gyro_z(0),
                   mag_x(0), mag_y(0), mag_z(0) {}
};

class WTCalibrationTool {
private:
    int serial_fd;
    std::string device_path;
    int baud_rate;
    SensorData calibration_before;
    SensorData calibration_after;

    // 校准命令定义（根据WT传感器协议）
    static const uint8_t CMD_ACCEL_CALIB = 0x01;
    static const uint8_t CMD_GYRO_CALIB = 0x02;
    static const uint8_t CMD_MAG_CALIB = 0x07;
    static const uint8_t CMD_SAVE_CONFIG = 0x00;

    // 静态实例指针，用于回调函数
    static WTCalibrationTool* instance;

public:
    WTCalibrationTool(const std::string& device = "/dev/ttyUSB0", int baud = 115200)
        : device_path(device), baud_rate(baud), serial_fd(-1) {
        instance = this;
    }

    ~WTCalibrationTool() {
        if (serial_fd >= 0) {
            close(serial_fd);
        }
        instance = nullptr;
    }

    bool initialize() {
        // 打开串口
        serial_fd = serial_open(device_path.c_str(), baud_rate);
        if (serial_fd < 0) {
            std::cerr << "Failed to open serial device: " << device_path << std::endl;
            return false;
        }

        // 初始化WT SDK
        WitInit(WIT_PROTOCOL_NORMAL, 0x50);
        WitSerialWriteRegister([](uint8_t *data, uint32_t len) {
            if (instance && instance->serial_fd >= 0) {
                write(instance->serial_fd, data, len);
            }
        });

        WitRegisterCallBack([](uint32_t reg, uint32_t reg_num) {
            std::cout << "Register updated: " << reg << ", num: " << reg_num << std::endl;
        });

        WitDelayMsRegister([](uint16_t ms) {
            usleep(ms * 1000);
        });

        std::cout << "WT Calibration Tool initialized successfully" << std::endl;
        return true;
    }

    void sendCalibrationCommand(uint8_t cmd) {
        uint8_t calibCmd[] = {0xFF, 0xAA, 0x01, cmd, 0x00};
        if (serial_fd >= 0) {
            write(serial_fd, calibCmd, sizeof(calibCmd));
            std::cout << "Sent calibration command: 0x" << std::hex << (int)cmd << std::dec << std::endl;
        }
    }

    void saveConfiguration() {
        uint8_t saveCmd[] = {0xFF, 0xAA, 0x00, CMD_SAVE_CONFIG, 0x00};
        if (serial_fd >= 0) {
            write(serial_fd, saveCmd, sizeof(saveCmd));
            std::cout << "Configuration saved to device" << std::endl;
        }
    }

    SensorData readSensorDataValues() {
        SensorData data;
        unsigned char buffer[256];
        int len = serial_read_data(serial_fd, buffer, sizeof(buffer));

        if (len > 0) {
            for (int i = 0; i < len; i++) {
                WitSerialDataIn(buffer[i]);
            }

            // 计算传感器数据
            data.accel_x = (double)sReg[AX] / 32768.0 * 16.0 * 9.8;
            data.accel_y = (double)sReg[AY] / 32768.0 * 16.0 * 9.8;
            data.accel_z = (double)sReg[AZ] / 32768.0 * 16.0 * 9.8;

            data.gyro_x = (double)sReg[GX] / 32768.0 * 2000.0 * M_PI / 180.0;
            data.gyro_y = (double)sReg[GY] / 32768.0 * 2000.0 * M_PI / 180.0;
            data.gyro_z = (double)sReg[GZ] / 32768.0 * 2000.0 * M_PI / 180.0;

            data.mag_x = (double)sReg[HX] / 32768.0 * 1000.0;
            data.mag_y = (double)sReg[HY] / 32768.0 * 1000.0;
            data.mag_z = (double)sReg[HZ] / 32768.0 * 1000.0;
        }
        return data;
    }

    void displaySensorData(const SensorData& data) {
        std::cout << "Accel: [" << data.accel_x << ", " << data.accel_y << ", " << data.accel_z << "] m/s²" << std::endl;
        std::cout << "Gyro:  [" << data.gyro_x << ", " << data.gyro_y << ", " << data.gyro_z << "] rad/s" << std::endl;
        std::cout << "Mag:   [" << data.mag_x << ", " << data.mag_y << ", " << data.mag_z << "] μT" << std::endl;
    }
    void displayCalibrationComparison(const std::string& sensor_type,
                                     const SensorData& before,
                                     const SensorData& after) {
        std::cout << "\n=== " << sensor_type << " 校准前后对比 ===" << std::endl;

        if (sensor_type == "加速度计" || sensor_type == "完整") {
            std::cout << "加速度计 (m/s²):" << std::endl;
            std::cout << "  校准前: [" << before.accel_x << ", " << before.accel_y << ", " << before.accel_z << "]" << std::endl;
            std::cout << "  校准后: [" << after.accel_x << ", " << after.accel_y << ", " << after.accel_z << "]" << std::endl;

            double accel_improvement = sqrt(pow(before.accel_x - after.accel_x, 2) +
                                           pow(before.accel_y - after.accel_y, 2) +
                                           pow(before.accel_z - after.accel_z, 2));
            std::cout << "  改善程度: " << accel_improvement << " m/s²" << std::endl;
        }

        if (sensor_type == "陀螺仪" || sensor_type == "完整") {
            std::cout << "陀螺仪 (rad/s):" << std::endl;
            std::cout << "  校准前: [" << before.gyro_x << ", " << before.gyro_y << ", " << before.gyro_z << "]" << std::endl;
            std::cout << "  校准后: [" << after.gyro_x << ", " << after.gyro_y << ", " << after.gyro_z << "]" << std::endl;

            double gyro_improvement = sqrt(pow(before.gyro_x - after.gyro_x, 2) +
                                          pow(before.gyro_y - after.gyro_y, 2) +
                                          pow(before.gyro_z - after.gyro_z, 2));
            std::cout << "  改善程度: " << gyro_improvement << " rad/s" << std::endl;
        }

        if (sensor_type == "磁力计" || sensor_type == "完整") {
            std::cout << "磁力计 (μT):" << std::endl;
            std::cout << "  校准前: [" << before.mag_x << ", " << before.mag_y << ", " << before.mag_z << "]" << std::endl;
            std::cout << "  校准后: [" << after.mag_x << ", " << after.mag_y << ", " << after.mag_z << "]" << std::endl;

            double mag_improvement = sqrt(pow(before.mag_x - after.mag_x, 2) +
                                         pow(before.mag_y - after.mag_y, 2) +
                                         pow(before.mag_z - after.mag_z, 2));
            std::cout << "  改善程度: " << mag_improvement << " μT" << std::endl;
        }
    }

    void performAccelerometerCalibration() {
        std::cout << "\n=== 加速度计校准 ===" << std::endl;
        std::cout << "请将传感器水平放置，确保静止状态..." << std::endl;
        std::cout << "正在读取校准前数据..." << std::endl;

        // 读取校准前数据
        std::this_thread::sleep_for(std::chrono::seconds(1));
        calibration_before = readSensorDataValues();
        std::cout << "校准前数据:" << std::endl;
        displaySensorData(calibration_before);

        std::cout << "按回车键开始校准...";
        std::cin.get();

        sendCalibrationCommand(CMD_ACCEL_CALIB);
        std::cout << "加速度计校准中，请保持设备静止..." << std::endl;

        // 等待校准完成
        std::this_thread::sleep_for(std::chrono::seconds(5));

        // 读取校准后数据
        std::cout << "正在读取校准后数据..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        calibration_after = readSensorDataValues();

        std::cout << "加速度计校准完成！" << std::endl;
        displayCalibrationComparison("加速度计", calibration_before, calibration_after);
    }

    void performGyroscopeCalibration() {
        std::cout << "\n=== 陀螺仪校准 ===" << std::endl;
        std::cout << "请将传感器保持完全静止..." << std::endl;
        std::cout << "正在读取校准前数据..." << std::endl;

        // 读取校准前数据
        std::this_thread::sleep_for(std::chrono::seconds(1));
        calibration_before = readSensorDataValues();
        std::cout << "校准前数据:" << std::endl;
        displaySensorData(calibration_before);

        std::cout << "按回车键开始校准...";
        std::cin.get();

        sendCalibrationCommand(CMD_GYRO_CALIB);
        std::cout << "陀螺仪校准中，请保持设备完全静止..." << std::endl;

        // 等待校准完成
        std::this_thread::sleep_for(std::chrono::seconds(5));

        // 读取校准后数据
        std::cout << "正在读取校准后数据..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        calibration_after = readSensorDataValues();

        std::cout << "陀螺仪校准完成！" << std::endl;
        displayCalibrationComparison("陀螺仪", calibration_before, calibration_after);
    }

    void performMagnetometerCalibration() {
        std::cout << "\n=== 磁力计校准 ===" << std::endl;
        std::cout << "请按8字形缓慢转动传感器..." << std::endl;
        std::cout << "正在读取校准前数据..." << std::endl;

        // 读取校准前数据
        std::this_thread::sleep_for(std::chrono::seconds(1));
        calibration_before = readSensorDataValues();
        std::cout << "校准前数据:" << std::endl;
        displaySensorData(calibration_before);

        std::cout << "按回车键开始校准...";
        std::cin.get();

        sendCalibrationCommand(CMD_MAG_CALIB);
        std::cout << "磁力计校准中，请按8字形转动设备30秒..." << std::endl;

        // 等待校准完成
        std::this_thread::sleep_for(std::chrono::seconds(30));

        // 读取校准后数据
        std::cout << "正在读取校准后数据..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        calibration_after = readSensorDataValues();

        std::cout << "磁力计校准完成！" << std::endl;
        displayCalibrationComparison("磁力计", calibration_before, calibration_after);
    }

    void monitorSensorData(int duration_seconds = 10) {
        std::cout << "\n=== 传感器数据监控 ===" << std::endl;
        std::cout << "监控时间: " << duration_seconds << " 秒" << std::endl;

        auto start_time = std::chrono::steady_clock::now();
        while (true) {
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time);

            if (elapsed.count() >= duration_seconds) {
                break;
            }

            SensorData data = readSensorDataValues();
            displaySensorData(data);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
};

// 静态成员定义
WTCalibrationTool* WTCalibrationTool::instance = nullptr;

void printUsage(const char* prog_name) {
    std::cout << "用法: " << prog_name << " [选项]" << std::endl;
    std::cout << "WT传感器校准工具" << std::endl;
    std::cout << "选项:" << std::endl;
    std::cout << "  -d, --device PATH     串口设备路径 (默认: /dev/ttyUSB0)" << std::endl;
    std::cout << "  -b, --baud RATE       波特率 (默认: 115200)" << std::endl;
    std::cout << "  -h, --help            显示帮助信息" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string device = "/dev/ttyUSB0";
    int baud = 115200;

    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-d" || arg == "--device") {
            if (i + 1 < argc) {
                device = argv[++i];
            }
        } else if (arg == "-b" || arg == "--baud") {
            if (i + 1 < argc) {
                baud = std::atoi(argv[++i]);
            }
        } else if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        }
    }

    WTCalibrationTool calibTool(device, baud);

    if (!calibTool.initialize()) {
        std::cerr << "初始化失败！" << std::endl;
        return -1;
    }

    std::cout << "\n=== WT传感器校准工具 ===" << std::endl;
    std::cout << "设备: " << device << std::endl;
    std::cout << "波特率: " << baud << std::endl;

    while (true) {
        std::cout << "\n请选择操作:" << std::endl;
        std::cout << "1. 加速度计校准" << std::endl;
        std::cout << "2. 陀螺仪校准" << std::endl;
        std::cout << "3. 磁力计校准" << std::endl;
        std::cout << "4. 完整校准 (推荐)" << std::endl;
        std::cout << "5. 监控传感器数据" << std::endl;
        std::cout << "6. 保存配置" << std::endl;
        std::cout << "0. 退出" << std::endl;
        std::cout << "选择: ";

        int choice;
        std::cin >> choice;
        std::cin.ignore(); // 清除输入缓冲区

        switch (choice) {
            case 1:
                calibTool.performAccelerometerCalibration();
                break;
            case 2:
                calibTool.performGyroscopeCalibration();
                break;
            case 3:
                calibTool.performMagnetometerCalibration();
                break;
            case 4: {
                std::cout << "\n=== 完整校准流程 ===" << std::endl;

                // 记录初始状态
                SensorData initial_data = calibTool.readSensorDataValues();
                std::cout << "初始传感器状态:" << std::endl;
                calibTool.displaySensorData(initial_data);

                calibTool.performAccelerometerCalibration();
                calibTool.performGyroscopeCalibration();
                calibTool.performMagnetometerCalibration();
                calibTool.saveConfiguration();

                // 显示最终对比
                SensorData final_data = calibTool.readSensorDataValues();
                std::cout << "\n=== 完整校准前后总对比 ===" << std::endl;
                calibTool.displayCalibrationComparison("完整", initial_data, final_data);

                std::cout << "完整校准流程完成！" << std::endl;
                break;
            }
            case 5:
                calibTool.monitorSensorData(10);
                break;
            case 6:
                calibTool.saveConfiguration();
                break;
            case 0:
                std::cout << "退出校准工具" << std::endl;
                return 0;
            default:
                std::cout << "无效选择，请重试" << std::endl;
                break;
        }
    }

    return 0;
}