// thermalworker.h
#ifndef THERMALWORKER_H
#define THERMALWORKER_H

#include <QObject>
#include <opencv2/core.hpp>

class ThermalWorker : public QObject {
    Q_OBJECT

public:
    ThermalWorker(int cameraIndex);

signals:
    void frameReady(int camIndex, const cv::Mat& frame, bool thresholdExceeded);

public slots:
    void process();

private:
    int camIndex;
};

#endif // THERMALWORKER_H
