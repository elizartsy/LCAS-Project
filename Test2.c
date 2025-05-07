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
#define MAX_CHANNELS  4         // Number of thermal sensors/cameras

#define N_ROW         32
#define N_PIXEL       (N_ROW * N_ROW)
#define N_READ        ((N_PIXEL + 1) * 2 + 1)

#define I2C_DEV       "/dev/i2c-1"
#define RST_GPIO_PIN  23        // BCM pin for hardware reset

static int               g_fd = -1;
static struct gpiod_chip *chip = NULL;
static struct gpiod_line *lineRST = NULL;
uint8_t      rbuf[N_READ];
double       pix_data[N_PIXEL];

typedef struct {
    int fd;
    cv::VideoCapture cap;
} SensorCam;

// ********************* I2C Helpers *********************
int i2c_write(int fd, const uint8_t *data, size_t len) {
    return write(fd, data, len) == (ssize_t)len ? 0 : -1;
}

int i2c_read_reg(int fd, uint8_t reg, uint8_t *buf, size_t len) {
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
bool checkPEC(const uint8_t buf[], int n) {
    uint8_t crc = calc_crc((D6T_ADDR << 1) | 1);
    for (int i = 0; i < n; i++) crc = calc_crc(buf[i] ^ crc);
    return crc != buf[n];
}
int16_t conv16_le(const uint8_t* b, int i) {
    return (int16_t)(b[i] | (b[i+1] << 8));
}

// ********************* TCA9548A & D6T *********************
void hardware_reset(void) {
    if (!lineRST) return;
    gpiod_line_set_value(lineRST, 0);
    usleep(10*1000);
    gpiod_line_set_value(lineRST, 1);
    usleep(50*1000);
}

void reset_switch(int fd) {
    ioctl(fd, I2C_SLAVE, TCA_ADDR);
    uint8_t z = 0;
    i2c_write(fd, &z,1);
    usleep(50*1000);
}

bool select_channel(int fd, uint8_t ch) {
    if (ch >= MAX_CHANNELS) return false;
    if (ioctl(fd, I2C_SLAVE, TCA_ADDR) < 0) return false;
    uint8_t cmd = 1u << ch;
    if (i2c_write(fd, &cmd,1) < 0) return false;
    usleep(50*1000);
    return ioctl(fd, I2C_SLAVE, D6T_ADDR) == 0;
}

void initialSetting(int fd, uint8_t ch) {
    if (!select_channel(fd,ch)) return;
    uint8_t dat[] = { D6T_SET_ADD, uint8_t((D6T_IIR<<4)|(D6T_AVERAGE & 0x0F)) };
    if (i2c_write(fd, dat, sizeof(dat))<0)
        perror("initialSetting");
}

// ********************* Thermal Capture *********************
bool readThermal(int fd, double *outBuf) {
    if (i2c_read_reg(fd, D6T_CMD, rbuf, N_READ) < 0) return false;
    if (checkPEC(rbuf, N_READ-1)) return false;
    // skip PTAT at rbuf[0..1]
    for(int i=0;i<N_PIXEL;i++) outBuf[i] = conv16_le(rbuf, 2+2*i)/10.0;
    return true;
}

cv::Mat toColorMap(const double *buf) {
    cv::Mat m(N_ROW,N_ROW,CV_64F,(void*)buf);
    cv::Mat n8, c;
    cv::normalize(m, n8, 0, 255, cv::NORM_MINMAX);
    n8.convertTo(n8, CV_8U);
    cv::applyColorMap(n8, c, cv::COLORMAP_JET);
    return c;
}

// ********************* Signal Handling *********************
void cleanup(int code) {
    hardware_reset();
    if (g_fd>=0) close(g_fd);
    if (lineRST) { gpiod_line_release(lineRST); }
    if (chip)    { gpiod_chip_close(chip); }
    exit(code);
}
void sigint_handler(int) { cleanup(0); }

// ********************* Main *****************************
int main() {
    signal(SIGINT, sigint_handler);

    // GPIO init for reset
    chip = gpiod_chip_open_by_name("gpiochip0");
    lineRST = gpiod_chip_get_line(chip, RST_GPIO_PIN);
    gpiod_line_request_output(lineRST,"tca_rst",1);
    hardware_reset();

    // Open I2C bus
    int fd = open(I2C_DEV, O_RDWR);
    if (fd<0) { perror("open i2c"); return 1; }
    g_fd = fd;
    reset_switch(fd);

    // Thermal init
    for(uint8_t ch=0;ch<MAX_CHANNELS;ch++) initialSetting(fd,ch);

    // Open cameras
    std::vector<SensorCam> cams;
    for(int i=0;i<MAX_CHANNELS;i++) {
        SensorCam sc{fd, cv::VideoCapture(i)};
        if (!sc.cap.isOpened()) {
            fprintf(stderr,"Failed to open camera %d\n",i);
            cleanup(1);
        }
        sc.cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
        sc.cap.set(cv::CAP_PROP_FRAME_HEIGHT,480);
        cams.push_back(sc);
    }

    // Live loop
    while(true) {
        for(int ch=0; ch<MAX_CHANNELS; ch++) {
            // Thermal
            if (select_channel(fd,ch)) {
                double buf[N_PIXEL];
                if (readThermal(fd,buf)) {
                    auto thermalImg = toColorMap(buf);
                    cv::imshow("Thermal " + std::to_string(ch), thermalImg);
                }
            }
            // Video
            cv::Mat frame;
            cams[ch].cap >> frame;
            if (!frame.empty()) {
                cv::imshow("Camera " + std::to_string(ch), frame);
            }
        }
        if (cv::waitKey(30) >= 0) break;
    }

    cleanup(0);
    return 0;
}
