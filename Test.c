#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <signal.h>
#include <time.h>

#define D6T_ADDR      0x0A      // fixed address of the Melexis D6T
#define D6T_CMD       0x4D
#define D6T_SET_ADD   0x01
#define D6T_IIR       0x00
#define D6T_AVERAGE   0x04

#define TCA_ADDR      0x70      // address of your TCA9548A (A0–A2 tied to select this)
#define MAX_CHANNELS  8

#define N_ROW         32
#define N_PIXEL       (N_ROW * N_ROW)
#define N_READ        ((N_PIXEL + 1) * 2 + 1) // PTAT + all pixels + PEC byte

#define I2C_DEV       "/dev/i2c-1"

static int g_fd = -1;  // global for signal handler
uint8_t  rbuf[N_READ];
double   ptat;
double   pix_data[N_PIXEL];

// Low-level I²C write; returns 0 on success
int i2c_write(int fd, const uint8_t *data, size_t len) {
    return write(fd, data, len) == (ssize_t)len ? 0 : -1;
}

// Low-level I²C read via register write-then-read; returns 0 on success
int i2c_read_reg(int fd, uint8_t reg, uint8_t *buf, size_t len) {
    if (write(fd, &reg, 1) != 1) return -1;
    if (read(fd, buf, len) != (ssize_t)len) return -1;
    return 0;
}

// CRC-8 (PEC) for Melexis D6T
uint8_t calc_crc(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        uint8_t temp = data;
        data <<= 1;
        if (temp & 0x80) data ^= 0x07;
    }
    return data;
}

// Check packet error code; returns true on error
bool D6T_checkPEC(const uint8_t buf[], int n) {
    uint8_t crc = calc_crc((D6T_ADDR << 1) | 1);
    for (int i = 0; i < n; i++) {
        crc = calc_crc(buf[i] ^ crc);
    }
    if (crc != buf[n]) {
        fprintf(stderr, "PEC check failed: calc=0x%02X, got=0x%02X\n", crc, buf[n]);
        return true;
    }
    return false;
}

// Convert two little-endian bytes at buf[n], buf[n+1] into signed int16
int16_t conv16_le(const uint8_t* buf, int n) {
    return (int16_t)(buf[n] | (buf[n + 1] << 8));
}

// Reset (disable) all TCA channels
void reset_switch(int fd) {
    if (ioctl(fd, I2C_SLAVE, TCA_ADDR) < 0) {
        perror("ioctl to TCA for reset");
        return;
    }
    uint8_t zero = 0x00;
    if (i2c_write(fd, &zero, 1) < 0) {
        perror("Write reset to TCA");
    }
    usleep(100 * 1000);
}

// After selecting a channel, explicitly re-point to the D6T sensor
bool point_to_D6T(int fd) {
    if (ioctl(fd, I2C_SLAVE, D6T_ADDR) < 0) {
        perror("ioctl to D6T_ADDR");
        return false;
    }
    return true;
}

// Select one channel on the TCA9548A by writing (1<<channel) to it
bool select_channel(int fd, uint8_t channel) {
    if (channel >= MAX_CHANNELS) {
        fprintf(stderr, "Invalid TCA channel %u\n", channel);
        return false;
    }
    uint8_t cmd = 1u << channel;

    if (ioctl(fd, I2C_SLAVE, TCA_ADDR) < 0) {
        perror("ioctl to TCA");
        return false;
    }
    if (i2c_write(fd, &cmd, 1) < 0) {
        perror("Write channel-select to TCA");
        return false;
    }
    // Let the switch hardware settle (100 ms)
    usleep(100 * 1000);
    return true;
}

// After selecting a channel, do the D6T "initial setting"
void initialSetting(int fd, uint8_t ch) {
    if (!select_channel(fd, ch)) return;
    if (!point_to_D6T(fd)) return;

    uint8_t dat[] = {
        D6T_SET_ADD,
        uint8_t(((D6T_IIR << 4) & 0xF0) | (0x0F & D6T_AVERAGE))
    };
    if (i2c_write(fd, dat, sizeof(dat)) < 0) {
        perror("D6T initialSetting");
    }
}

// Capture, check PEC, and display the thermal image
void capture_and_display(int fd, uint8_t ch) {
    if (!select_channel(fd, ch)) return;
    if (!point_to_D6T(fd)) return;

    int ret;
    for (int retry = 0; retry < 5; retry++) {
        ret = i2c_read_reg(fd, D6T_CMD, rbuf, N_READ);
        if (ret == 0) break;
        usleep(60 * 1000);
    }
    if (ret < 0) {
        fprintf(stderr, "Read error on channel %u\n", ch);
        return;
    }
    if (D6T_checkPEC(rbuf, N_READ - 1)) return;

    ptat = conv16_le(rbuf, 0) / 10.0;
    for (int i = 0; i < N_PIXEL; i++) {
        pix_data[i] = conv16_le(rbuf, 2 + 2*i) / 10.0;
    }

    cv::Mat therm(N_ROW, N_ROW, CV_64F, pix_data);
    cv::Mat norm, color;

    cv::normalize(therm, norm, 0, 255, cv::NORM_MINMAX);
    norm.convertTo(norm, CV_8U);
    cv::applyColorMap(norm, color, cv::COLORMAP_JET);

    char win[32];
    snprintf(win, sizeof(win), "Thermal Channel %u", ch+1);
    cv::imshow(win, color);
    cv::waitKey(1);
}

// Handle Ctrl-C: reset switch and close bus
void handle_sigint(int sig) {
    if (g_fd >= 0) {
        reset_switch(g_fd);
        close(g_fd);
    }
    printf("\nExiting and reset TCA9548A.\n");
    exit(0);
}

int main() {
    signal(SIGINT, handle_sigint);

    int fd = open(I2C_DEV, O_RDWR);
    if (fd < 0) {
        perror("Open I2C device");
        return 1;
    }
    g_fd = fd;

    // Start from a clean slate: disable all channels
    reset_switch(fd);

    // Run initialSetting on each of the first 4 channels once
    for (uint8_t ch = 0; ch < 4; ch++) {
        initialSetting(fd, ch);
    }

    // Continuous capture
    while (true) {
        for (uint8_t ch = 0; ch < 4; ch++) {
            capture_and_display(fd, ch);
            printf("Captured channel %u. Press ENTER to continue…\n", ch+1);
            getchar();
        }
    }

    // never reached
    reset_switch(fd);
    close(fd);
    return 0;
}
