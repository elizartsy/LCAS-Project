#include <QApplication>
#include "mainwindow.h"
#include "ThermalCameraManager.h"

ThermalCameraManager thermalManager;

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return app.exec();
}
