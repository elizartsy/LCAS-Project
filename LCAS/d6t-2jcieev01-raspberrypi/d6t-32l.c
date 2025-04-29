/*
 * MIT License
 * Copyright (c) 2019, 2018 - present OMRON Corporation
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/* includes */
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
#include <linux/i2c.h> //add
#include <wiringPi.h>

/* defines */
#define D6T_ADDR 0x0A  // for I2C 7bit address
#define D6T_CMD 0x4D  // for D6T-32L-01A, compensated output.
#define D6T_SET_ADD 0x01

// for use with PCA9546 4-channel I2C multiplexer.
#define A0_PIN 0  // GPIO 17
#define A1_PIN 1  // GPIO 18
#define A2_PIN 2  // GPIO 27

#define N_ROW 32
#define N_PIXEL (32 * 32)
#define N_READ ((N_PIXEL + 1) * 2 + 1)
#define RASPBERRY_PI_I2C    "/dev/i2c-1"
#define I2CDEV              RASPBERRY_PI_I2C

uint8_t rbuf[N_READ];
double ptat;
double pix_data[N_PIXEL];

/******* setting parameter *******/
#define D6T_IIR 0x00 
#define D6T_AVERAGE 0x04  
/*********************************/

 void select_channel(uint8_t channel) {
    switch (channel) {
        case 0:
            digitalWrite(A0_PIN, LOW);
            digitalWrite(A1_PIN, LOW);
            digitalWrite(A2_PIN, LOW);
            break;
        case 1:
            digitalWrite(A0_PIN, HIGH);
            digitalWrite(A1_PIN, LOW);
            digitalWrite(A2_PIN, LOW);
            break;
        case 2:
            digitalWrite(A0_PIN, LOW);
            digitalWrite(A1_PIN, HIGH);
            digitalWrite(A2_PIN, LOW);
            break;
        case 3:
            digitalWrite(A0_PIN, HIGH);
            digitalWrite(A1_PIN, HIGH);
            digitalWrite(A2_PIN, LOW);
            break;
        default:
            fprintf(stderr, "Invalid channel selection!\n");
            break;
    }
}

uint32_t i2c_read_reg8(uint8_t channel, uint8_t devAddr, uint8_t regAddr,
    uint8_t *data, int length) {

    int fd = open(I2CDEV, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Failed to open device: %s\n", strerror(errno));
        return 21;
    }

    select_channel(channel);

    int err = 0;
    do {
        struct i2c_msg messages[] = {
            { devAddr, 0, 1, &regAddr },
            { devAddr, I2C_M_RD, length, data },
        };
        struct i2c_rdwr_ioctl_data ioctl_data = { messages, 2 };
        if (ioctl(fd, I2C_RDWR, &ioctl_data) != 2) {
            fprintf(stderr, "i2c_read: failed to ioctl: %s\n", strerror(errno));
        }

    } while (false);

    close(fd);
    return err;
}

uint32_t i2c_write_reg8(uint8_t channel, uint8_t devAddr,
                        uint8_t *data, int length) {

    int fd = open(I2CDEV, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Failed to open device: %s\n", strerror(errno));
        return 21;
    }

    select_channel(channel);

    int err = 0;
    do {
        if (ioctl(fd, I2C_SLAVE, devAddr) < 0) {
            fprintf(stderr, "Failed to select device: %s\n", strerror(errno));
            err = 22; break;
        }
        if (write(fd, data, length) != length) {
            fprintf(stderr, "Failed to write reg: %s\n", strerror(errno));
            err = 23; break;
        }
    } while (false);

    close(fd);
    return err;
}

uint8_t calc_crc(uint8_t data) {
    int index;
    uint8_t temp;
    for (index = 0; index < 8; index++) {
        temp = data;
        data <<= 1;
        if (temp & 0x80) { data ^= 0x07; }
    }
    return data;
}

bool D6T_checkPEC(uint8_t buf[], int n) {
    int i;
    uint8_t crc = calc_crc((D6T_ADDR << 1) | 1);
    for (i = 0; i < n; i++) {
        crc = calc_crc(buf[i] ^ crc);
    }
    bool ret = crc != buf[n];
    if (ret) {
        fprintf(stderr,
            "PEC check failed: %02X(cal)-%02X(get)\n", crc, buf[n]);
    }
    return ret;
}

int16_t conv8us_s16_le(uint8_t* buf, int n) {
    uint16_t ret;
    ret = (uint16_t)buf[n];
    ret += ((uint16_t)buf[n + 1]) << 8;
    return (int16_t)ret;
}

void delay(int msec) {
    struct timespec ts = { .tv_sec = msec / 1000,
                          .tv_nsec = (msec % 1000) * 1000000 };
    nanosleep(&ts, NULL);
}

void initialSetting(void) {
    uint8_t dat1[] = {D6T_SET_ADD, (((uint8_t)D6T_IIR << 4)&&0xF0) | (0x0F && (uint8_t)D6T_AVERAGE)};
    i2c_write_reg8(0, D6T_ADDR, dat1, sizeof(dat1));
}

void setup_gpio() {
    if (wiringPiSetup() == -1) {
        fprintf(stderr, "WiringPi setup failed: %s\n", strerror(errno));
        exit(1);
    }
    pinMode(A0_PIN, OUTPUT);
    pinMode(A1_PIN, OUTPUT);
    pinMode(A2_PIN, OUTPUT);
}

void capture_and_display_thermal_image(uint8_t channel) {
    if (channel < 1 || channel > 4) {
        fprintf(stderr, "Invalid channel: %d. Must be 1-4.\n", channel);
        return;
    }

    memset(rbuf, 0, N_READ);

    for (int retry = 0; retry < 10; retry++) {
        if (i2c_read_reg8(channel - 1, D6T_ADDR, D6T_CMD, rbuf, N_READ) == 0) {
            break;
        } else {
            delay(60);
        }
    }

    if (D6T_checkPEC(rbuf, N_READ - 1)) {
        fprintf(stderr, "PEC check failed\n");
        return;
    }

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
    setup_gpio();
    initialSetting();

    while (true) {
        for (uint8_t channel = 1; channel <= 4; channel++) {
            capture_and_display_thermal_image(channel);
            printf("Press any key to capture the next channel...\n");
            getchar(); // Wait for a key press before moving to the next channel
        }
    }

    return 0;
}
