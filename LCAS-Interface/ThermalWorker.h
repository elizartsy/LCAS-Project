#include <QObject>
#include <QTimer>
#include <opencv2/opencv.hpp>

class ThermalWorker : public QObject {
    Q_OBJECT
public:
    explicit ThermalWorker(int cameraIndex, QObject *parent = nullptr);

public slots:
    void start();  // start capturing
    void stop();   // stop capturing

signals:
    void frameReady(int camIndex, const cv::Mat& frame, bool thresholdExceeded);

private slots:
    void process();

private:
    int camIndex;
    QTimer* captureTimer;
};
