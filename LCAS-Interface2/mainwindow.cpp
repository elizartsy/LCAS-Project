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

    adcProcess = new QProcess(this);
    adcProcess->setProgram("python3");
    adcProcess->setArguments(QStringList() << "/home/pi/readadcsimple.py");
    adcProcess->setProcessChannelMode(QProcess::MergedChannels); // Merge stdout + stderr
    connect(adcProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::handleADCOutput);
    adcProcess->start();

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

void MainWindow::handleADCOutput() {
    while (adcProcess->canReadLine()) {
        QByteArray line = adcProcess->readLine().trimmed();
        QList<QByteArray> values = line.split(',');

        if (values.size() >= 4) {
            bool ok[4];
            double val0 = values[0].toDouble(&ok[0]);
            double val1 = values[1].toDouble(&ok[1]);
            double val2 = values[2].toDouble(&ok[2]);
            double val3 = values[3].toDouble(&ok[3]);

            if (ok[0]) ui->lcdNumber->display(val0);
            if (ok[1]) ui->lcdNumber_1->display(val1);
            if (ok[2]) ui->lcdNumber_2->display(val2);
            if (ok[3]) ui->lcdNumber_3->display(val3);
        }
    }
}




