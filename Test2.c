/*
 * MIT License
 * Adapted to drive 4 OMRON D6T-32L sensors via TCA9548A on Raspberry Pi
 * Displays a live 32x32 heatmap from each sensor using OpenCV
 */

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cerrno>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <opencv2/opencv.hpp>

// I2C addresses
#define TCA_ADDR      0x70  // TCA9548A multiplexer
#define D6T_ADDR      0x0A  // D6T-32L sensor

#define I2C_DEV       "/dev/i2c-1"
#define N_ROW         32
#define N_PIXEL       (N_ROW * N_ROW)
#define N_READ        ((N_PIXEL + 1) * 2 + 1)

// D6T commands and settings
#define D6T_CMD       0x4D
#define D6T_SET_ADD   0x01
#define D6T_IIR       0x00
#define D6T_AVERAGE   0x04

uint8_t rbuf[N_READ];
double pix_data[N_PIXEL];

// Multiplexer: select channel 0-7
bool tca_select(int channel) {
    int fd = open(I2C_DEV, O_RDWR);
    if (fd < 0) return false;
    if (ioctl(fd, I2C_SLAVE, TCA_ADDR) < 0) { close(fd); return false; }
    uint8_t cmd = 1 << channel;
    if (write(fd, &cmd, 1) != 1) { close(fd); return false; }
    close(fd);
    return true;
}

uint8_t calc_crc(uint8_t data) {
    for (int i = 0; i < 8; ++i) {
        uint8_t temp = data;
        data <<= 1;
        if (temp & 0x80) data ^= 0x07;
    }
    return data;
}

bool check_pec(uint8_t buf[], int n) {
    uint8_t crc = calc_crc((D6T_ADDR << 1) | 1);
    for (int i = 0; i < n; ++i) crc = calc_crc(buf[i] ^ crc);
    return (crc == buf[n]);
}

int16_t conv_s16_le(uint8_t* buf, int idx) {
    uint16_t lo = buf[idx];
    uint16_t hi = buf[idx+1] << 8;
    return static_cast<int16_t>(lo | hi);
}

bool read_d6t() {
    int fd = open(I2C_DEV, O_RDWR);
    if (fd < 0) return false;
    // set slave address
    if (ioctl(fd, I2C_SLAVE, D6T_ADDR) < 0) { close(fd); return false; }
    uint8_t cmd = D6T_CMD;
    if (write(fd, &cmd, 1) != 1) { close(fd); return false; }
    // read block
    if (read(fd, rbuf, N_READ) != N_READ) { close(fd); return false; }
    close(fd);
    if (!check_pec(rbuf, N_READ-1)) return false;

    // convert
    for (int i = 0; i < N_PIXEL; ++i) {
        int16_t raw = conv_s16_le(rbuf, 2 + 2*i);
        pix_data[i] = raw / 10.0;
    }
    return true;
}

int main() {
    // Prepare OpenCV windows
    cv::namedWindow("Sensor0", cv::WINDOW_AUTOSIZE);
    cv::namedWindow("Sensor1", cv::WINDOW_AUTOSIZE);
    cv::namedWindow("Sensor2", cv::WINDOW_AUTOSIZE);
    cv::namedWindow("Sensor3", cv::WINDOW_AUTOSIZE);

    // main loop
    while (true) {
        for (int chan = 0; chan < 4; ++chan) {
            if (!tca_select(chan)) continue;
            usleep(50000); // allow bus settle
            // initial setting once per sensor
            uint8_t init_dat[] = {D6T_SET_ADD, static_cast<uint8_t>(((D6T_IIR<<4)&0xF0)|(D6T_AVERAGE&0x0F))};
            int fd = open(I2C_DEV, O_RDWR);
            ioctl(fd, I2C_SLAVE, D6T_ADDR);
            write(fd, init_dat, sizeof(init_dat));
            close(fd);
            usleep(200000);

            if (!read_d6t()) continue;
            // build Mat
            cv::Mat img(N_ROW, N_ROW, CV_32F, pix_data);
            // normalize to 0-255
            cv::Mat norm;
            cv::normalize(img, norm, 0, 255, cv::NORM_MINMAX);
            norm.convertTo(norm, CV_8U);
            cv::applyColorMap(norm, norm, cv::COLORMAP_JET);
            // show
            cv::imshow("Sensor" + std::to_string(chan), norm);
        }
        if (cv::waitKey(30) >= 0) break;
    }
    return 0;
}
