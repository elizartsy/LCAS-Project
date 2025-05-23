/**
 * Live 4-channel thermal feed via TCA9548A multiplexer
 * 2x2 mosaic display with error handling and initialization
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
#include <stdlib.h>

// I2C and sensor constants
#define I2C_DEV        "/dev/i2c-1"
#define TCA_ADDR       0x70
#define D6T_ADDR       0x0A
#define D6T_CMD        0x4D
#define D6T_SET_ADD    0x01
#define D6T_IIR        0x00
#define D6T_AVERAGE    0x04
#define N_ROW          32
#define N_COL          32
#define N_PIXEL        (N_ROW * N_COL)
#define N_READ         ((N_PIXEL + 1) * 2 + 1)

// GPIO reset pin via sysfs
#define RESET_GPIO     17
#define SYSFS_GPIO_DIR "/sys/class/gpio"

uint8_t rbuf[N_READ];

static void delayMs(int ms) {
    struct timespec ts = { .tv_sec = ms/1000, .tv_nsec = (ms%1000)*1000000 };
    nanosleep(&ts, NULL);
}

// Sysfs GPIO helpers
static int gpioWrite(const char *path, const char *value) {
    int fd = open(path, O_WRONLY);
    if (fd < 0) return -1;
    write(fd, value, strlen(value));
    close(fd);
    return 0;
}

static void initResetPin() {
    gpioWrite(SYSFS_GPIO_DIR "/export", "17");
    delayMs(100);
    gpioWrite(SYSFS_GPIO_DIR "/gpio17/direction", "out");
    gpioWrite(SYSFS_GPIO_DIR "/gpio17/value", "1");
}

static void resetMux() {
    gpioWrite(SYSFS_GPIO_DIR "/gpio17/value", "0");
    delayMs(5);
    gpioWrite(SYSFS_GPIO_DIR "/gpio17/value", "1");
    delayMs(5);
}

// Select TCA channel
static void selectMux(int ch) {
    int fd = open(I2C_DEV, O_RDWR);
    if (fd<0) return;
    ioctl(fd, I2C_SLAVE, TCA_ADDR);
    uint8_t cfg = 1<<ch;
    write(fd, &cfg, 1);
    close(fd);
    delayMs(1);
}

// Initialize D6T sensor on active channel
static void initSensor() {
    int fd = open(I2C_DEV, O_RDWR);
    if (fd<0) return;
    ioctl(fd, I2C_SLAVE, D6T_ADDR);
    uint8_t cfg[2] = { D6T_SET_ADD, uint8_t(((D6T_IIR<<4)&0xF0)| (D6T_AVERAGE&0x0F)) };
    write(fd, cfg, 2);
    close(fd);
    delayMs(10);
}

// Read frame; returns true on success
static bool readFrame(std::vector<float>& frame) {
    int fd = open(I2C_DEV, O_RDWR);
    if (fd<0) return false;
    ioctl(fd, I2C_SLAVE, D6T_ADDR);
    uint8_t cmd = D6T_CMD;
    if (write(fd, &cmd, 1)!=1) { close(fd); return false; }
    delayMs(1);
    if (read(fd, rbuf, N_READ)!=N_READ) { close(fd); return false; }
    close(fd);
    frame.resize(N_PIXEL);
    for (int i=0;i<N_PIXEL;i++){
        int16_t val = (int16_t)(rbuf[2+2*i] | (rbuf[3+2*i]<<8));
        frame[i] = val/10.0f;
    }
    return true;
}

int main(){
    // Ensure OpenCV can create windows when running as root
    setenv("XDG_RUNTIME_DIR","/tmp",1);

    // Setup
    initResetPin();
    cv::namedWindow("Thermal 2x2", cv::WINDOW_AUTOSIZE);
    const int dispSize=256;
    std::vector<cv::Mat> cells(4, cv::Mat::zeros(dispSize, dispSize, CV_8UC3));

    // Main loop
    while(true){
        for(int ch=0; ch<4; ch++){
            selectMux(ch);
            initSensor();
            delayMs(350);
            std::vector<float> raw;
            if(readFrame(raw)){
                cv::Mat temp(N_ROW, N_COL, CV_32F, raw.data());
                cv::Mat norm, cmapped, resized;
                cv::normalize(temp, norm, 0, 255, cv::NORM_MINMAX);
                norm.convertTo(norm, CV_8U);
                cv::applyColorMap(norm, cmapped, cv::COLORMAP_JET);
                cv::resize(cmapped, resized, cv::Size(dispSize, dispSize), 0,0,cv::INTER_NEAREST);
                cells[ch]=resized;
            }
            resetMux();
        }
        // Build mosaic and display
        cv::Mat top, bottom, mosaic;
        cv::hconcat(cells[0], cells[1], top);
        cv::hconcat(cells[2], cells[3], bottom);
        cv::vconcat(top, bottom, mosaic);

        if(mosaic.cols>0 && mosaic.rows>0)
            cv::imshow("Thermal 2x2", mosaic);
        if(cv::waitKey(1)==27) break;
    }
    // Cleanup
    gpioWrite(SYSFS_GPIO_DIR "/unexport", "17");
    return 0;
}
