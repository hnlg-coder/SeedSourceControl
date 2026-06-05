#include "simulationworker.h"
#include "communication/command.h"
#include <QDebug>

SimulationWorker::SimulationWorker(QObject* parent)
    : QObject(parent)
    , m_simulatorCore(nullptr)
    , m_parser(nullptr)
    , m_running(false)
    , m_connected(false)
{
}

SimulationWorker::~SimulationWorker()
{
    stopCommunication();
    if (m_simulatorCore) {
        delete m_simulatorCore;
        m_simulatorCore = nullptr;
    }
}

void SimulationWorker::setProtocolParser(IProtocolParser* parser)
{
    m_parser = parser;
}

void SimulationWorker::startCommunication()
{
    m_running = true;
    emit logMessage(QStringLiteral("Simulation communication started"));
}

void SimulationWorker::stopCommunication()
{
    m_running = false;
    m_connected = false;
    if (m_simulatorCore) {
        m_simulatorCore->stopSimulation();
    }
}

void SimulationWorker::connectDevice()
{
    if (!m_simulatorCore) {
        m_simulatorCore = new SimulatorCore(this);
        m_simulatorCore->initialize();
        connect(m_simulatorCore, &SimulatorCore::logMessage,
                this, &SimulationWorker::logMessage);
    }

    m_simulatorCore->reset();
    m_simulatorCore->startSimulation();
    m_connected = true;

    emit connectionStateChanged(true);
    emit logMessage(QStringLiteral("Simulation mode: device connected"));
}

void SimulationWorker::disconnectDevice()
{
    m_connected = false;

    if (m_simulatorCore) {
        m_simulatorCore->stopSimulation();
    }

    emit connectionStateChanged(false);
    emit logMessage(QStringLiteral("Simulation mode: device disconnected"));
}

void SimulationWorker::sendCommand(QSharedPointer<ICommand> command)
{
    if (!m_running || !m_connected || !command) {
        return;
    }

    processCommand(command);
}

void SimulationWorker::processCommand(QSharedPointer<ICommand> command)
{
    if (!m_parser || !m_simulatorCore) {
        return;
    }

    QByteArray requestFrame = command->buildRequest();
    if (requestFrame.isEmpty()) {
        emit logMessage(QStringLiteral("Sim: empty request frame"));
        emit commandCompleted(command->id(), false, QVariant());
        return;
    }

    QByteArray responseFrame = m_simulatorCore->handleFrame(requestFrame);
    if (responseFrame.isEmpty()) {
        emit logMessage(QStringLiteral("Sim: empty response frame"));
        emit commandCompleted(command->id(), false, QVariant());
        return;
    }

    command->onResponse(responseFrame);

    if (command->state() == ICommand::Completed) {
        emit commandCompleted(command->id(), true, command->result());
    } else {
        emit commandCompleted(command->id(), false, QVariant());
    }
}

void SimulationWorker::onCommandFinished(quint32 cmdId, bool success, QVariant result)
{
    Q_UNUSED(cmdId);
    Q_UNUSED(success);
    Q_UNUSED(result);
}
