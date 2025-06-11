#include "PowerSupplyManager.h"
#include <QThread>

PowerSupplyManager::PowerSupplyManager(QObject* parent)
    : QObject(parent), serial(new QSerialPort(this)) {
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setDataBits(QSerialPort::Data8);
}

PowerSupplyManager::~PowerSupplyManager() {
    disconnect();
}

bool PowerSupplyManager::connectToPort(const QString& portName, int baudRate) {
    serial->setPortName(portName);
    serial->setBaudRate(baudRate);
    if (!serial->open(QIODevice::ReadWrite)) {
        emit errorOccurred("Failed to open port");
        return false;
    }
    return true;
}

void PowerSupplyManager::disconnect() {
    if (serial->isOpen()) {
        serial->close();
    }
}

QString PowerSupplyManager::makeCommand(const QString& cmd) const {
    return cmd + "\r"; // Commands must end with CR (ASCII 13)
}

bool PowerSupplyManager::sendCommand(const QString& cmd, bool expectResponse) {
    QString fullCmd = makeCommand(cmd);
    serial->write(fullCmd.toUtf8());
    if (!serial->waitForBytesWritten(100)) {
        emit errorOccurred("Write timeout");
        return false;
    }

    if (expectResponse) {
        return serial->waitForReadyRead(200);
    }

    // Allow time between commands per spec
    QThread::msleep(100);
    return true;
}

QString PowerSupplyManager::readResponse() {
    if (!serial->waitForReadyRead(500))
        return "";

    QByteArray response = serial->readAll();
    while (serial->waitForReadyRead(100)) {
        response += serial->readAll();
    }
    return QString(response).trimmed();
}

bool PowerSupplyManager::setAddress(int address) {
    currentAddress = address;
    return sendCommand(QString("ADR %1").arg(address), true) &&
           readResponse().contains("OK");
}

bool PowerSupplyManager::reset() {
    return sendCommand("RST", true) && readResponse().contains("OK");
}

bool PowerSupplyManager::setRemoteMode() {
    return sendCommand("RMT 1", true) && readResponse().contains("OK");
}

bool PowerSupplyManager::setVoltage(double volts) {
    return sendCommand(QString("PV %1").arg(volts), true) && readResponse().contains("OK");
}

bool PowerSupplyManager::setCurrent(double amps) {
    return sendCommand(QString("PC %1").arg(amps), true) && readResponse().contains("OK");
}

bool PowerSupplyManager::enableOutput(bool on) {
    return sendCommand(QString("OUT %1").arg(on ? 1 : 0), true) && readResponse().contains("OK");
}

QString PowerSupplyManager::queryVoltage() {
    sendCommand("MV?", true);
    return readResponse();
}

QString PowerSupplyManager::querySetVoltage() {
    sendCommand("PV?", true);
    return readResponse();
}

QString PowerSupplyManager::queryCurrent() {
    sendCommand("MC?", true);
    return readResponse();
}

QString PowerSupplyManager::querySetCurrent() {
    sendCommand("PC?", true);
    return readResponse();
}
