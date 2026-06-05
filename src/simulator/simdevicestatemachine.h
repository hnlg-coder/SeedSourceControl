#ifndef SIMDEVICESTATEMACHINE_H
#define SIMDEVICESTATEMACHINE_H

#include <QObject>
#include <QTimer>
#include <QRandomGenerator>
#include <QElapsedTimer>
#include "simregistermanager.h"

class SimDeviceStateMachine : public QObject {
    Q_OBJECT
public:
    enum DeviceState {
        Idle = 1,
        Starting,
        Running,
        Stopping,
        Error
    };
    Q_ENUM(DeviceState)

    explicit SimDeviceStateMachine(SimRegisterManager* regMgr, QObject* parent = nullptr);

    void start();
    void stop();
    void reset();

    DeviceState currentState() const { return m_state; }
    bool isRunning() const { return m_state == Running; }
    void setTargetCurrent(double current) { m_targetCurrent = current; }

    void startSimulation();
    void stopSimulation();

signals:
    void stateChanged(SimDeviceStateMachine::DeviceState newState);
    void dataUpdated(double current, double temperature, double power);
    void logMessage(const QString& message);

private slots:
    void updateSimulation();

private:
    void writeStatusRegister(DeviceState state);

    SimRegisterManager* m_regMgr;
    DeviceState m_state;

    double m_targetCurrent;
    double m_currentReading;
    double m_temperatureReading;
    double m_powerReading;

    double m_noiseCurrent;
    double m_noiseTemp;
    double m_noisePower;

    QTimer* m_simulationTimer;
    QRandomGenerator m_rng;
    qint64 m_startTimestamp;
};

#endif // SIMDEVICESTATEMACHINE_H
