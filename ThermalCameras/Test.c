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
#include <gpiod.h>

#define D6T_ADDR      0x0A      // Melexis D6T fixed address
#define D6T_CMD       0x4D
#define D6T_SET_ADD   0x01
#define D6T_IIR       0x00
#define D6T_AVERAGE   0x04

#define TCA_ADDR      0x70      // TCA9548A address (A0–A2 tied to GND)
#define MAX_CHANNELS  8

#define N_ROW         32
#define N_PIXEL       (N_ROW * N_ROW)
#define N_READ        ((N_PIXEL + 1) * 2 + 1)

#define I2C_DEV       "/dev/i2c-1"
#define RST_GPIO_PIN  23        // Pi GPIO pin connected to TCA RESET (BCM numbering)

static int               g_fd = -1;
static struct gpiod_chip *chip = NULL;
static struct gpiod_line *lineRST = NULL;
uint8_t      rbuf[N_READ];
double       ptat;
double       pix_data[N_PIXEL];

// Low-level I2C write
int i2c_write(int fd, const uint8_t *data, size_t len) {
    return write(fd, data, len) == (ssize_t)len ? 0 : -1;
}
// Low-level I2C read
int i2c_read_reg(int fd, uint8_t reg, uint8_t *buf, size_t len) {
    if (write(fd, &reg, 1) != 1) return -1;
    if (read(fd, buf, len) != (ssize_t)len) return -1;
    return 0;
}
// CRC-8 PEC
uint8_t calc_crc(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        uint8_t temp = data;
        data <<= 1;
        if (temp & 0x80) data ^= 0x07;
    }
    return data;
}
bool D6T_checkPEC(const uint8_t buf[], int n) {
    uint8_t crc = calc_crc((D6T_ADDR << 1) | 1);
    for (int i = 0; i < n; i++) crc = calc_crc(buf[i] ^ crc);
    if (crc != buf[n]) {
        fprintf(stderr, "PEC failed: calc=0x%02X got=0x%02X\n", crc, buf[n]);
        return true;
    }
    return false;
}
int16_t conv16_le(const uint8_t* buf, int n) {
    return (int16_t)(buf[n] | (buf[n+1] << 8));
}

// Drive RESET pin low then high via GPIO
void hardware_reset(void) {
    if (!lineRST) return;
    gpiod_line_set_value(lineRST, 0);
    usleep(10 * 1000);
    gpiod_line_set_value(lineRST, 1);
    usleep(50 * 1000);
}

// Disable all TCA channels via I2C
void reset_switch(int fd) {
    if (ioctl(fd, I2C_SLAVE, TCA_ADDR) < 0) return;
    uint8_t zero = 0;
    i2c_write(fd, &zero, 1);
    usleep(50 * 1000);
}

// Select a single channel on the TCA
bool select_channel(int fd, uint8_t ch) {
    if (ch >= MAX_CHANNELS) return false;
    if (ioctl(fd, I2C_SLAVE, TCA_ADDR) < 0) return false;
    uint8_t cmd = 1u << ch;
    if (i2c_write(fd, &cmd, 1) < 0) return false;
    usleep(50 * 1000);
    return true;
}
// Re-point bus to D6T sensor
bool point_to_D6T(int fd) {
    return ioctl(fd, I2C_SLAVE, D6T_ADDR) == 0;
}

void initialSetting(int fd, uint8_t ch) {
    if (!select_channel(fd, ch) || !point_to_D6T(fd)) return;
    uint8_t dat[] = { D6T_SET_ADD, uint8_t((D6T_IIR<<4)|(D6T_AVERAGE & 0x0F)) };
    if (i2c_write(fd, dat, sizeof(dat)) < 0)
        perror("initialSetting");
}

void capture_and_display(int fd, uint8_t ch) {
    if (!select_channel(fd, ch) || !point_to_D6T(fd)) return;
    int ret = -1;
    for (int i = 0; i < 5; i++) {
        ret = i2c_read_reg(fd, D6T_CMD, rbuf, N_READ);
        if (ret == 0) break;
        usleep(60 * 1000);
    }
    if (ret < 0 || D6T_checkPEC(rbuf, N_READ-1)) return;
    ptat = conv16_le(rbuf, 0) / 10.0;
    for (int i = 0; i < N_PIXEL; i++)
        pix_data[i] = conv16_le(rbuf, 2 + 2*i) / 10.0;

    cv::Mat m(N_ROW, N_ROW, CV_64F, pix_data), n, c;
    cv::normalize(m, n, 0, 255, cv::NORM_MINMAX);
    n.convertTo(n, CV_8U);
    cv::applyColorMap(n, c, cv::COLORMAP_JET);

    char win[32];
    snprintf(win, sizeof(win), "Thermal %u", ch+1);
    cv::imshow(win, c);
    cv::waitKey(1);
}

// Cleanup on exit: hardware reset, close I2C and GPIO
void cleanup_and_exit(int code) {
    hardware_reset();
    if (g_fd >= 0) close(g_fd);
    if (lineRST) {
        gpiod_line_release(lineRST);
        lineRST = NULL;
    }
    if (chip) {
        gpiod_chip_close(chip);
        chip = NULL;
    }
    exit(code);
}

void handle_sigint(int sig) {
    (void)sig;
    printf("\nSIGINT received, cleaning up...\n");
    cleanup_and_exit(0);
}

int main() {
    signal(SIGINT, handle_sigint);

    // Initialize GPIO for reset
    chip = gpiod_chip_open_by_name("gpiochip0");
    if (!chip) {
        perror("gpiod_chip_open_by_name");
        return 1;
    }
    lineRST = gpiod_chip_get_line(chip, RST_GPIO_PIN);
    if (!lineRST || gpiod_line_request_output(lineRST, "tca_rst", 1) < 0) {
        perror("gpiod_line_request_output");
        cleanup_and_exit(1);
    }

    // Hardware reset at startup
    hardware_reset();

    // Open I2C bus
    int fd = open(I2C_DEV, O_RDWR);
    if (fd < 0) {
        perror("open i2c");
        cleanup_and_exit(1);
    }
    g_fd = fd;

    // Software reset of all channels
    reset_switch(fd);

    // Initial setting for first 4 sensors
    for (uint8_t ch = 0; ch < 4; ch++)
        initialSetting(fd, ch);

    // Main loop
    while (true) {
        for (uint8_t ch = 0; ch < 4; ch++) {
            capture_and_display(fd, ch);
            printf("Channel %u. Press ENTER to continue...\n", ch+1);
            getchar();
        }
    }

    // never reached
    cleanup_and_exit(0);
    return 0;
}
