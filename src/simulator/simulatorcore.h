#ifndef SIMULATORCORE_H
#define SIMULATORCORE_H

#include <QObject>
#include <QByteArray>
#include "simregistermanager.h"
#include "simdevicestatemachine.h"
#include "simprotocolhandler.h"
#include "faultinjector.h"

class SimulatorCore : public QObject {
    Q_OBJECT
public:
    explicit SimulatorCore(QObject* parent = nullptr);
    ~SimulatorCore();

    void initialize();
    void reset();

    QByteArray handleFrame(const QByteArray& request);

    void startSimulation();
    void stopSimulation();

    bool isSimulating() const { return m_simulating; }

    double currentReading() const { return m_currentReading; }
    double temperatureReading() const { return m_temperatureReading; }
    double powerReading() const { return m_powerReading; }
    quint32 statusValue() const;
    quint32 alarmValue() const;

    SimDeviceStateMachine* stateMachine() const { return m_stateMachine; }
    FaultInjector* faultInjector() const { return m_faultInjector; }

    quint32 readRawRegister(quint8 baseAddr, quint8 offset) const;
    void writeRawRegister(quint8 baseAddr, quint8 offset, quint32 value);

signals:
    void dataUpdated(double current, double temperature, double power);
    void logMessage(const QString& message);
    void faultInjected(const QString& description);

private slots:
    void onStateDataUpdated(double current, double temperature, double power);

private:
    SimRegisterManager* m_regMgr;
    SimDeviceStateMachine* m_stateMachine;
    SimProtocolHandler* m_protocolHandler;
    FaultInjector* m_faultInjector;

    double m_currentReading;
    double m_temperatureReading;
    double m_powerReading;
    bool m_simulating;
};

#endif // SIMULATORCORE_H
