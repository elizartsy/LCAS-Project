/*
 * therm2x2.cpp
 * Raspberry Pi: Read four D6T thermal sensors via TCA9548A I2C multiplexer,
 * reset TCA channels via GPIO (ioctl), and display feeds in a 2x2 OpenCV window.
 */

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/gpio.h>
#include <time.h>
#include <opencv2/opencv.hpp>

/* Thermal sensor defines */
#define D6T_ADDR       0x0A
#define D6T_CMD        0x4D
#define D6T_SET_ADD    0x01
#define N_ROW          32
#define N_PIXEL        (N_ROW * N_ROW)
#define N_READ         ((N_PIXEL + 1) * 2 + 1)
#define I2C_DEVICE     "/dev/i2c-1"

/* TCA9548A defines */
#define TCA_ADDR       0x70
#define NUM_CHANNELS   4

/* GPIO reset pin (TCA reset) */
#define RESET_CHIP     0   /* GPIO line offset */
#define GPIO_DEVICE    "/dev/gpiochip0"

/* IIR / AVERAGE settings */
#define D6T_IIR        0x00
#define D6T_AVERAGE    0x04

/* Globals */
static uint8_t rbuf[N_READ];
static double pix_data[N_PIXEL];

/* Simple sleep in ms */
static void delay_ms(int ms) {
    struct timespec ts = { ms/1000, (ms%1000)*1000000 };
    nanosleep(&ts, NULL);
}

/* Switch TCA channel */
static int tca_select(int channel) {
    int fd = open(I2C_DEVICE, O_RDWR);
    if (fd < 0) { perror("open i2c"); return -1; }
    if (ioctl(fd, I2C_SLAVE, TCA_ADDR) < 0) { perror("ioctl tca"); close(fd); return -1; }
    uint8_t mask = 1 << channel;
    if (write(fd, &mask, 1) != 1) {
        perror("tca write"); close(fd); return -1;
    }
    close(fd);
    return 0;
}

/* GPIO reset via ioctl */
static int gpio_reset_chip(void) {
    int fd = open(GPIO_DEVICE, O_RDONLY);
    if (fd < 0) { perror("open gpiochip"); return -1; }
    struct gpiohandle_request req = {0};
    req.lineoffsets[0] = RESET_CHIP;
    req.flags = GPIOHANDLE_REQUEST_OUTPUT;
    req.lines = 1;
    if (ioctl(fd, GPIO_GET_LINEHANDLE_IOCTL, &req) < 0) {
        perror("gpio get handle"); close(fd); return -1; }
    struct gpiohandle_data data = { .values = {0} };
    ioctl(req.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
    delay_ms(10);
    data.values[0] = 1;
    ioctl(req.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
    close(req.fd);
    close(fd);
    return 0;
}

/* CRC for PEC */
static uint8_t calc_crc(uint8_t d) {
    for (int i = 0; i < 8; i++) {
        if (d & 0x80) d = (d << 1) ^ 0x07;
        else d <<= 1;
    }
    return d;
}

static bool D6T_checkPEC(uint8_t buf[], int n) {
    uint8_t crc = calc_crc((D6T_ADDR << 1) | 1);
    for (int i = 0; i < n; i++) crc = calc_crc(buf[i] ^ crc);
    if (crc != buf[n]) {
        fprintf(stderr, "PEC fail %02X!=%02X\n", crc, buf[n]);
        return false;
    }
    return true;
}

static int16_t conv_le16(const uint8_t *b, int idx) {
    return (int16_t)(b[idx] | (b[idx+1] << 8));
}

/* Initialize D6T sensor */
static void initialSetting(void) {
    int fd = open(I2C_DEVICE, O_RDWR);
    if (fd < 0) return;
    ioctl(fd, I2C_SLAVE, D6T_ADDR);
    uint8_t dat[2] = { D6T_SET_ADD, (uint8_t)((D6T_IIR << 4) | (D6T_AVERAGE & 0x0F)) };
    write(fd, dat, 2);
    close(fd);
    delay_ms(350);
}

int main() {
    // Prepare OpenCV window
    const std::string winName = "Thermal 2x2";
    cv::namedWindow(winName, cv::WINDOW_AUTOSIZE);

    // Sensor init
    initialSetting();
    delay_ms(390);

    while (true) {
        // Combined image (grayscale)
        cv::Mat big(N_ROW * 2, N_ROW * 2, CV_8UC1);

        for (int ch = 0; ch < NUM_CHANNELS; ch++) {
            // Reset & select
            gpio_reset_chip();
            tca_select(ch);
            delay_ms(50);

            // Read raw data
            int fd = open(I2C_DEVICE, O_RDWR);
            ioctl(fd, I2C_SLAVE, D6T_ADDR);
            uint8_t cmd = D6T_CMD;
            write(fd, &cmd, 1);
            read(fd, rbuf, N_READ);
            close(fd);
            D6T_checkPEC(rbuf, N_READ - 1);

            // Fill small tile
            cv::Mat tile(N_ROW, N_ROW, CV_8UC1);
            for (int i = 0; i < N_PIXEL; i++) {
                pix_data[i] = conv_le16(rbuf, 2 + 2*i) / 10.0;
                tile.data[i] = static_cast<uint8_t>(pix_data[i]);
            }

            // Copy into big
            int dx = (ch % 2) * N_ROW;
            int dy = (ch / 2) * N_ROW;
            tile.copyTo(big(cv::Rect(dx, dy, N_ROW, N_ROW)));
        }

        // Apply color map and display
        cv::Mat color;
        cv::applyColorMap(big, color, cv::COLORMAP_JET);
        cv::imshow(winName, color);

        if (cv::waitKey(1) == 27) break;  // ESC to exit
        delay_ms(200);
    }

    return 0;
}

