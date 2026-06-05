#include "simdevicestatemachine.h"
#include <algorithm>
#include <QDateTime>

SimDeviceStateMachine::SimDeviceStateMachine(SimRegisterManager* regMgr, QObject* parent)
    : QObject(parent)
    , m_regMgr(regMgr)
    , m_state(Idle)
    , m_currentReading(0)
    , m_temperatureReading(0)
    , m_powerReading(0)
    , m_noiseCurrent(0)
    , m_noiseTemp(0)
    , m_noisePower(0)
    , m_simulationTimer(nullptr)
    , m_startTimestamp(0)
    , m_currentSlewRate(0.1)
    , m_noiseAmplitude(0.02)
    , m_startupDelayMs(2000)
    , m_tempResponseLag(0.05)
{
}

void SimDeviceStateMachine::startSimulation()
{
    if (!m_simulationTimer) {
        m_simulationTimer = new QTimer(this);
        connect(m_simulationTimer, &QTimer::timeout, this, &SimDeviceStateMachine::updateSimulation);
    }
    m_simulationTimer->start(100);
    if (m_state == Idle) {
        m_state = Idle;
        writeStatusRegister(Idle);
    }
}

void SimDeviceStateMachine::stopSimulation()
{
    if (m_simulationTimer) {
        m_simulationTimer->stop();
    }
}

void SimDeviceStateMachine::start()
{
    if (m_state == Idle) {
        m_state = Starting;
        m_startTimestamp = QDateTime::currentMSecsSinceEpoch();
        writeStatusRegister(Starting);
        emit logMessage(QStringLiteral("Device state: Starting"));
    }
}

void SimDeviceStateMachine::stop()
{
    if (m_state == Running || m_state == Starting) {
        m_state = Stopping;
        m_startTimestamp = QDateTime::currentMSecsSinceEpoch();
        writeStatusRegister(Stopping);
        emit logMessage(QStringLiteral("Device state: Stopping"));
    }
}

void SimDeviceStateMachine::reset()
{
    m_state = Idle;
    m_currentReading = 0;
    m_temperatureReading = 0;
    m_powerReading = 0;
    writeStatusRegister(Idle);
    emit logMessage(QStringLiteral("Device state: Idle (reset)"));
}

void SimDeviceStateMachine::updateSimulation()
{
    if (m_state == Starting) {
        qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - m_startTimestamp;
        if (elapsed >= m_startupDelayMs) {
            m_state = Running;
            writeStatusRegister(Running);
            emit logMessage(QStringLiteral("Device state: Running"));
        }
        return;
    }

    if (m_state == Stopping) {
        qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - m_startTimestamp;
        if (elapsed >= 1500) {
            m_state = Idle;
            m_currentReading = 0;
            m_temperatureReading = 0;
            m_powerReading = 0;
            writeStatusRegister(Idle);
            emit logMessage(QStringLiteral("Device state: Idle"));
        }
        return;
    }

    if (m_state != Running) {
        return;
    }

    quint32 curSet = m_regMgr->readRaw(0x02, 0x00);
    quint32 tempSet = m_regMgr->readRaw(0x03, 0x00);

    double targetCurrent = curSet / 100.0;
    double targetTemp = tempSet / 1000.0;

    double currentNoise = m_rng.bounded(50) / 100.0;
    double tempNoise = m_rng.bounded(100) / 1000.0;

    double currentDelta = targetCurrent - m_currentReading;
    double currentSlew = currentDelta * m_currentSlewRate;
    m_currentReading += currentSlew;

    double tempDelta = targetTemp - m_temperatureReading;
    double tempSlew = tempDelta * m_tempResponseLag;
    m_temperatureReading += tempSlew;

    m_currentReading = std::max(0.0, m_currentReading + currentNoise * 0.01);
    m_temperatureReading = std::max(20.0, m_temperatureReading + tempNoise * 0.01);
    m_powerReading = m_currentReading * m_temperatureReading * 0.01;

    double curNoiseMag = m_currentReading * m_noiseAmplitude;
    double tempNoiseMag = m_temperatureReading * m_noiseAmplitude * 0.25;
    double pwrNoiseMag = m_powerReading * m_noiseAmplitude * 1.5;

    int curNBins = std::max(1, static_cast<int>(curNoiseMag * 100));
    int tempNBins = std::max(1, static_cast<int>(tempNoiseMag * 100));
    int pwrNBins = std::max(1, static_cast<int>(pwrNoiseMag * 100));
    m_noiseCurrent = m_rng.bounded(curNBins) / 100.0;
    m_noiseTemp = m_rng.bounded(tempNBins) / 100.0;
    m_noisePower = m_rng.bounded(pwrNBins) / 100.0;

    quint32 curReading = static_cast<quint32>((m_currentReading + m_noiseCurrent) * 100);
    quint32 tempReading = static_cast<quint32>((m_temperatureReading + m_noiseTemp) * 1000);
    quint32 powerReading = static_cast<quint32>((m_powerReading + m_noisePower) * 100);

    m_regMgr->writeRaw(0x00, 0x00, Running);
    m_regMgr->writeRaw(0x04, 0x00, powerReading);

    if (curSet > 0) {
        m_regMgr->writeRaw(0x02, 0x01, curReading);
    }
    if (tempSet > 0) {
        m_regMgr->writeRaw(0x03, 0x01, tempReading);
    }

    if (m_currentReading > targetCurrent * 1.1 && targetCurrent > 0) {
        m_state = Error;
        m_regMgr->writeRaw(0x00, 0x00, Error);
        m_regMgr->writeRaw(0x01, 0x00, 0x00000001);
        writeStatusRegister(Error);
        emit logMessage(QStringLiteral("Device state: Error (Over-current)"));
    }

    emit dataUpdated(m_currentReading + m_noiseCurrent,
                     m_temperatureReading + m_noiseTemp,
                     m_powerReading + m_noisePower);
}

void SimDeviceStateMachine::writeStatusRegister(DeviceState state)
{
    quint32 statusVal = 0;
    switch (state) {
        case Idle:     statusVal = 0x00000001; break;
        case Starting: statusVal = 0x00000002; break;
        case Running:  statusVal = 0x00000004; break;
        case Stopping: statusVal = 0x00000008; break;
        case Error:    statusVal = 0x00000010; break;
    }
    m_regMgr->writeRaw(0x00, 0x00, statusVal);
}

void SimDeviceStateMachine::setCurrentSlewRate(double v)
{
    if (v < 0.01) v = 0.01;
    if (v > 1.0) v = 1.0;
    if (m_currentSlewRate != v) {
        m_currentSlewRate = v;
        emit currentSlewRateChanged(v);
    }
}

void SimDeviceStateMachine::setNoiseAmplitude(double v)
{
    if (v < 0) v = 0;
    if (v > 0.2) v = 0.2;
    if (m_noiseAmplitude != v) {
        m_noiseAmplitude = v;
        emit noiseAmplitudeChanged(v);
    }
}

void SimDeviceStateMachine::setStartupDelayMs(int ms)
{
    if (ms < 100) ms = 100;
    if (ms > 10000) ms = 10000;
    if (m_startupDelayMs != ms) {
        m_startupDelayMs = ms;
        emit startupDelayMsChanged(ms);
    }
}

void SimDeviceStateMachine::setTempResponseLag(double v)
{
    if (v < 0.01) v = 0.01;
    if (v > 0.5) v = 0.5;
    if (m_tempResponseLag != v) {
        m_tempResponseLag = v;
        emit tempResponseLagChanged(v);
    }
}
