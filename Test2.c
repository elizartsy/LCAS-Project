/*
 * multi_d6t_mosaic.cpp
 * Capture live feeds from four Omron D6T-32L sensors via TCA9548A and display a 2×2 mosaic.
 * Build with:
 *   g++ multi_d6t_mosaic.cpp -o thermal_mosaic `pkg-config --cflags --libs opencv` -std=c++11
 * Requires OpenCV2 C++ API and Linux I2C headers.
 */

#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <opencv2/opencv.hpp>

// I2C device and addresses
static constexpr const char* I2C_DEV = "/dev/i2c-1";
static constexpr uint8_t MUX_ADDR = 0x70;  // TCA9548A
static constexpr uint8_t D6T_ADDR = 0x0A;  // Omron D6T-32L
static constexpr uint8_t D6T_CMD  = 0x4D;

// Sensor dimensions and buffer sizes
static constexpr int N_ROW   = 32;
static constexpr int N_COL   = 32;
static constexpr int N_PIXEL = N_ROW * N_COL;
static constexpr int N_BYTES = (N_PIXEL + 1) * 2 + 1;  // PTAT + pixels, 2 bytes each + 1 PEC

// Millisecond sleep
static void msleep(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// Select a channel on the multiplexer
bool selectChannel(int fd, int ch) {
    uint8_t cmd = static_cast<uint8_t>(1u << ch);
    if (ioctl(fd, I2C_SLAVE, MUX_ADDR) < 0) return false;
    return write(fd, &cmd, 1) == 1;
}

// Initialize the D6T sensor on currently selected channel
bool initSensor(int fd) {
    uint8_t buf[2] = {0x01, 0x04};  // SET_ADD, config (IIR=0, average=4)
    if (ioctl(fd, I2C_SLAVE, D6T_ADDR) < 0) return false;
    return write(fd, buf, 2) == 2;
}

// Read temperature data into a float vector of size N_PIXEL
bool readD6T(int fd, std::vector<float>& out) {
    std::vector<uint8_t> raw(N_BYTES);
    if (ioctl(fd, I2C_SLAVE, D6T_ADDR) < 0) return false;
    // Trigger read command
    if (write(fd, &D6T_CMD, 1) != 1) return false;
    if (read(fd, raw.data(), N_BYTES) != N_BYTES) return false;

    out.resize(N_PIXEL);
    // Unpack pixels (skip PTAT at index 0)
    for (int i = 0; i < N_PIXEL; ++i) {
        int idx = 2 + 2*i;
        int16_t val = static_cast<int16_t>(raw[idx] | (raw[idx+1] << 8));
        out[i] = static_cast<float>(val) / 10.0f;
    }
    return true;
}

int main() {
    // Open I2C device
    int fd = open(I2C_DEV, O_RDWR);
    if (fd < 0) {
        std::cerr << "Failed to open I2C device: " << strerror(errno) << std::endl;
        return 1;
    }

    // Initialize all four sensors
    for (int ch = 0; ch < 4; ++ch) {
        if (!selectChannel(fd, ch)) {
            std::cerr << "Failed to select channel " << ch << std::endl;
            return 2;
        }
        msleep(50);
        if (!initSensor(fd)) {
            std::cerr << "Failed to init sensor on channel " << ch << std::endl;
            return 3;
        }
    }
    msleep(400);

    cv::namedWindow("Thermal Mosaic", cv::WINDOW_AUTOSIZE);
    std::vector<cv::Mat> tiles(4);
    for (auto& m : tiles) {
        m = cv::Mat::zeros(256, 256, CV_8UC1);
    }

    cv::Mat mosaic(512, 512, CV_8UC1);
    std::vector<float> pixelBuf;

    while (true) {
        for (int ch = 0; ch < 4; ++ch) {
            selectChannel(fd, ch);
            msleep(20);
            if (!readD6T(fd, pixelBuf)) {
                std::cerr << "Read error on channel " << ch << std::endl;
                continue;
            }
            // Normalize 20–40 °C to 0–255
            cv::Mat raw(N_ROW, N_COL, CV_8UC1);
            for (int i = 0; i < N_PIXEL; ++i) {
                float v = (pixelBuf[i] - 20.0f) * (255.0f / 20.0f);
                raw.data[i] = static_cast<uint8_t>(std::clamp(v, 0.0f, 255.0f));
            }
            // Resize and colorize
            cv::resize(raw, tiles[ch], tiles[ch].size(), 0, 0, cv::INTER_NEAREST);
            cv::applyColorMap(tiles[ch], tiles[ch], cv::COLORMAP_JET);
            cv::cvtColor(tiles[ch], tiles[ch], cv::COLOR_BGR2GRAY); // single channel for mosaic
        }

        // Build mosaic
        tiles[0].copyTo(mosaic(cv::Rect(0,   0, 256, 256)));
        tiles[1].copyTo(mosaic(cv::Rect(256, 0, 256, 256)));
        tiles[2].copyTo(mosaic(cv::Rect(0, 256, 256, 256)));
        tiles[3].copyTo(mosaic(cv::Rect(256,256,256,256)));

        cv::imshow("Thermal Mosaic", mosaic);
        if (cv::waitKey(1) == 27) break;
    }

    close(fd);
    return 0;
}

