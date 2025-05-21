// main.cpp: Live 4-channel Omron D6T-324 thermal video viewer over TCA9548A on Raspberry Pi 5

#include <iostream>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <opencv2/opencv.hpp>

// I2C addresses
constexpr int I2C_BUS = 1;            // /dev/i2c-1
constexpr int TCA_ADDR = 0x70;
constexpr int D6T_ADDR = 0x0A;        // Omron D6T default

// D6T-324: 32x32 pixels -> 1024 readings; 2 bytes each + 2-byte status
constexpr int D6T_PX = 32 * 32;
constexpr int D6T_BYTES = 2 + D6T_PX * 2;

// Select channel on TCA9548A
bool selectChannel(int fd, int channel) {
    if (channel < 0 || channel > 7) return false;
    uint8_t cfg = 1 << channel;
    if (ioctl(fd, I2C_SLAVE, TCA_ADDR) < 0) return false;
    if (write(fd, &cfg, 1) != 1) return false;
    return true;
}

// Read raw D6T data into buffer (big-endian signed 16-bit)
bool readD6T(int fd, std::vector<int16_t>& buf) {
    if (ioctl(fd, I2C_SLAVE, D6T_ADDR) < 0) return false;
    std::vector<uint8_t> raw(D6T_BYTES);
    if (read(fd, raw.data(), D6T_BYTES) != D6T_BYTES) return false;
    buf.resize(D6T_PX);
    // Skip first two status bytes, then each pixel is signed 16-bit BE
    for (int i = 0; i < D6T_PX; ++i) {
        int idx = 2 + i * 2;
        int16_t val = (raw[idx] << 8) | raw[idx + 1];
        buf[i] = val;
    }
    return true;
}

int main() {
    // Open I2C bus
    int fd = open("/dev/i2c-1", O_RDWR);
    if (fd < 0) {
        std::cerr << "Failed to open I2C bus" << std::endl;
        return 1;
    }

    // Prepare OpenCV windows
    cv::namedWindow("Thermal Mosaic", cv::WINDOW_AUTOSIZE);

    std::vector<std::vector<int16_t>> frames(4);
    while (true) {
        // For each sensor channel
        std::vector<cv::Mat> panels;
        for (int ch = 0; ch < 4; ++ch) {
            if (!selectChannel(fd, ch)) {
                std::cerr << "Failed to select channel " << ch << std::endl;
                continue;
            }
            if (!readD6T(fd, frames[ch])) {
                std::cerr << "Read error on sensor " << ch << std::endl;
                continue;
            }
            // Convert to float matrix
            cv::Mat raw(D6T_PX, 1, CV_16S, frames[ch].data());
            cv::Mat tmp;
            raw.convertTo(tmp, CV_32F, 0.1f); // 0.1°C per LSB
            cv::Mat img = tmp.reshape(1, 32);
            // Normalize and apply colormap
            cv::Mat norm;
            cv::normalize(img, norm, 0.0, 255.0, cv::NORM_MINMAX);
            norm.convertTo(norm, CV_8U);
            cv::Mat color;
            cv::applyColorMap(norm, color, cv::COLORMAP_JET);
            // Resize for better visibility
            cv::resize(color, color, cv::Size(), 8, 8, cv::INTER_NEAREST);
            panels.push_back(color);
        }
        // Stitch 2x2 mosaic
        int h = panels[0].rows, w = panels[0].cols;
        cv::Mat top, bot, mosaic;
        cv::hconcat(panels[0], panels[1], top);
        cv::hconcat(panels[2], panels[3], bot);
        cv::vconcat(top, bot, mosaic);

        cv::imshow("Thermal Mosaic", mosaic);
        if (cv::waitKey(1) == 27) break;  // Exit on ESC
    }

    close(fd);
    return 0;
}

// CMakeLists.txt:
// ----------------
// cmake_minimum_required(VERSION 3.10)
// project(thermal_vibe_code)
// find_package(OpenCV REQUIRED)
// add_executable(main main.cpp)
// target_include_directories(main PRIVATE /usr/include)
// target_link_libraries(main PRIVATE ${OpenCV_LIBS})
// set_target_properties(main PROPERTIES CXX_STANDARD 11)
