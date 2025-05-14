/*
 * multi_d6t_display.c
 * Capture live feeds from four Omron D6T-32L sensors via TCA9548A and display a 2x2 mosaic.
 * Build with:
 *   gcc multi_d6t_display.c -o multi_d6t_display `pkg-config --cflags --libs opencv` 
 * Requires OpenCV C API and Linux I2C headers.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <time.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>

// I2C addresses
#define I2C_DEV         "/dev/i2c-1"
#define MUX_ADDR        0x70    // TCA9548A
#define D6T_ADDR        0x0A    // Omron D6T-32L
#define D6T_CMD         0x4D

#define N_ROW           32
#define N_COL           32
#define N_PIXEL         (N_ROW * N_COL)
#define N_BYTES         ((N_PIXEL + 1) * 2 + 1) // PTAT + pixels, 2 bytes each + 1 PEC

// Utility: sleep in milliseconds
static void msleep(int ms) {
    struct timespec ts = {.tv_sec = ms/1000, .tv_nsec = (ms%1000)*1000000};
    nanosleep(&ts, NULL);
}

// Select TCA9548A channel [0..7]
static int select_channel(int fd, int ch) {
    uint8_t cmd = 1u << ch;
    if (ioctl(fd, I2C_SLAVE, MUX_ADDR) < 0) return -1;
    if (write(fd, &cmd, 1) != 1) return -1;
    return 0;
}

// Initialize D6T sensor on current bus
static int init_sensor(int fd) {
    uint8_t buf[2] = {0x01, 0x04}; // SET_ADD, config (IIR=0,Aver=4)
    if (ioctl(fd, I2C_SLAVE, D6T_ADDR) < 0) return -1;
    if (write(fd, buf, 2) != 2) return -1;
    return 0;
}

// Read raw data into float array (32x32)
static int read_d6t(int fd, float *out) {
    uint8_t raw[N_BYTES];
    if (ioctl(fd, I2C_SLAVE, D6T_ADDR) < 0) return -1;
    // request block
    if (write(fd, (uint8_t[]){D6T_CMD}, 1) != 1) return -1;
    if (read(fd, raw, N_BYTES) != N_BYTES) return -1;
    // unpack: ignore PEC
    for (int i = 0; i < N_PIXEL; i++) {
        int idx = 2 + 2*i;
        int16_t v = (int16_t)(raw[idx] | (raw[idx+1] << 8));
        out[i] = v / 10.0f;
    }
    return 0;
}

int main() {
    int fd = open(I2C_DEV, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Failed to open I2C: %s\n", strerror(errno));
        return 1;
    }

    // Initialize all four sensors
    for (int ch = 0; ch < 4; ch++) {
        if (select_channel(fd, ch) < 0) {
            fprintf(stderr, "Mux select failed on ch %d\n", ch);
            return 2;
        }
        msleep(50);
        if (init_sensor(fd) < 0) {
            fprintf(stderr, "Sensor init failed on ch %d\n", ch);
            return 3;
        }
    }
    msleep(400);

    // Prepare OpenCV windows and images
    cvNamedWindow("Thermal Mosaic", CV_WINDOW_AUTOSIZE);
    IplImage *tile[4];
    for (int i = 0; i < 4; i++) {
        tile[i] = cvCreateImage(cvSize(256,256), IPL_DEPTH_8U, 1);
    }
    IplImage *mosaic = cvCreateImage(cvSize(512,512), IPL_DEPTH_8U, 1);

    float buf_pixels[N_PIXEL];
    uint8_t norm[N_PIXEL];

    while (1) {
        // Read each channel
        for (int ch = 0; ch < 4; ch++) {
            select_channel(fd, ch);
            msleep(20);
            if (read_d6t(fd, buf_pixels) < 0) {
                fprintf(stderr, "Read failed on ch %d\n", ch);
                continue;
            }
            // Normalize 20-40C to 0-255
            for (int i = 0; i < N_PIXEL; i++) {
                float v = (buf_pixels[i] - 20.0f) * (255.0f/20.0f);
                if (v < 0) v = 0; else if (v > 255) v = 255;
                norm[i] = (uint8_t)v;
            }
            // Fill and resize to 256x256
            IplImage *raw32 = cvCreateImageHeader(cvSize(32,32), IPL_DEPTH_8U, 1);
            cvSetData(raw32, norm, 32);
            cvResize(raw32, tile[ch], CV_INTER_NN);
            cvReleaseImageHeader(&raw32);
        }
        // Build 2x2 mosaic
        cvSetImageROI(mosaic, cvRect(0,   0, 256,256)); cvCopy(tile[0], mosaic, NULL);
        cvSetImageROI(mosaic, cvRect(256, 0, 256,256)); cvCopy(tile[1], mosaic, NULL);
        cvSetImageROI(mosaic, cvRect(0, 256, 256,256)); cvCopy(tile[2], mosaic, NULL);
        cvSetImageROI(mosaic, cvRect(256,256,256,256)); cvCopy(tile[3], mosaic, NULL);
        cvResetImageROI(mosaic);

        cvShowImage("Thermal Mosaic", mosaic);
        int key = cvWaitKey(1);
        if ((key & 0xFF) == 27) break; // Esc
    }

    // Cleanup
    for (int i = 0; i < 4; i++) cvReleaseImage(&tile[i]);
    cvReleaseImage(&mosaic);
    cvDestroyAllWindows();
    close(fd);
    return 0;
}
