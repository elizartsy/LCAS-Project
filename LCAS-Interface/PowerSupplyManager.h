#ifndef POWER_SUPPLY_MANAGER_H
#define POWER_SUPPLY_MANAGER_H

#include <QString>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QObject>
#include <QTimer>

class PowerSupplyManager : public QObject {
    Q_OBJECT

public:
    explicit PowerSupplyManager(QObject* parent = nullptr);
    ~PowerSupplyManager();

    bool connectToPort(const QString& portName, int baudRate = 9600);
    void disconnect();

    bool setAddress(int address);
    bool reset();
    bool setRemoteMode();

    bool setVoltage(double volts);
    bool setCurrent(double amps);
    bool enableOutput(bool on);

    QString queryVoltage();   // Actual output voltage
    QString querySetVoltage(); // Programmed voltage
    QString queryCurrent();   // Actual output current
    QString querySetCurrent(); // Programmed current

signals:
    void responseReceived(const QString& response);
    void errorOccurred(const QString& error);

private:
    QSerialPort* serial;
    int currentAddress = 0;

    bool sendCommand(const QString& cmd, bool expectResponse = false);
    QString readResponse();

    QString makeCommand(const QString& cmd) const;
};

#endif // POWER_SUPPLY_MANAGER_H
