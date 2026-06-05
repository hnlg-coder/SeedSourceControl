#ifndef SIMULATIONWORKER_H
#define SIMULATIONWORKER_H

#include <QObject>
#include <QAtomicInt>
#include <QSharedPointer>
#include "simulatorcore.h"

class IProtocolParser;
class ICommand;

class SimulationWorker : public QObject {
    Q_OBJECT
public:
    explicit SimulationWorker(QObject* parent = nullptr);
    ~SimulationWorker();

    void setProtocolParser(IProtocolParser* parser);
    bool isConnected() const { return m_connected; }

signals:
    void dataReceived(const QByteArray& data);
    void commandCompleted(quint32 cmdId, bool success, QVariant result);
    void connectionStateChanged(bool connected);
    void logMessage(const QString& message);

public slots:
    void startCommunication();
    void stopCommunication();
    void connectDevice();
    void disconnectDevice();
    void sendCommand(QSharedPointer<ICommand> command);

private:
    void processCommand(QSharedPointer<ICommand> command);
    void onCommandFinished(quint32 cmdId, bool success, QVariant result);

    SimulatorCore* m_simulatorCore;
    IProtocolParser* m_parser;

    QAtomicInt m_running;
    QAtomicInt m_connected;
};

#endif // SIMULATIONWORKER_H
