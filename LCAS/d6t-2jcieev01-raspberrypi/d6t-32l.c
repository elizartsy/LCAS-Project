#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdbool.h>
#include <time.h>
#include <gpiod.h>

#define D6T_ADDR 0x0A
#define D6T_CMD  0x4D
#define D6T_SET_ADD 0x01

#define CH1_ADDR 0x70
#define CH2_ADDR 0x71
#define CH3_ADDR 0x72

#define A0_PIN 17
#define A1_PIN 27
#define A2_PIN 22

#define N_ROW 32
#define N_PIXEL (32 * 32)
#define N_READ ((N_PIXEL + 1) * 2 + 1)
#define RASPBERRY_PI_I2C "/dev/i2c-1"

#define D6T_IIR 0x00
#define D6T_AVERAGE 0x04

uint8_t rbuf[N_READ];
double ptat;
double pix_data[N_PIXEL];

struct gpiod_chip *chip;
struct gpiod_line *lineA0, *lineA1, *lineA2;

void delay(int msec) {
    usleep(msec * 1000);
}

int setup_gpio() {
    chip = gpiod_chip_open_by_name("gpiochip0");
    if (!chip) {
        perror("gpiod_chip_open_by_name");
        return -1;
    }

    lineA0 = gpiod_chip_get_line(chip, A0_PIN);
    lineA1 = gpiod_chip_get_line(chip, A1_PIN);
    lineA2 = gpiod_chip_get_line(chip, A2_PIN);

    if (!lineA0 || !lineA1 || !lineA2) {
        perror("gpiod_chip_get_line");
        return -1;
    }

    if (gpiod_line_request_output(lineA0, "thermal", 0) < 0 ||
        gpiod_line_request_output(lineA1, "thermal", 0) < 0 ||
        gpiod_line_request_output(lineA2, "thermal", 0) < 0) {
        perror("gpiod_line_request_output");
        return -1;
    }

    return 0;
}

void gpio_set(int pin, int value) {
    struct gpiod_line *line = NULL;
    switch (pin) {
        case A0_PIN: line = lineA0; break;
        case A1_PIN: line = lineA1; break;
        case A2_PIN: line = lineA2; break;
    }
    if (line) gpiod_line_set_value(line, value);
}

void select_channel(uint8_t channel) {
    switch (channel) {
        case 0:
            gpio_set(A0_PIN, 0);
            gpio_set(A1_PIN, 0);
            gpio_set(A2_PIN, 0);
            break;
        case 1:
            gpio_set(A0_PIN, 1);
            gpio_set(A1_PIN, 0);
            gpio_set(A2_PIN, 0);
            break;
        case 2:
            gpio_set(A0_PIN, 0);
            gpio_set(A1_PIN, 1);
            gpio_set(A2_PIN, 0);
            break;
        case 3:
            gpio_set(A0_PIN, 1);
            gpio_set(A1_PIN, 1);
            gpio_set(A2_PIN, 0);
            break;
        default:
            fprintf(stderr, "Invalid channel selection!\n");
            break;
    }
}

int i2c_write(int fd, uint8_t *data, size_t len) {
    return write(fd, data, len) == (ssize_t)len ? 0 : -1;
}

int i2c_read(int fd, uint8_t reg, uint8_t *buf, size_t len) {
    if (write(fd, &reg, 1) != 1) return -1;
    if (read(fd, buf, len) != (ssize_t)len) return -1;
    return 0;
}

uint8_t calc_crc(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        uint8_t temp = data;
        data <<= 1;
        if (temp & 0x80) data ^= 0x07;
    }
    return data;
}

bool D6T_checkPEC(uint8_t buf[], int n) {
    uint8_t crc = calc_crc((D6T_ADDR << 1) | 1);
    for (int i = 0; i < n; i++) {
        crc = calc_crc(buf[i] ^ crc);
    }
    if (crc != buf[n]) {
        fprintf(stderr, "PEC check failed: %02X(cal)-%02X(get)\n", crc, buf[n]);
        return true;
    }
    return false;
}

int16_t conv8us_s16_le(uint8_t* buf, int n) {
    return (int16_t)(buf[n] | (buf[n + 1] << 8));
}

void initialSetting(int fd) {
    uint8_t dat1[] = {D6T_SET_ADD, ((D6T_IIR << 4) & 0xF0) | (0x0F & D6T_AVERAGE)};
    i2c_write(fd, dat1, sizeof(dat1));
}

void capture_and_display_thermal_image(int fd, uint8_t channel) {
    if (channel < 1 || channel > 4) {
        fprintf(stderr, "Invalid channel: %d. Must be 1-4.\n", channel);
        return;
    }

    select_channel(channel - 1);
    delay(50);

    for (int retry = 0; retry < 10; retry++) {
        if (i2c_read(fd, D6T_CMD, rbuf, N_READ) == 0) break;
        delay(60);
    }

    if (D6T_checkPEC(rbuf, N_READ - 1)) return;

    ptat = (double)conv8us_s16_le(rbuf, 0) / 10.0;
    for (int j = 0; j < N_PIXEL; j++) {
        int16_t itemp = conv8us_s16_le(rbuf, 2 + 2 * j);
        pix_data[j] = (double)itemp / 10.0;
    }

    cv::Mat image(32, 32, CV_64F, pix_data);
    cv::Mat normalized, colored;

    cv::normalize(image, normalized, 0, 255, cv::NORM_MINMAX);
    normalized.convertTo(normalized, CV_8U);
    cv::applyColorMap(normalized, colored, cv::COLORMAP_JET);

    char windowName[32];
    snprintf(windowName, sizeof(windowName), "Thermal Image - Channel %d", channel);
    cv::imshow(windowName, colored);
    cv::waitKey(1);
}

int main() {
    if (setup_gpio() < 0) return 1;

    int fd = open(RASPBERRY_PI_I2C, O_RDWR);
    if (fd < 0) {
        perror("Failed to open I2C device");
        return 1;
    }

    if (ioctl(fd, I2C_SLAVE, CH1_ADDR) < 0) {
        perror("Failed to set I2C address");
        close(fd);
        return 1;
    }

    initialSetting(fd);

    while (true) {
        for (uint8_t channel = 1; channel <= 4; channel++) {
            capture_and_display_thermal_image(fd, channel);
            printf("Press ENTER to capture the next channel...\n");
            getchar();
        }
    }

    close(fd);
    gpiod_chip_close(chip);
    return 0;
}
