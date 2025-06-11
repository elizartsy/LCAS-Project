#pragma once

#include <QMainWindow>
#include <QTimer>
#include <QLabel>
#include <QProcess>
#include <QSerialPort>
#include "LCASGUIV2.h"
#include "ThermalCameraManager.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void updateFrames();

private:
    Ui::MainWindow *ui;
    QTimer* updateTimer;
    ThermalCameraManager thermalManager;
    
    QLabel* label_cam0;
    QLabel* label_cam1;
    QLabel* label_cam2;
    QLabel* label_cam3;

    QProcess* adcProcess;           // Process to run the ADC Python script
    void handleADCOutput();         // Slot to handle new ADC data

    QSerialPort* powerSerial;
    bool powerShutdownTriggered = false;


};
