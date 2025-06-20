#include "ThermalWorker.h"
#include "ThermalCameraManager.h"
#include <QThread>
#include <QDebug>

extern ThermalCameraManager thermalManager;

ThermalWorker::ThermalWorker(int cameraIndex, QObject *parent)
    : QObject(parent), camIndex(cameraIndex), captureTimer(nullptr) {}

void ThermalWorker::start() {
    qDebug() << "ThermalWorker::start() called for cam" << camIndex;

    captureTimer = new QTimer(this);  // now created in correct thread
    captureTimer->setInterval(50);
    connect(captureTimer, &QTimer::timeout, this, &ThermalWorker::process);
    captureTimer->start();

    qDebug() << "Timer started in thread:" << QThread::currentThread();
}

void ThermalWorker::stop() {
    if (captureTimer) {
        captureTimer->stop();
        captureTimer->deleteLater();
        captureTimer = nullptr;
    }
}

void ThermalWorker::process() {
    cv::Mat frame = thermalManager.getThermalFrame(camIndex);
    if (frame.empty()) {
        qDebug() << "Camera" << camIndex << "returned empty frame.";
        return;
    }
    bool triggered = thermalManager.checkAndSaveIfThresholdExceeded(camIndex, frame);
    qDebug() << "ThermalWorker emitting frameReady for cam" << camIndex << ", triggered =" << triggered;
    emit frameReady(camIndex, frame.clone(), triggered);  // safer with clone
}
