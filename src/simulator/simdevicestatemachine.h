#ifndef SIMDEVICESTATEMACHINE_H
#define SIMDEVICESTATEMACHINE_H

#include <QObject>
#include <QTimer>
#include <QRandomGenerator>
#include "simregistermanager.h"

class SimDeviceStateMachine : public QObject {
    Q_OBJECT
    Q_PROPERTY(double currentSlewRate READ currentSlewRate WRITE setCurrentSlewRate NOTIFY currentSlewRateChanged)
    Q_PROPERTY(double noiseAmplitude READ noiseAmplitude WRITE setNoiseAmplitude NOTIFY noiseAmplitudeChanged)
    Q_PROPERTY(int startupDelayMs READ startupDelayMs WRITE setStartupDelayMs NOTIFY startupDelayMsChanged)
    Q_PROPERTY(double tempResponseLag READ tempResponseLag WRITE setTempResponseLag NOTIFY tempResponseLagChanged)
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

    void startSimulation();
    void stopSimulation();

    double currentSlewRate() const { return m_currentSlewRate; }
    void setCurrentSlewRate(double v);

    double noiseAmplitude() const { return m_noiseAmplitude; }
    void setNoiseAmplitude(double v);

    int startupDelayMs() const { return m_startupDelayMs; }
    void setStartupDelayMs(int ms);

    double tempResponseLag() const { return m_tempResponseLag; }
    void setTempResponseLag(double v);

signals:
    void stateChanged(SimDeviceStateMachine::DeviceState newState);
    void dataUpdated(double current, double temperature, double power);
    void logMessage(const QString& message);
    void currentSlewRateChanged(double);
    void noiseAmplitudeChanged(double);
    void startupDelayMsChanged(int);
    void tempResponseLagChanged(double);

private slots:
    void updateSimulation();

private:
    void writeStatusRegister(DeviceState state);

    SimRegisterManager* m_regMgr;
    DeviceState m_state;

    double m_currentReading;
    double m_temperatureReading;
    double m_powerReading;

    double m_noiseCurrent;
    double m_noiseTemp;
    double m_noisePower;

    QTimer* m_simulationTimer;
    QRandomGenerator m_rng;
    qint64 m_startTimestamp;

    double m_currentSlewRate;
    double m_noiseAmplitude;
    int m_startupDelayMs;
    double m_tempResponseLag;
};

#endif // SIMDEVICESTATEMACHINE_H
