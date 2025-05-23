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
#include <linux/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <opencv2/opencv.hpp>

#define D6T_ADDR 0x0A
#define D6T_CMD 0x4D
#define D6T_SET_ADD 0x01
#define N_ROW 32
#define N_PIXEL (32 * 32)
#define N_READ ((N_PIXEL + 1) * 2 + 1)
#define I2CDEV "/dev/i2c-1"
#define GPIOCHIP "/dev/gpiochip0"
#define MUX_ADDR 0x70
#define MUX_RESET_GPIO 23
#define MAX_CHANNELS 8

using namespace cv;

uint8_t rbuf[N_READ];
double pix_data[N_PIXEL];
double ptat;

int i2c_write(uint8_t addr, uint8_t *data, int len) {
    int fd = open(I2CDEV, O_RDWR);
    if (fd < 0) return -1;
    ioctl(fd, I2C_SLAVE, addr);
    write(fd, data, len);
    close(fd);
    return 0;
}

int i2c_read(uint8_t devAddr, uint8_t regAddr, uint8_t* data, int length) {
    int fd = open(I2CDEV, O_RDWR);
    if (fd < 0) return -1;

    struct i2c_msg msgs[] = {
        { devAddr, 0, 1, &regAddr },
        { devAddr, I2C_M_RD, (uint16_t)length, data }
    };
    struct i2c_rdwr_ioctl_data ioctl_data = { msgs, 2 };
    int ret = ioctl(fd, I2C_RDWR, &ioctl_data);
    close(fd);
    return ret == 2 ? 0 : -1;
}

void delay(int ms) {
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000000 };
    nanosleep(&ts, NULL);
}

int16_t conv8us_s16_le(uint8_t* buf, int n) {
    return (int16_t)((buf[n + 1] << 8) | buf[n]);
}

void mux_reset() {
    int fd = open(GPIOCHIP, O_RDWR);
    struct gpiohandle_request req = {};
    strcpy(req.consumer_label, "mux_reset");
    req.lines = 1;
    req.lineoffsets[0] = MUX_RESET_GPIO;
    req.flags = GPIOHANDLE_REQUEST_OUTPUT;
    req.default_values[0] = 0;

    if (ioctl(fd, GPIO_GET_LINEHANDLE_IOCTL, &req) == -1) {
        perror("GPIO request failed");
        close(fd);
        return;
    }

    struct gpiohandle_data data = {};
    data.values[0] = 0;
    ioctl(req.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
    delay(100);

    data.values[0] = 1;
    ioctl(req.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
    delay(100);

    close(req.fd);
    close(fd);
}

void mux_select_channel(int ch) {
    if (ch < 0 || ch > 7) return;
    uint8_t cmd = 1 << ch;
    i2c_write(MUX_ADDR, &cmd, 1);
    delay(10);
}

void read_thermal_data() {
    memset(rbuf, 0, N_READ);
    i2c_read(D6T_ADDR, D6T_CMD, rbuf, N_READ);
    ptat = conv8us_s16_le(rbuf, 0) / 10.0;
    for (int i = 0; i < N_PIXEL; ++i) {
        int16_t t = conv8us_s16_le(rbuf, 2 + 2 * i);
        pix_data[i] = t / 10.0;
    }
}

Mat visualize_thermal_data() {
    Mat img(N_ROW, N_ROW, CV_8UC1);
    for (int i = 0; i < N_PIXEL; ++i) {
        int val = std::clamp((int)(pix_data[i] * 3), 0, 255);
        img.data[i] = (uint8_t)val;
    }
    Mat color;
    applyColorMap(img, color, COLORMAP_JET);
    return color;
}

int main() {
    mux_reset();

    namedWindow("Thermal Camera", WINDOW_AUTOSIZE);

    while (true) {
        for (int ch = 0; ch < MAX_CHANNELS; ++ch) {
            mux_select_channel(ch);
            delay(300); // let camera settle
            read_thermal_data();
            Mat frame = visualize_thermal_data();
            imshow("Thermal Camera", frame);
            waitKey(1);
        }
    }

    return 0;
}
