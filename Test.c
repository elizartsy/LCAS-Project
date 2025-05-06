/*
 * Thermal Array Display using libgpiod for I2C bit-banging and OpenCV for visualization
 *
 * Wiring assumptions:
 * - TCA9546A multiplexer on hardware I2C (SDA=GPIO2, SCL=GPIO3)
 * - Four D6T-32L sensors on channels 0..3 of the multiplexer
 * - libgpiod-controlled GPIO lines for bit-banged I2C on SDA/SCL:
 *     * Change `GPIOCHIP_NAME`, `SDA_LINE`, and `SCL_LINE` to match your wiring.
 *
 * Compile with:
 *   g++ thermal_display.cpp -std=c++17 -lgpiod -lopencv_core -lopencv_imgproc -lopencv_highgui -o thermal_display
 */

#include <gpiod.h>
#include <opencv2/opencv.hpp>
#include <unistd.h>
#include <cstdint>
#include <vector>
#include <string>
#include <cstdio>

// Constants for sensors & multiplexer
static constexpr uint8_t TCA_ADDR    = 0x70;  // 7-bit address of TCA9546A
static constexpr uint8_t D6T_ADDR    = 0x0A;  // 7-bit address of D6T-32L
static constexpr uint8_t D6T_CMD     = 0x4D;
static constexpr int     N_ROW       = 32;
static constexpr int     N_PIXELS    = N_ROW*N_ROW;
static constexpr int     N_READ      = (N_PIXELS+1)*2 + 1;

// libgpiod bitbang parameters
static constexpr char    GPIOCHIP_NAME[] = "gpiochip0";
static constexpr unsigned int SDA_LINE = 2;   // BCM GPIO2 -> line offset
static constexpr unsigned int SCL_LINE = 3;   // BCM GPIO3 -> line offset
static constexpr useconds_t I2C_DELAY_USEC = 5;

class BitBangI2C {
public:
    BitBangI2C(const char *chipname, unsigned int sda_offset, unsigned int scl_offset) {
        chip = gpiod_chip_open_by_name(chipname);
        sda  = gpiod_chip_get_line(chip, sda_offset);
        scl  = gpiod_chip_get_line(chip, scl_offset);
        gpiod_line_request_output(sda, "i2c", 1);
        gpiod_line_request_output(scl, "i2c", 1);
    }

    ~BitBangI2C() {
        gpiod_line_release(sda);
        gpiod_line_release(scl);
        gpiod_chip_close(chip);
    }

    void start() {
        set_sda(1); set_scl(1); delay();
        set_sda(0); delay();
        set_scl(0); delay();
    }

    void stop() {
        set_sda(0); set_scl(1); delay();
        set_sda(1); delay();
    }

    bool write_byte(uint8_t data) {
        for(int i=0; i<8; ++i) {
            set_sda((data & 0x80) != 0);
            delay(); set_scl(1); delay(); set_scl(0); delay();
            data <<= 1;
        }
        gpiod_line_release(sda);
        gpiod_line_request_input(sda, "i2c_ack");
        delay(); set_scl(1); delay();
        int ack = gpiod_line_get_value(sda);
        set_scl(0); delay();
        gpiod_line_release(sda);
        gpiod_line_request_output(sda, "i2c", 1);
        return (ack == 0);
    }

    bool read_bytes(uint8_t *buf, int length) {
        for(int i=0; i<length; ++i) {
            uint8_t byte = 0;
            gpiod_line_release(sda);
            gpiod_line_request_input(sda, "i2c_read");
            for(int b=0; b<8; ++b) {
                set_scl(1); delay();
                byte = (byte << 1) | gpiod_line_get_value(sda);
                set_scl(0); delay();
            }
            buf[i] = byte;
            gpiod_line_release(sda);
            gpiod_line_request_output(sda, "i2c", (i<length-1)?0:1);
            delay(); set_scl(1); delay(); set_scl(0); delay();
        }
        return true;
    }

private:
    gpiod_chip *chip;
    gpiod_line *sda;
    gpiod_line *scl;

    void set_sda(int v) { gpiod_line_set_value(sda, v); }
    void set_scl(int v) { gpiod_line_set_value(scl, v); }
    void delay()      { usleep(I2C_DELAY_USEC); }
};

bool select_channel(BitBangI2C &bus, uint8_t ch) {
    bus.start();
    bool ok = bus.write_byte((TCA_ADDR<<1)|0);
    ok &= bus.write_byte(1 << ch);
    bus.stop();
    usleep(1000);
    return ok;
}

bool read_d6t(BitBangI2C &bus, std::vector<double> &pixels) {
    uint8_t rbuf[N_READ] = {0};
    bus.start();
    if (!bus.write_byte((D6T_ADDR<<1)|0)) { bus.stop(); return false; }
    if (!bus.write_byte(D6T_CMD)) { bus.stop(); return false; }
    bus.stop();
    usleep(1000);
    bus.start();
    if (!bus.write_byte((D6T_ADDR<<1)|1)) { bus.stop(); return false; }
    if (!bus.read_bytes(rbuf, N_READ)) { bus.stop(); return false; }
    bus.stop();
    for(int i=0; i<N_PIXELS; ++i) {
        int16_t raw = static_cast<int16_t>((uint16_t)rbuf[2+2*i] | ((uint16_t)rbuf[3+2*i]<<8));
        pixels[i] = raw / 10.0;
    }
    return true;
}

int main() {
    BitBangI2C bus(GPIOCHIP_NAME, SDA_LINE, SCL_LINE);
    std::vector<double> pixels(N_PIXELS);

    for(int ch=0; ch<4; ++ch) {
        if(!select_channel(bus, ch)) {
            fprintf(stderr, "Failed to select channel %d\n", ch);
            continue;
        }
        usleep(50000);

        if(!read_d6t(bus, pixels)) {
            fprintf(stderr, "Failed to read sensor on channel %d\n", ch);
            continue;
        }

        cv::Mat img(N_ROW, N_ROW, CV_32FC1, pixels.data());
        cv::Mat img8;
        img.convertTo(img8, CV_8UC1, 2.55);
        cv::Mat colored;
        cv::applyColorMap(img8, colored, cv::COLORMAP_INFERNO);

        std::string title = "Sensor Channel " + std::to_string(ch);
        cv::namedWindow(title, cv::WINDOW_AUTOSIZE);
        cv::imshow(title, colored);
        cv::waitKey(0);
        cv::destroyWindow(title);
    }
    return 0;
}
