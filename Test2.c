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
#include <stdlib.h>
#include <time.h>
#include <gpiod.h>

#define D6T_ADDR      0x0A      // Melexis D6T fixed address
#define D6T_CMD       0x4D      // Temperature read command
#define D6T_SET_ADD   0x01
#define D6T_IIR       0x00
#define D6T_AVERAGE   0x04

#define TCA_ADDR      0x70      // TCA9548A address
#define MAX_CHANNELS  8

#define N_ROW         32
#define N_PIXEL       (N_ROW * N_ROW)
#define N_READ        ((N_PIXEL + 1) * 2 + 1)

#define I2C_DEV       "/dev/i2c-1"
#define RST_GPIO_PIN  23        // BCM pin for TCA reset

static int g_fd = -1;
static struct gpiod_chip *chip = NULL;
static struct gpiod_line *lineRST = NULL;
uint8_t rbuf[N_READ];
double pix_data[N_PIXEL];

typedef uint8_t u8;

// I2C helpers
int i2c_write_bytes(int fd, const u8 *data, size_t len) {
    ssize_t w = write(fd, data, len);
    if (w < 0) perror("i2c write");
    return (w == (ssize_t)len) ? 0 : -1;
}
int i2c_read_bytes(int fd, u8 cmd, u8 *buf, size_t len) {
    if (write(fd, &cmd, 1) != 1) {
        perror("i2c write cmd");
        return -1;
    }
    ssize_t r = read(fd, buf, len);
    if (r != (ssize_t)len) {
        perror("i2c read data");
        return -1;
    }
    return 0;
}

// Reset TCA via GPIO
void hardware_reset(void) {
    if (!lineRST) return;
    gpiod_line_set_value(lineRST, 0);
    usleep(10000);
    gpiod_line_set_value(lineRST, 1);
    usleep(50000);
}

// Select a TCA channel
bool select_channel(int ch) {
    if (ch >= MAX_CHANNELS) return false;
    if (ioctl(g_fd, I2C_SLAVE, TCA_ADDR) < 0) {
        perror("TCA ioctl sel");
        return false;
    }
    u8 cmd = (1u << ch);
    if (i2c_write_bytes(g_fd, &cmd, 1) < 0) {
        fprintf(stderr, "TCA write channel %u failed\n", ch);
        return false;
    }
    usleep(50000);
    return true;
}

// Point to D6T device
bool select_sensor(void) {
    if (ioctl(g_fd, I2C_SLAVE, D6T_ADDR) < 0) {
        perror("D6T ioctl");
        return false;
    }
    return true;
}

// Initialize sensor at a given channel
void initialSetting(uint8_t ch) {
    if (!select_channel(ch) || !select_sensor()) {
        fprintf(stderr, "initialSetting: channel %u select failed\n", ch);
        return;
    }
    u8 cfg[] = { D6T_SET_ADD, (u8)((D6T_IIR << 4) | (D6T_AVERAGE & 0x0F)) };
    if (i2c_write_bytes(g_fd, cfg, sizeof(cfg)) < 0)
        fprintf(stderr, "initialSetting: write cfg failed on ch %u\n", ch);
    else
        usleep(50000);
}

// Capture one colored frame
cv::Mat capture_frame(uint8_t ch) {
    cv::Mat mat;
    if (!select_channel(ch) || !select_sensor()) return mat;

    int ret = -1;
    for (int i = 0; i < 5; i++) {
        ret = i2c_read_bytes(g_fd, D6T_CMD, rbuf, N_READ);
        if (ret == 0) break;
        usleep(60000);
    }
    if (ret < 0) {
        fprintf(stderr, "capture_frame: read failed on ch %u\n", ch);
        return mat;
    }

    // parse temperature pixels
    for (int i = 0; i < N_PIXEL; i++) {
        int16_t v = (int16_t)(rbuf[2+2*i] | (rbuf[3+2*i] << 8));
        pix_data[i] = v / 10.0;
    }

    cv::Mat src(N_ROW, N_ROW, CV_64F, pix_data);
    cv::Mat norm, color;
    cv::normalize(src, norm, 0, 255, cv::NORM_MINMAX);
    norm.convertTo(norm, CV_8U);
    cv::applyColorMap(norm, color, cv::COLORMAP_JET);
    return color;
}

// Cleanup and exit
void cleanup_and_exit(int code) {
    hardware_reset();
    if (g_fd >= 0) close(g_fd);
    if (lineRST) { gpiod_line_release(lineRST); }
    if (chip)    { gpiod_chip_close(chip); }
    exit(code);
}

void handle_sigint(int) {
    printf("\nSIGINT, cleaning up...\n");
    cleanup_and_exit(0);
}

int main() {
    signal(SIGINT, handle_sigint);

    // Prevent QStandardPaths warning
    setenv("XDG_RUNTIME_DIR", "/tmp/runtime-root", 1);

    // GPIO setup
    chip = gpiod_chip_open_by_name("gpiochip0");
    if (!chip) { perror("gpiochip open"); return 1; }
    lineRST = gpiod_chip_get_line(chip, RST_GPIO_PIN);
    if (!lineRST || gpiod_line_request_output(lineRST, "tca_rst", 1) < 0) {
        perror("line request"); cleanup_and_exit(1);
    }
    hardware_reset();

    // I2C open
    g_fd = open(I2C_DEV, O_RDWR);
    if (g_fd < 0) { perror("open i2c"); cleanup_and_exit(1); }

    // Init each sensor once
    for (uint8_t ch = 0; ch < 4; ch++) initialSetting(ch);

    // Prep windows
    for (uint8_t ch = 0; ch < 4; ch++) {
        cv::namedWindow("Thermal " + std::to_string(ch+1), cv::WINDOW_AUTOSIZE);
    }

    // Live loop
    while (true) {
        for (uint8_t ch = 0; ch < 4; ch++) {
            cv::Mat frame = capture_frame(ch);
            if (!frame.empty()) cv::imshow("Thermal " + std::to_string(ch+1), frame);
        }
        int key = cv::waitKey(30);
        if (key == 27) break;  // ESC
    }

    cleanup_and_exit(0);
    return 0;
}

