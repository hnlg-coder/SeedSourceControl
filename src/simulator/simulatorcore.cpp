#include "simulatorcore.h"

SimulatorCore::SimulatorCore(QObject* parent)
    : QObject(parent)
    , m_regMgr(nullptr)
    , m_stateMachine(nullptr)
    , m_protocolHandler(nullptr)
    , m_faultInjector(nullptr)
    , m_currentReading(0)
    , m_temperatureReading(0)
    , m_powerReading(0)
    , m_simulating(false)
{
}

SimulatorCore::~SimulatorCore()
{
    stopSimulation();
}

void SimulatorCore::initialize()
{
    m_regMgr = new SimRegisterManager(this);
    m_regMgr->initialize();

    m_stateMachine = new SimDeviceStateMachine(m_regMgr, this);
    connect(m_stateMachine, &SimDeviceStateMachine::dataUpdated,
            this, &SimulatorCore::onStateDataUpdated);
    connect(m_stateMachine, &SimDeviceStateMachine::logMessage,
            this, &SimulatorCore::logMessage);

    m_protocolHandler = new SimProtocolHandler(m_regMgr, m_stateMachine, this);
    connect(m_protocolHandler, &SimProtocolHandler::logMessage,
            this, &SimulatorCore::logMessage);

    m_faultInjector = new FaultInjector(this);
    connect(m_faultInjector, &FaultInjector::logMessage,
            this, &SimulatorCore::logMessage);
    connect(m_faultInjector, &FaultInjector::faultInjected,
            this, &SimulatorCore::faultInjected);

    m_simulating = false;
    m_currentReading = 0;
    m_temperatureReading = 0;
    m_powerReading = 0;
}

void SimulatorCore::reset()
{
    stopSimulation();
    if (m_regMgr) {
        m_regMgr->reset();
    }
    if (m_stateMachine) {
        m_stateMachine->reset();
    }
    m_currentReading = 0;
    m_temperatureReading = 0;
    m_powerReading = 0;
}

QByteArray SimulatorCore::handleFrame(const QByteArray& request)
{
    if (!m_protocolHandler) {
        return QByteArray();
    }
    QByteArray response = m_protocolHandler->handleFrame(request);
    if (m_faultInjector) {
        response = m_faultInjector->process(response);
    }
    return response;
}

void SimulatorCore::startSimulation()
{
    if (m_simulating) return;
    m_simulating = true;
    if (m_stateMachine) {
        m_stateMachine->startSimulation();
    }
}

void SimulatorCore::stopSimulation()
{
    if (!m_simulating) return;
    m_simulating = false;
    if (m_stateMachine) {
        m_stateMachine->stopSimulation();
    }
}

quint32 SimulatorCore::statusValue() const
{
    if (!m_regMgr) return 0;
    return m_regMgr->readRaw(0x00, 0x00);
}

quint32 SimulatorCore::alarmValue() const
{
    if (!m_regMgr) return 0;
    return m_regMgr->readRaw(0x01, 0x00);
}

void SimulatorCore::onStateDataUpdated(double current, double temperature, double power)
{
    m_currentReading = current;
    m_temperatureReading = temperature;
    m_powerReading = power;
    emit dataUpdated(current, temperature, power);
}

quint32 SimulatorCore::readRawRegister(quint8 baseAddr, quint8 offset) const
{
    if (!m_regMgr) return 0;
    return m_regMgr->readRaw(baseAddr, offset);
}

void SimulatorCore::writeRawRegister(quint8 baseAddr, quint8 offset, quint32 value)
{
    if (m_regMgr) {
        m_regMgr->writeRaw(baseAddr, offset, value);
    }
}
