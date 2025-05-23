/*
 * Live 4-channel thermal feed via TCA9548A I2C multiplexer on Raspberry Pi
 * Displays 2x2 mosaic window using OpenCV
 * Resets multiplexer via GPIO sysfs after each channel use
 * Initializes each D6T sensor before reading
 *
 * MIT License (sensor code by OMRON)
 */

#include <opencv2/opencv.hpp>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <vector>

// I2C device
#define I2C_DEV "/dev/i2c-1"

// Thermal sensor (D6T) parameters
#define D6T_ADDR       0x0A
#define D6T_CMD        0x4D
#define D6T_SET_ADD    0x01
#define D6T_IIR        0x00    // infinite impulse response filter setting
#define D6T_AVERAGE    0x04    // averaging setting
#define N_ROW          32
#define N_COL          32
#define N_PIXEL        (N_ROW * N_COL)
#define N_READ         ((N_PIXEL + 1) * 2 + 1)

// TCA9548A multiplexer address
#define TCA_ADDR       0x70

// GPIO sysfs for TCA reset (BCM 17)
#define RESET_GPIO     17
#define SYSFS_GPIO_DIR "/sys/class/gpio"

uint8_t rbuf[N_READ];

void delayMs(int ms) {
    struct timespec ts = { .tv_sec = ms / 1000,
                          .tv_nsec = (ms % 1000) * 1000000 };
    nanosleep(&ts, NULL);
}

// Export and configure GPIO pin for output
int gpioExport(int gpio) {
    char buf[64];
    int len, fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
    if (fd < 0) return -1;
    len = snprintf(buf, sizeof(buf), "%d", gpio);
    write(fd, buf, len);
    close(fd);
    return 0;
}

int gpioUnexport(int gpio) {
    char buf[64];
    int len, fd = open(SYSFS_GPIO_DIR "/unexport", O_WRONLY);
    if (fd < 0) return -1;
    len = snprintf(buf, sizeof(buf), "%d", gpio);
    write(fd, buf, len);
    close(fd);
    return 0;
}

int gpioSetDirection(int gpio, const char *dir) {
    char path[80];
    int fd;
    snprintf(path, sizeof(path), SYSFS_GPIO_DIR "/gpio%d/direction", gpio);
    fd = open(path, O_WRONLY);
    if (fd < 0) return -1;
    write(fd, dir, strlen(dir));
    close(fd);
    return 0;
}

int gpioWriteValue(int gpio, int value) {
    char path[80];
    int fd;
    snprintf(path, sizeof(path), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
    fd = open(path, O_WRONLY);
    if (fd < 0) return -1;
    write(fd, value ? "1" : "0", 1);
    close(fd);
    return 0;
}

// Initialize GPIO reset pin
void initResetPin() {
    gpioExport(RESET_GPIO);
    delayMs(100);
    gpioSetDirection(RESET_GPIO, "out");
    gpioWriteValue(RESET_GPIO, 1);
}

// Reset TCA by toggling GPIO
void resetMux() {
    gpioWriteValue(RESET_GPIO, 0);
    delayMs(5);
    gpioWriteValue(RESET_GPIO, 1);
    delayMs(5);
}

// Select TCA channel
void selectMux(int channel) {
    int fd = open(I2C_DEV, O_RDWR);
    if (fd < 0) return;
    ioctl(fd, I2C_SLAVE, TCA_ADDR);
    uint8_t cfg = 1 << channel;
    write(fd, &cfg, 1);
    close(fd);
    delayMs(1);
}

// Initialize a single D6T sensor on current mux channel
void initSensor() {
    int fd = open(I2C_DEV, O_RDWR);
    if (fd < 0) return;
    ioctl(fd, I2C_SLAVE, D6T_ADDR);
    uint8_t dat[] = { D6T_SET_ADD, (uint8_t)(((D6T_IIR << 4) & 0xF0) | (D6T_AVERAGE & 0x0F)) };
    write(fd, dat, sizeof(dat));
    close(fd);
    delayMs(10);
}

// Read one thermal frame
int readSensorFrame(std::vector<float>& frame) {
    int fd = open(I2C_DEV, O_RDWR);
    if (fd < 0) return -1;
    ioctl(fd, I2C_SLAVE, D6T_ADDR);
    uint8_t cmd = D6T_CMD;
    if (write(fd, &cmd, 1) != 1) { close(fd); return -2; }
    delayMs(1);
    if (read(fd, rbuf, N_READ) != N_READ) { close(fd); return -3; }
    close(fd);
    frame.resize(N_PIXEL);
    for (int i = 0; i < N_PIXEL; i++) {
        int16_t tmp = (int16_t)(rbuf[2+2*i] | (rbuf[3+2*i] << 8));
        frame[i] = tmp / 10.0f;
    }
    return 0;
}

int main() {
    // Setup reset GPIO
    initResetPin();

    cv::namedWindow("Thermal 2x2", cv::WINDOW_AUTOSIZE);
    const int dispSize = 256;
    std::vector<cv::Mat> cells(4);

    while (true) {
        for (int ch = 0; ch < 4; ch++) {
            selectMux(ch);
            initSensor();                    // initialize sensor on selected channel
            delayMs(350);

            std::vector<float> rawFrame;
            if (readSensorFrame(rawFrame) == 0) {
                cv::Mat temp(N_ROW, N_COL, CV_32F, rawFrame.data());
                cv::Mat norm, colored, resized;
                cv::normalize(temp, norm, 0, 255, cv::NORM_MINMAX);
                norm.convertTo(norm, CV_8U);
                cv::applyColorMap(norm, colored, cv::COLORMAP_JET);
                cv::resize(colored, resized, cv::Size(dispSize, dispSize), 0, 0, cv::INTER_NEAREST);
                cells[ch] = resized;
            }
            resetMux();
        }

        cv::Mat top, bottom, mosaic;
        cv::hconcat(cells[0], cells[1], top);
        cv::hconcat(cells[2], cells[3], bottom);
        cv::vconcat(top, bottom, mosaic);

        cv::imshow("Thermal 2x2", mosaic);
        if (cv::waitKey(1) == 27) break;
    }

    // Cleanup
    gpioUnexport(RESET_GPIO);
    return 0;
}
/*
QStandardPaths: XDG_RUNTIME_DIR not set, defaulting to '/tmp/runtime-root'
terminate called after throwing an instance of 'cv::Exception'
  what():  OpenCV(4.6.0) ./modules/highgui/src/window.cpp:967: error: (-215:Assertion failed) size.width>0 && size.height>0 in function 'imshow'

Aborted
*/
