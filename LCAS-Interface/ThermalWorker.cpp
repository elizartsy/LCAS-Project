// thermalworker.cpp
#include "thermalworker.h"
#include "thermalmanager.h"  // Assume you already have this class
#include <QThread>
#include <ctime>
#include <sys/stat.h>
#include <opencv2/imgcodecs.hpp>

extern ThermalCameraManager thermalManager;  // Assume global/shared instance

ThermalWorker::ThermalWorker(int cameraIndex) : camIndex(cameraIndex) {}

void ThermalWorker::process() {
    cv::Mat frame = thermalManager.getThermalFrame(camIndex);
    bool triggered = thermalManager.checkAndSaveIfThresholdExceeded(camIndex, frame);
    emit frameReady(camIndex, frame, triggered);
}
