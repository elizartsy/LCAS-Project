#include <opencv2/opencv.hpp>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <gpiod.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define I2C_DEV       "/dev/i2c-1"
#define TCA_ADDR      0x70      // TCA9548A
#define D6T_ADDR      0x0A      // Melexis D6T
#define D6T_CMD       0x4D      // Read command
#define D6T_SET_ADD   0x01
#define D6T_IIR       0x00
#define D6T_AVERAGE   0x04

#define RST_PIN       23        // BCM
#define N_ROW         32
#define N_PIXEL       (N_ROW * N_ROW)
#define N_READ        ((N_PIXEL + 1) * 2 + 1)

static int i2c_fd = -1;
static struct gpiod_chip *chip = NULL;
static struct gpiod_line *line = NULL;
static uint8_t rbuf[N_READ];
static double pix[N_PIXEL];

typedef uint8_t u8;

// Disable all TCA channels
void disable_all_channels() {
    ioctl(i2c_fd, I2C_SLAVE, TCA_ADDR);
    u8 zero = 0;
    write(i2c_fd, &zero, 1);
    usleep(50000);
}

// Select TCA channel (only this channel will be active)
bool select_channel(u8 ch) {
    disable_all_channels();  // ensure other channels off
    if (ioctl(i2c_fd, I2C_SLAVE, TCA_ADDR) < 0) return false;
    u8 cmd = (1 << ch);
    if (write(i2c_fd, &cmd, 1) != 1) return false;
    usleep(50000);
    return true;
}

// Select D6T sensor
bool select_sensor() {
    return ioctl(i2c_fd, I2C_SLAVE, D6T_ADDR) == 0;
}

// Initialize one sensor channel
void init_channel(u8 ch) {
    if (!select_channel(ch) || !select_sensor()) return;
    u8 cfg[2] = { D6T_SET_ADD, (u8)((D6T_IIR<<4)|(D6T_AVERAGE&0x0F)) };
    write(i2c_fd, cfg, 2);
    usleep(50000);
}

// Read frame from a single channel
cv::Mat read_frame(u8 ch) {
    if (!select_channel(ch) || !select_sensor()) return cv::Mat();
    u8 cmd = D6T_CMD;
    if (write(i2c_fd, &cmd, 1) != 1) return cv::Mat();

    if (read(i2c_fd, rbuf, N_READ) != N_READ) return cv::Mat();
    for (int i = 0; i < N_PIXEL; ++i) {
        int16_t v = (int16_t)(rbuf[2+2*i] | (rbuf[3+2*i]<<8));
        pix[i] = v / 10.0;
    }
    cv::Mat m(N_ROW, N_ROW, CV_64F, pix), n, c;
    cv::normalize(m, n, 0, 255, cv::NORM_MINMAX);
    n.convertTo(n, CV_8U);
    cv::applyColorMap(n, c, cv::COLORMAP_JET);
    return c;
}

void cleanup(int code) {
    if (i2c_fd >= 0) close(i2c_fd);
    if (line) gpiod_line_release(line);
    if (chip) gpiod_chip_close(chip);
    exit(code);
}

void sigint(int) { cleanup(0); }

int main() {
    signal(SIGINT, sigint);

    // GPIO reset line
    chip = gpiod_chip_open_by_name("gpiochip0");
    if (!chip) { perror("chip"); return 1; }
    line = gpiod_chip_get_line(chip, RST_PIN);
    if (!line || gpiod_line_request_output(line, "rst", 1) < 0) { perror("line"); return 1; }
    // Hardware reset
    gpiod_line_set_value(line, 0); usleep(10000);
    gpiod_line_set_value(line, 1); usleep(50000);

    // Open I2C
    i2c_fd = open(I2C_DEV, O_RDWR);
    if (i2c_fd < 0) { perror("i2c open"); cleanup(1); }

    // Initialize each channel
    for (u8 ch = 0; ch < 4; ++ch) {
        init_channel(ch);
    }

    // Create windows
    for (int ch = 0; ch < 4; ++ch) {
        cv::namedWindow("Thermal " + std::to_string(ch+1), cv::WINDOW_AUTOSIZE);
    }

    // Live loop
    while (true) {
        for (u8 ch = 0; ch < 4; ++ch) {
            cv::Mat frame = read_frame(ch);
            if (!frame.empty()) {
                cv::imshow("Thermal " + std::to_string(ch+1), frame);
            }
        }
        if (cv::waitKey(100) == 27) break;  // ESC
    }

    cleanup(0);
    return 0;
}
