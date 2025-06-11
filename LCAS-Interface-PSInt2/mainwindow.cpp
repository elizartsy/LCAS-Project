#include "mainwindow.h"
#include <QPixmap>
#include <QImage>
#include <QTimer>
#include <QProcess>
#include <QLCDNumber>
#include <QDebug>
#include <QThread>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), updateTimer(new QTimer(this)) {
    ui->setupUi(this);
    thermalManager.initialize();

    connect(ui->doubleSpinBox_Vset, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        this, &MainWindow::handleVoltageChanged);
    connect(ui->doubleSpinBox_Iset, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        this, &MainWindow::handleCurrentChanged);
    connect(ui->PSoutputButton, &QPushButton::clicked,
        this, &MainWindow::handleToggleOutput);

    connect(ui->doubleSpinBox_Vset_2, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        this, &MainWindow::handleVoltageChanged2);
    connect(ui->doubleSpinBox_Iset_2, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        this, &MainWindow::handleCurrentChanged2);
    connect(ui->PSoutputButton_2, &QPushButton::clicked,
        this, &MainWindow::handleToggleOutput2);


    adcProcess = new QProcess(this);
    adcProcess->setProgram("python3");
    adcProcess->setArguments(QStringList() << "/home/admin/Desktop/LCAS-Interface-PSInt2/readadcsimple.py");
    adcProcess->setProcessChannelMode(QProcess::MergedChannels); // Merge stdout + stderr
    connect(adcProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::handleADCOutput);
    adcProcess->start();

    powerSerial = new QSerialPort(this);
    powerSerial->setPortName("/dev/ttyUSB0");  
    powerSerial->setBaudRate(QSerialPort::Baud9600);  
    powerSerial->setDataBits(QSerialPort::Data8);
    powerSerial->setParity(QSerialPort::NoParity);
    powerSerial->setStopBits(QSerialPort::OneStop);
    powerSerial->setFlowControl(QSerialPort::NoFlowControl);
    if (powerSerial->open(QIODevice::ReadWrite)) {
    qDebug() << "Power serial port opened successfully.";
    } else {
    qDebug() << "Failed to open power serial port:" << powerSerial->errorString();
    }


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
    updateTimer->start(50);  // ms
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
            double val[4] = {
                values[0].toDouble(&ok[0]),
                values[1].toDouble(&ok[1]),
                values[2].toDouble(&ok[2]),
                values[3].toDouble(&ok[3])
            };

            if (ok[0]) ui->lcdNumber->display(val[0]);
            if (ok[1]) ui->lcdNumber_2->display(val[1]);
            if (ok[2]) ui->lcdNumber_3->display(val[2]);
            if (ok[3]) ui->lcdNumber_4->display(val[3]);

            // Check thresholds for all 4 channels
            for (int i = 0; i < 4; ++i) {
                if (!ok[i]) continue;

                double threshold = 0.0;
                switch (i) {
                    case 0: threshold = ui->doubleSpinBox->value(); break;
                    case 1: threshold = ui->doubleSpinBox_2->value(); break;
                    case 2: threshold = ui->doubleSpinBox_3->value(); break;
                    case 3: threshold = ui->doubleSpinBox_4->value(); break;
                }

                if (val[i] > threshold && !powerShutdownTriggered) {
                    powerShutdownTriggered = true;

                    QByteArray shutdownCmd = "PC 0\n";  // Replace with your device's RS485 command
                    if (powerSerial && powerSerial->isOpen()) {
                        powerSerial->write(shutdownCmd);
                        qDebug() << QString("Channel %1 exceeded threshold (%2 > %3). Shutdown issued.")
                                    .arg(i).arg(val[i]).arg(threshold);
                    } else {
                        qDebug() << "Serial port not open — cannot send shutdown command.";
                    }

                    break;  // Exit loop after first triggering channel
                }
            }
        }
    }
}

void MainWindow::sendCommandToPowerSupply(const QString& address, const QString& command) {
    if (powerSerial && powerSerial->isOpen()) {
        QByteArray adrCmd = QString("ADR %1\r").arg(address).toUtf8();
        QByteArray mainCmd = command.toUtf8();

        powerSerial->write(adrCmd);
        powerSerial->waitForBytesWritten(50);
        QThread::msleep(100);

        powerSerial->write(mainCmd);
        powerSerial->waitForBytesWritten(50);

        qDebug() << "Sent to PS" << address << ":" << command.trimmed();
    } else {
        qDebug() << "Serial port not open — cannot send command to PS" << address;
    }
}


void MainWindow::handleVoltageChanged(double voltage) {
    sendCommandToPowerSupply("06", QString("PV %1\r").arg(voltage, 0, 'f', 2));
}

void MainWindow::handleCurrentChanged(double current) {
    sendCommandToPowerSupply("06", QString("PC %1\r").arg(current, 0, 'f', 2));
}

void MainWindow::handleToggleOutput() {
    static bool outputOn1 = false;
    outputOn1 = !outputOn1;
    sendCommandToPowerSupply("06", QString("OUT %1\r").arg(outputOn1 ? 1 : 0));

    QString style = outputOn1
        ? "background-color: green; border: 1px solid black;"
        : "background-color: red; border: 1px solid black;";
    ui->OutIndicatorFrame->setStyleSheet(style);
}

void MainWindow::handleVoltageChanged2(double voltage) {
    sendCommandToPowerSupply("07", QString("PV %1\r").arg(voltage, 0, 'f', 2));
}

void MainWindow::handleCurrentChanged2(double current) {
    sendCommandToPowerSupply("07", QString("PC %1\r").arg(current, 0, 'f', 2));
}

void MainWindow::handleToggleOutput2() {
    static bool outputOn2 = false;
    outputOn2 = !outputOn2;
    sendCommandToPowerSupply("07", QString("OUT %1\r").arg(outputOn2 ? 1 : 0));

    QString style = outputOn2
        ? "background-color: green; border: 1px solid black;"
        : "background-color: red; border: 1px solid black;";
    ui->OutIndicatorFrame_2->setStyleSheet(style);
}
