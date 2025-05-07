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
    if (write(i2c_fd, &zero, 1) != 1)
        fprintf(stderr, "[WARN] disable_all_channels write failed: %s\n", strerror(errno));
    usleep(50000);
}

// Select TCA channel
bool select_channel(u8 ch) {
    disable_all_channels();
    if (ioctl(i2c_fd, I2C_SLAVE, TCA_ADDR) < 0) {
        perror("[ERR] ioctl TCA_ADDR");
        return false;
    }
    u8 cmd = (1 << ch);
    if (write(i2c_fd, &cmd, 1) != 1) {
        fprintf(stderr, "[ERR] select_channel %d write failed: %s\n", ch, strerror(errno));
        return false;
    }
    usleep(50000);
    return true;
}

// Point to D6T device
bool select_sensor() {
    if (ioctl(i2c_fd, I2C_SLAVE, D6T_ADDR) < 0) {
        perror("[ERR] ioctl D6T_ADDR");
        return false;
    }
    return true;
}

// Initialize a sensor channel
void init_channel(u8 ch) {
    if (!select_channel(ch) || !select_sensor()) {
        fprintf(stderr, "[WARN] init_channel %d select failed\n", ch);
        return;
    }
    u8 cfg[2] = { D6T_SET_ADD, (u8)((D6T_IIR<<4)|(D6T_AVERAGE&0x0F)) };
    if (write(i2c_fd, cfg, 2) != 2)
        fprintf(stderr, "[ERR] init_channel %d write cfg failed: %s\n", ch, strerror(errno));

    usleep(50000);
}

// Read one frame
cv::Mat read_frame(u8 ch) {
    if (!select_channel(ch) || !select_sensor()) {
        fprintf(stderr, "[WARN] read_frame %d select failed\n", ch);
        return cv::Mat();
    }
    u8 cmd = D6T_CMD;
    if (write(i2c_fd, &cmd, 1) != 1) {
        fprintf(stderr, "[ERR] read_frame %d write cmd failed: %s\n", ch, strerror(errno));
        return cv::Mat();
    }
    ssize_t r = read(i2c_fd, rbuf, N_READ);
    if (r != N_READ) {
        fprintf(stderr, "[ERR] read_frame %d read %zd/%d: %s\n", ch, r, N_READ, strerror(errno));
        return cv::Mat();
    }
    // Convert to temperature
    for (int i = 0; i < N_PIXEL; ++i) {
        int16_t v = (int16_t)(rbuf[2+2*i] | (rbuf[3+2*i] << 8));
        pix[i] = v / 10.0;
    }
    cv::Mat m(N_ROW, N_ROW, CV_64F, pix), norm, color;
    cv::normalize(m, norm, 0, 255, cv::NORM_MINMAX);
    norm.convertTo(norm, CV_8U);
    cv::applyColorMap(norm, color, cv::COLORMAP_JET);
    return color;
}

void cleanup(int code) {
    if (i2c_fd >= 0) close(i2c_fd);
    if (line) gpiod_line_release(line);
    if (chip) gpiod_chip_close(chip);
    exit(code);
}

void sigint_handler(int) {
    printf("\nSIGINT received, exiting...\n");
    cleanup(0);
}

int main() {
    signal(SIGINT, sigint_handler);
    
    // Setup GPIO reset
    chip = gpiod_chip_open_by_name("gpiochip0");
    if (!chip) { perror("[ERR] gpiod open"); return 1; }
    line = gpiod_chip_get_line(chip, RST_PIN);
    if (!line || gpiod_line_request_output(line, "rst", 1) < 0) {
        perror("[ERR] gpiod_line_request_output"); return 1;
    }
    // Hardware reset
    gpiod_line_set_value(line, 0);
    usleep(10000);
    gpiod_line_set_value(line, 1);
    usleep(50000);

    // Open I2C bus
    i2c_fd = open(I2C_DEV, O_RDWR);
    if (i2c_fd < 0) { perror("[ERR] open I2C"); return 1; }

    // Initialize sensors
    for (u8 ch = 0; ch < 4; ++ch) init_channel(ch);

    // Prepare windows
    for (int ch = 0; ch < 4; ++ch) {
        cv::namedWindow("Thermal " + std::to_string(ch+1), cv::WINDOW_AUTOSIZE);
    }

    // Live display
    while (true) {
        bool any = false;
        for (u8 ch = 0; ch < 4; ++ch) {
            cv::Mat frame = read_frame(ch);
            if (!frame.empty()) {
                cv::imshow("Thermal " + std::to_string(ch+1), frame);
                any = true;
            } else {
                fprintf(stderr, "[INFO] Channel %d frame empty\n", ch);
            }
        }
        if (!any) {
            fprintf(stderr, "[WARN] No data on any channel, retrying...\n");
        }
        int key = cv::waitKey(100);
        if (key == 27) break;
    }
    cleanup(0);
    return 0;
}
