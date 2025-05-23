/*
 * Live 4-channel thermal feed via TCA9548A I2C multiplexer on Raspberry Pi
 * Displays 2x2 mosaic window using OpenCV
 * Resets multiplexer via GPIO pin after each channel use
 *
 * MIT License (sensor code by OMRON)
 */

#include <opencv2/opencv.hpp>
#include <wiringPi.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>

// I2C device
#define I2C_DEV "/dev/i2c-1"

// Thermal sensor (D6T) parameters
#define D6T_ADDR       0x0A
#define D6T_CMD        0x4D
#define N_ROW          32
#define N_COL          32
#define N_PIXEL        (N_ROW * N_COL)
#define N_READ         ((N_PIXEL + 1) * 2 + 1)

// TCA9548A multiplexer address
#define TCA_ADDR       0x70

// GPIO pin for TCA reset (BCM numbering)
const int RESET_PIN = 17;

uint8_t rbuf[N_READ];

void delayMs(int ms) {
    struct timespec ts = { .tv_sec = ms / 1000,
                          .tv_nsec = (ms % 1000) * 1000000 };
    nanosleep(&ts, NULL);
}

// Select TCA channel 0-7
void selectMux(int channel) {
    int fd = open(I2C_DEV, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "I2C open failed: %s\n", strerror(errno));
        return;
    }
    if (ioctl(fd, I2C_SLAVE, TCA_ADDR) < 0) {
        fprintf(stderr, "Select TCA failed: %s\n", strerror(errno));
        close(fd);
        return;
    }
    uint8_t cfg = 1 << channel;
    write(fd, &cfg, 1);
    close(fd);
}

// Reset TCA reset pin after finishing on a channel
void resetMux() {
    digitalWrite(RESET_PIN, LOW);
    delayMs(5);
    digitalWrite(RESET_PIN, HIGH);
    delayMs(5);
}

// Read one frame from current channel
int readSensorFrame(std::vector<float>& frame) {
    int fd = open(I2C_DEV, O_RDWR);
    if (fd < 0) return -1;
    if (ioctl(fd, I2C_SLAVE, D6T_ADDR) < 0) {
        close(fd);
        return -2;
    }
    uint8_t cmd = D6T_CMD;
    if (write(fd, &cmd, 1) != 1) {
        close(fd);
        return -3;
    }
    delayMs(1);
    if (read(fd, rbuf, N_READ) != N_READ) {
        close(fd);
        return -4;
    }
    close(fd);

    frame.resize(N_PIXEL);
    for (int i = 0; i < N_PIXEL; i++) {
        int16_t tmp = (int16_t)(rbuf[2+2*i] | (rbuf[3+2*i] << 8));
        frame[i] = tmp / 10.0f;
    }
    return 0;
}

int main() {
    // Init wiringPi for GPIO
    wiringPiSetupGpio();
    pinMode(RESET_PIN, OUTPUT);
    digitalWrite(RESET_PIN, HIGH);

    // OpenCV window
    cv::namedWindow("Thermal 2x2", cv::WINDOW_AUTOSIZE);
    const int dispSize = 256;
    std::vector<cv::Mat> cells(4);

    while (true) {
        for (int ch = 0; ch < 4; ch++) {
            // Select and read
            selectMux(ch);
            delayMs(350);           // sensor wake delay
            std::vector<float> rawFrame;
            if (readSensorFrame(rawFrame) == 0) {
                // Build Mat
                cv::Mat temp(N_ROW, N_COL, CV_32F, rawFrame.data());
                cv::Mat norm, colored, resized;
                cv::normalize(temp, norm, 0, 255, cv::NORM_MINMAX);
                norm.convertTo(norm, CV_8U);
                cv::applyColorMap(norm, colored, cv::COLORMAP_JET);
                cv::resize(colored, resized, cv::Size(dispSize, dispSize), 0, 0, cv::INTER_NEAREST);
                cells[ch] = resized;
            }
            // After using channel, reset the mux
            resetMux();
        }

        // Compose mosaic
        cv::Mat top, bottom, mosaic;
        cv::hconcat(cells[0], cells[1], top);
        cv::hconcat(cells[2], cells[3], bottom);
        cv::vconcat(top, bottom, mosaic);

        cv::imshow("Thermal 2x2", mosaic);
        if (cv::waitKey(1) == 27) break; // ESC
    }
    return 0;
}
