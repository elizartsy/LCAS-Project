#include "mainwindow.h"
#include <QPixmap>
#include <QImage>
#include <QTimer>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), updateTimer(new QTimer(this)) {
    ui->setupUi(this);
    thermalManager.initialize();

    label_cam0 = new QLabel(ui->frame);
    label_cam1 = new QLabel(ui->frame_2);
    label_cam2 = new QLabel(ui->frame_3);
    label_cam3 = new QLabel(ui->frame_4);
    

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
        QPixmap pixmap = QPixmap::fromImage(image).scaled(label_cam0->size(), Qt::KeepAspectRatio);

        switch (cam) {
            case 0: label_cam0->setPixmap(pixmap); break;
            case 1: label_cam1->setPixmap(pixmap); break;
            case 2: label_cam2->setPixmap(pixmap); break;
            case 3: label_cam3->setPixmap(pixmap); break;
        }
    }
}
