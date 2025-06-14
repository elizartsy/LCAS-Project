#ifndef THERMAL_CAMERA_MANAGER_H
#define THERMAL_CAMERA_MANAGER_H

#include <opencv2/opencv.hpp>
#include <vector>

class ThermalCameraManager {
public:
    ThermalCameraManager(int numCameras = 4);
    ~ThermalCameraManager();

    void initialize();
    cv::Mat getThermalFrame(int camIndex);
    bool checkAndSaveIfThresholdExceeded(int camIndex, const cv::Mat& displayImage);
    
    static constexpr double TEMP_THRESHOLD = 40.0;
    static constexpr int N_ROW = 32;
    static constexpr int N_PIXEL = N_ROW * N_ROW;
    static constexpr int N_READ = (N_PIXEL + 1) * 2 + 1;

    static constexpr const char* I2C_DEV = "/dev/i2c-1";
    static constexpr const char* GPIO_CHIP = "/dev/gpiochip0";
    static constexpr int GPIO_LINE = 23;
    static constexpr int D6T_ADDR = 0x0A;
    static constexpr int D6T_CMD = 0x4D;
    static constexpr int MUX_ADDR = 0x70;

private:
    void resetMux();
    void selectMuxChannel(int channel);
    void initialSetting();
    cv::Mat fetchImage();

    double ptat;
    std::vector<double> pixelData;

    
};

#endif // THERMAL_CAMERA_MANAGER_H
