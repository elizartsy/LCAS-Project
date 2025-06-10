#include "mainwindow.h"
#include <QPixmap>
#include <QImage>
#include <QTimer>
#include <QProcess>
#include <QLCDNumber>
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
    
    for (QLabel* label : {label_cam0, label_cam1, label_cam2, label_cam3}) {
        label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        label->setAlignment(Qt::AlignCenter);
        label->setMinimumSize(200, 200);  
        label->setScaledContents(false);  
    }
        
    // Timer to update frames
    connect(updateTimer, &QTimer::timeout, this, &MainWindow::updateFrames);
    connect(updateTimer, &QTimer::timeout, this, &MainWindow::updateADCReading);
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

        QLabel* targetLabel = nullptr;
        switch (cam) {
            case 0: targetLabel = label_cam0; break;
            case 1: targetLabel = label_cam1; break;
            case 2: targetLabel = label_cam2; break;
            case 3: targetLabel = label_cam3; break;
        }

        if (targetLabel) {
            QPixmap pixmap = QPixmap::fromImage(image).scaled(
                targetLabel->size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation
            );
            targetLabel->setPixmap(pixmap);
        }
    }
}

void MainWindow::updateADCReading() {
    QProcess process;
    process.start("python3", QStringList() << "/home/pi/read_adc_once.py");

    if (!process.waitForStarted(1000)) {
        qWarning("Failed to start ADC script");
        return;
    }

    if (!process.waitForFinished(2000)) {
        qWarning("ADC script timeout");
        return;
    }

    QString output = process.readAllStandardOutput().trimmed();
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    .
    if (lines.size() >= 4) {
        ui->lcdADC0->display(lines[0].toDouble());
        ui->lcdADC1->display(lines[1].toDouble());
        ui->lcdADC2->display(lines[2].toDouble());
        ui->lcdADC3->display(lines[3].toDouble());
    } else {
        qWarning("Unexpected number of ADC channels received: %d", lines.size());
    }
}



