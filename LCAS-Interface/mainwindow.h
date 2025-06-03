#pragma once

#include <QMainWindow>
#include <QTimer>
#include "ui_mainwindow.h"
#include "ThermalCameraManager.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void updateFrames();

private:
    Ui::MainWindow ui;
    QTimer* updateTimer;
    ThermalCameraManager thermalManager;
};
