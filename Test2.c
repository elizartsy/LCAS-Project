#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>

#define D6T_ADDR 0x0A
#define D6T_CMD 0x4D
#define D6T_SET_ADD 0x01

#define N_ROW 32
#define N_PIXEL (32 * 32)
#define N_READ ((N_PIXEL + 1) * 2 + 1)

#define I2C_DEV "/dev/i2c-1"
#define MUX_ADDR 0x70
#define GPIO_CHIP "/dev/gpiochip0"
#define GPIO_LINE 23

#define D6T_IIR 0x00
#define D6T_AVERAGE 0x04

uint8_t rbuf[N_READ];
double pix_data[N_PIXEL];
double ptat;

void delay(int ms) {
    struct timespec ts = {.tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000000};
    nanosleep(&ts, NULL);
}

uint8_t calc_crc(uint8_t data) {
    for (int i = 0; i < 8; ++i) {
        if (data & 0x80) data = (data << 1) ^ 0x07;
        else data <<= 1;
    }
    return data;
}

bool D6T_checkPEC(uint8_t buf[], int n) {
    uint8_t crc = calc_crc((D6T_ADDR << 1) | 1);
    for (int i = 0; i < n; i++) {
        crc = calc_crc(buf[i] ^ crc);
    }
    bool failed = crc != buf[n];
    if (failed) fprintf(stderr, "PEC check failed: %02X != %02X\n", crc, buf[n]);
    return failed;
}

int16_t conv8us_s16_le(uint8_t* buf, int n) {
    return (int16_t)((buf[n + 1] << 8) | buf[n]);
}

int i2c_write(uint8_t addr, uint8_t* data, int length) {
    int fd = open(I2C_DEV, O_RDWR);
    if (fd < 0) return perror("open i2c"), -1;
    if (ioctl(fd, I2C_SLAVE, addr) < 0) return perror("ioctl I2C_SLAVE"), -1;
    int res = write(fd, data, length);
    close(fd);
    return res == length ? 0 : -1;
}

int i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t* data, int len) {
    int fd = open(I2C_DEV, O_RDWR);
    if (fd < 0) return perror("open i2c"), -1;
    struct i2c_msg msgs[2] = {
        {addr, 0, 1, &reg},
        {addr, I2C_M_RD, (uint16_t)len, data}
    };
    struct i2c_rdwr_ioctl_data ioctl_data = {msgs, 2};
    int result = ioctl(fd, I2C_RDWR, &ioctl_data);
    close(fd);
    return result == 2 ? 0 : -1;
}

void initialSetting() {
    uint8_t dat1[] = {D6T_SET_ADD, (((uint8_t)D6T_IIR << 4) & 0xF0) | (0x0F & (uint8_t)D6T_AVERAGE)};
    i2c_write(D6T_ADDR, dat1, sizeof(dat1));
}

void reset_mux() {
    int fd = open(GPIO_CHIP, O_RDONLY);
    if (fd < 0) { perror("open gpiochip"); return; }

    struct gpiohandle_request req = {0};
    req.lineoffsets[0] = GPIO_LINE;
    req.flags = GPIOHANDLE_REQUEST_OUTPUT;
    req.lines = 1;
    req.default_values[0] = 0;
    strcpy(req.consumer_label, "mux_reset");

    if (ioctl(fd, GPIO_GET_LINEHANDLE_IOCTL, &req) < 0) {
        perror("ioctl GPIO_GET_LINEHANDLE_IOCTL");
        close(fd);
        return;
    }

    struct gpiohandle_data data = {.values[0] = 0};
    ioctl(req.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
    delay(10);
    data.values[0] = 1;
    ioctl(req.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
    close(req.fd);
    close(fd);
}

void select_mux_channel(int channel) {
    uint8_t data = 1 << channel;
    i2c_write(MUX_ADDR, &data, 1);
    delay(10);  // give it time to switch
}

cv::Mat get_thermal_image() {
    memset(rbuf, 0, N_READ);
    for (int retry = 0; retry < 5; retry++) {
        if (i2c_read_reg(D6T_ADDR, D6T_CMD, rbuf, N_READ) == 0 &&
            !D6T_checkPEC(rbuf, N_READ - 1)) break;
        delay(50);
    }

    ptat = (double)conv8us_s16_le(rbuf, 0) / 10.0;
    for (int i = 0; i < N_PIXEL; i++) {
        int16_t itemp = conv8us_s16_le(rbuf, 2 + 2 * i);
        pix_data[i] = (double)itemp / 10.0;
    }

    cv::Mat thermal(N_ROW, N_ROW, CV_64F, pix_data);
    cv::Mat display;
    thermal.convertTo(display, CV_8U, 255.0 / 50.0); // scale for display
    cv::applyColorMap(display, display, cv::COLORMAP_JET);
    return display;
}

int main() {
    reset_mux();
    delay(100);

    for (int cam = 0; cam < 4; cam++) {
        select_mux_channel(cam);
        delay(350);
        initialSetting();
        delay(390);
    }

    while (true) {
        for (int cam = 0; cam < 4; cam++) {
            select_mux_channel(cam);
            delay(50);
            cv::Mat img = get_thermal_image();
            std::string win = "Camera " + std::to_string(cam);
            cv::imshow(win, img);
        }
        if (cv::waitKey(200) == 27) break; // ESC to quit
    }
    return 0;
}
