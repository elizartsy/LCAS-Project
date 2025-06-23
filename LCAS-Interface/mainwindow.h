#pragma once

#include <QMainWindow>
#include <QTimer>
#include <QLabel>
#include <QProcess>
#include <QSerialPort>
#include "LCASGUIV2.h"
#include "ThermalCameraManager.h"
#include "ThermalWorker.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:

    void handleThermalFrame(int camIndex, const cv::Mat& frame, bool thresholdExceeded);

    void handleVoltageChanged(double);
    void handleCurrentChanged(double);
    void handleToggleOutput();

    void handleVoltageChanged2(double);
    void handleCurrentChanged2(double);
    void handleToggleOutput2();

    void handleEmergencyStop();


private:
    Ui::MainWindow *ui;
    QTimer* updateTimer;
    ThermalCameraManager thermalManager;

    QThread* thermalThreads[4];
    ThermalWorker* thermalWorkers[4];

    QLabel* label_cam0;
    QLabel* label_cam1;
    QLabel* label_cam2;
    QLabel* label_cam3;

    QProcess* adcProcess;           // Process to run the ADC Python script
    void handleADCOutput();         // Slot to handle new ADC data

    QSerialPort* powerSerial;
    bool powerShutdownTriggered = false;

    void sendCommandToPowerSupply(const QString& address, const QString& command);


};
