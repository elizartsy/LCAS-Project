#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>
#include <thread>
#include <atomic>
#include <array>
#include <mutex>

#define D6T_ADDR      0x0A
#define D6T_CMD       0x4D
#define D6T_SET_ADD   0x01

#define N_ROW         32
#define N_PIXEL       (N_ROW * N_ROW)
#define N_READ        ((N_PIXEL + 1) * 2 + 1)

#define D6T_IIR       0x00
#define D6T_AVERAGE   0x04  // no averaging for fastest read

// I2C device paths created by i2c-mux overlay
const char* camera_dev[4] = {
    "/dev/i2c-10",
    "/dev/i2c-11",
    "/dev/i2c-13",
    "/dev/i2c-14"
};

static uint8_t rbuf[N_READ];
static double pix_data[N_PIXEL];
static double ptat_data[4];

std::array<cv::Mat,4> frames;
std::array<std::mutex,4> frame_mutex;
std::atomic<bool> running{true};

uint8_t calc_crc(uint8_t data) {
    for (int i = 0; i < 8; ++i) {
        if (data & 0x80) data = (data << 1) ^ 0x07;
        else data <<= 1;
    }
    return data;
}

bool D6T_checkPEC(uint8_t buf[], int n) {
    uint8_t crc = calc_crc((D6T_ADDR << 1) | 1);
    for (int i = 0; i < n; i++) {
        crc = calc_crc(buf[i] ^ crc);
    }
    if (crc != buf[n]) {
        fprintf(stderr, "PEC check failed: %02X != %02X\n", crc, buf[n]);
        return false;
    }
    return true;
}

int16_t conv8us_s16_le(uint8_t* buf, int n) {
    return (int16_t)((buf[n + 1] << 8) | buf[n]);
}

void initialSetting(int fd) {
    uint8_t dat1[] = { D6T_SET_ADD, ((uint8_t)D6T_IIR << 4) | (0x0F & (uint8_t)D6T_AVERAGE) };
    ioctl(fd, I2C_SLAVE, D6T_ADDR);
    write(fd, dat1, sizeof(dat1));
}

cv::Mat readFrame(int fd, int cam_index) {
    memset(rbuf, 0, N_READ);
    // single attempt, skip retries to minimize delay
    ioctl(fd, I2C_SLAVE, D6T_ADDR);
    struct i2c_rdwr_ioctl_data msgset;
    uint8_t cmd_buf = D6T_CMD;
    struct i2c_msg msgs[2] = {
        { D6T_ADDR, 0, 1, &cmd_buf },
        { D6T_ADDR, I2C_M_RD, (uint16_t)N_READ, rbuf }
    };
    msgset.msgs = msgs;
    msgset.nmsgs = 2;
    ioctl(fd, I2C_RDWR, &msgset);

    // option: skip PEC check or keep it
    //D6T_checkPEC(rbuf, N_READ - 1);

    ptat_data[cam_index] = conv8us_s16_le(rbuf, 0) / 10.0;
    for (int i = 0; i < N_PIXEL; i++) {
        pix_data[i] = conv8us_s16_le(rbuf, 2 + 2*i) / 10.0;
    }
    cv::Mat thermal(N_ROW, N_ROW, CV_64F, pix_data);
    cv::Mat display;
    thermal.convertTo(display, CV_8U, 255.0 / 50.0);
    cv::applyColorMap(display, display, cv::COLORMAP_JET);
    return display;
}

void cameraThread(int cam) {
    int fd = open(camera_dev[cam], O_RDWR);
    if (fd < 0) {
        perror("open camera device");
        return;
    }
    initialSetting(fd);
    while (running) {
        auto img = readFrame(fd, cam);
        {
            std::lock_guard<std::mutex> lk(frame_mutex[cam]);
            frames[cam] = img;
        }
        // small yield to avoid busy spin
        std::this_thread::yield();
    }
    close(fd);
}

int main() {
    // launch threads
    std::array<std::thread,4> workers;
    for (int i = 0; i < 4; ++i) {
        workers[i] = std::thread(cameraThread, i);
    }
    
    // display loop
    while (running) {
        for (int i = 0; i < 4; ++i) {
            cv::Mat img;
            {
                std::lock_guard<std::mutex> lk(frame_mutex[i]);
                img = frames[i];
            }
            if (!img.empty()) {
                cv::imshow("Camera " + std::to_string(i), img);
            }
        }
        if (cv::waitKey(1) == 27) {
            running = false;
        }
    }

    for (auto &t : workers) {
        if (t.joinable()) t.join();
    }
    return 0;
}
