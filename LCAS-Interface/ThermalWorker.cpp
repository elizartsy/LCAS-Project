#include "thermalworker.h"
#include "ThermalCameraManager.h"
#include <QThread>

extern ThermalCameraManager thermalManager;

ThermalWorker::ThermalWorker(int cameraIndex, QObject *parent)
    : QObject(parent), camIndex(cameraIndex) {
    captureTimer = new QTimer(this);
    captureTimer->setInterval(50);  // 20 FPS approx

    connect(captureTimer, &QTimer::timeout, this, &ThermalWorker::process);
}

void ThermalWorker::start() {
    captureTimer->start();
}

void ThermalWorker::stop() {
    captureTimer->stop();
}

void ThermalWorker::process() {
    cv::Mat frame = thermalManager.getThermalFrame(camIndex);
    bool triggered = thermalManager.checkAndSaveIfThresholdExceeded(camIndex, frame);
    qDebug() << "ThermalWorker emitting frameReady for cam" << camIndex << ", triggered =" << triggered;
    emit frameReady(camIndex, frame, triggered);

}
