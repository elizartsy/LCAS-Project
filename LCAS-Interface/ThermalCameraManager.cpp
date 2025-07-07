#include "ThermalCameraManager.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <sys/stat.h>

static void delay(int ms) {
    timespec ts = {.tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000000};
    nanosleep(&ts, nullptr);
}

static uint8_t calc_crc(uint8_t data) {
    for (int i = 0; i < 8; ++i)
        data = (data & 0x80) ? (data << 1) ^ 0x07 : (data << 1);
    return data;
}

static bool D6T_checkPEC(uint8_t buf[], int n) {
    uint8_t crc = calc_crc((ThermalCameraManager::D6T_ADDR << 1) | 1);
    for (int i = 0; i < n; i++)
        crc = calc_crc(buf[i] ^ crc);
    if (crc != buf[n]) {
        fprintf(stderr, "PEC check failed: %02X != %02X\n", crc, buf[n]);
        return true;
    }
    return false;
}

static int16_t conv8us_s16_le(uint8_t* buf, int n) {
    return (int16_t)((buf[n + 1] << 8) | buf[n]);
}

static int i2c_write(uint8_t addr, uint8_t* data, int length) {
    int fd = open(ThermalCameraManager::I2C_DEV, O_RDWR);
    if (fd < 0) return perror("open i2c"), -1;
    if (ioctl(fd, I2C_SLAVE, addr) < 0) return perror("ioctl I2C_SLAVE"), -1;
    int res = write(fd, data, length);
    close(fd);
    return res == length ? 0 : -1;
}

static int i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t* data, int len) {
    int fd = open(ThermalCameraManager::I2C_DEV, O_RDWR);
    if (fd < 0) return perror("open i2c"), -1;
    struct i2c_msg msgs[2] = {
        {addr, 0, 1, &reg},
        {addr, I2C_M_RD, (uint16_t)len, data}
    };
    struct i2c_rdwr_ioctl_data ioctl_data = {msgs, 2};
    int result = ioctl(fd, I2C_RDWR, &ioctl_data);
    close(fd);
    return result == 2 ? 0 : -1;
}

ThermalCameraManager::ThermalCameraManager(int numCameras) {
    pixelData.resize(N_PIXEL);
}

ThermalCameraManager::~ThermalCameraManager() = default;

void ThermalCameraManager::resetMux() {
    int fd = open(GPIO_CHIP, O_RDONLY);
    if (fd < 0) { perror("open gpiochip"); return; }

    struct gpiohandle_request req = {};
    req.lineoffsets[0] = GPIO_LINE;
    req.flags = GPIOHANDLE_REQUEST_OUTPUT;
    req.lines = 1;
    req.default_values[0] = 0;
    strcpy(req.consumer_label, "mux_reset");

    if (ioctl(fd, GPIO_GET_LINEHANDLE_IOCTL, &req) < 0) {
        perror("ioctl GPIO_GET_LINEHANDLE_IOCTL");
        close(fd);
        return;
    }

    struct gpiohandle_data data = {};
    data.values[0] = 0;
    ioctl(req.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
    delay(10);
    data.values[0] = 1;
    ioctl(req.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
    close(req.fd);
    close(fd);
}

void ThermalCameraManager::selectMuxChannel(int channel) {
    uint8_t data = 1 << channel;
    i2c_write(MUX_ADDR, &data, 1);
}

void ThermalCameraManager::initialSetting() {
    constexpr uint8_t D6T_SET_ADD = 0x01;
    constexpr uint8_t D6T_IIR = 0x00;
    constexpr uint8_t D6T_AVERAGE = 0x04;

    uint8_t dat1[] = {D6T_SET_ADD, (((uint8_t)D6T_IIR << 4) & 0xF0) | (0x0F & (uint8_t)D6T_AVERAGE)};
    i2c_write(D6T_ADDR, dat1, sizeof(dat1));
}

void ThermalCameraManager::initialize() {
    resetMux();
    delay(100);
    for (int cam = 0; cam < 4; ++cam) {
        selectMuxChannel(cam);
        initialSetting();
    }
}

cv::Mat ThermalCameraManager::fetchImage() {
    uint8_t rbuf[N_READ] = {0};
    for (int retry = 0; retry < 5; retry++) {
        if (i2c_read_reg(D6T_ADDR, D6T_CMD, rbuf, N_READ) == 0 &&
            !D6T_checkPEC(rbuf, N_READ - 1)) break;
    }

    ptat = (double)conv8us_s16_le(rbuf, 0) / 10.0;
    for (int i = 0; i < N_PIXEL; i++) {
        int16_t itemp = conv8us_s16_le(rbuf, 2 + 2 * i);
        pixelData[i] = (double)itemp / 10.0;
    }

    cv::Mat thermal(N_ROW, N_ROW, CV_64F, pixelData.data());
    cv::Mat display;
    thermal.convertTo(display, CV_8U, 255.0 / 50.0);
    cv::applyColorMap(display, display, cv::COLORMAP_JET);
    return display;
}

cv::Mat ThermalCameraManager::getThermalFrame(int camIndex) {
    QMutexLocker locker(&mutex);
    selectMuxChannel(camIndex);
    return fetchImage();
}

bool ThermalCameraManager::checkAndSaveIfThresholdExceeded(int camIndex, const cv::Mat& displayImage) {
    for (int i = 0; i < N_PIXEL; i++) {
        if (pixelData[i] > TEMP_THRESHOLD) {
            time_t now = time(0);
            struct tm* t = localtime(&now);
            char filename[256];
            snprintf(filename, sizeof(filename), "thermal_alerts/cam%d_%04d%02d%02d_%02d%02d%02d.jpg",
                     camIndex, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                     t->tm_hour, t->tm_min, t->tm_sec);
            mkdir("thermal_alerts", 0755);
            cv::imwrite(filename, displayImage);
            return true;
        }
    }
    return false;
}

void ThermalCameraManager::tempThresChange(double tempChange) {
    TEMP_THRESHOLD = tempChange;
}