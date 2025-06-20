#include <QApplication>
#include <QMetaType>
#include "mainwindow.h"
#include "ThermalCameraManager.h"

ThermalCameraManager thermalManager;

int main(int argc, char *argv[]) {
    qRegisterMetaType<cv::Mat>("cv::Mat");
    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return app.exec();
}
