#include "mainwindow.h"
#include <QPixmap>
#include <QImage>
#include <QTimer>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), updateTimer(new QTimer(this)) {
    ui.setupUi(this);
    thermalManager.initialize();

    // Optional: show label placeholders
    ui.label_1->setText("Camera 0");
    ui.label_2->setText("Camera 1");
    ui.label_3->setText("Camera 2");
    ui.label_4->setText("Camera 3");

    // Timer to update frames
    connect(updateTimer, &QTimer::timeout, this, &MainWindow::updateFrames);
    updateTimer->start(1000);  // ms
}

MainWindow::~MainWindow() = default;

static QImage matToQImage(const cv::Mat& mat) {
    cv::Mat rgb;
    if (mat.channels() == 1) {
        cv::cvtColor(mat, rgb, cv::COLOR_GRAY2RGB);
    } else {
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
    }
    return QImage(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888).copy();
}

void MainWindow::updateFrames() {
    for (int cam = 0; cam < 4; ++cam) {
        cv::Mat frame = thermalManager.getThermalFrame(cam);
        thermalManager.checkAndSaveIfThresholdExceeded(cam, frame);

        QImage image = matToQImage(frame);
        QPixmap pixmap = QPixmap::fromImage(image).scaled(ui.label_1->size(), Qt::KeepAspectRatio);

        switch (cam) {
            case 0: ui.label_1->setPixmap(pixmap); break;
            case 1: ui.label_2->setPixmap(pixmap); break;
            case 2: ui.label_3->setPixmap(pixmap); break;
            case 3: ui.label_4->setPixmap(pixmap); break;
        }
    }
}
