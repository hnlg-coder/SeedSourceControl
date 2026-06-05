#ifndef SIMPROTOCOLHANDLER_H
#define SIMPROTOCOLHANDLER_H

#include <QObject>
#include <QByteArray>
#include "simregistermanager.h"
#include "simdevicestatemachine.h"

class SimProtocolHandler : public QObject {
    Q_OBJECT
public:
    explicit SimProtocolHandler(SimRegisterManager* regMgr, SimDeviceStateMachine* stateMachine, QObject* parent = nullptr);

    QByteArray handleFrame(const QByteArray& frame);

signals:
    void logMessage(const QString& message);
    void commandExecuted(quint8 command);
    void commandFailed(quint8 command, const QString& error);

private:
    bool handleCommand(quint8 deviceAddr, quint8 command, const QByteArray& data, QByteArray& responseData);
    bool handleReadRegister(const QByteArray& data, QByteArray& responseData);
    bool handleWriteRegister(const QByteArray& data, QByteArray& responseData);
    bool handleReadStatus(const QByteArray& data, QByteArray& responseData);
    bool handleControlDevice(const QByteArray& data, QByteArray& responseData);

    QByteArray buildResponseFrame(quint8 deviceAddr, quint8 command, const QByteArray& data);
    QByteArray buildErrorFrame(quint8 deviceAddr, quint8 command, quint8 errorCode);
    quint8 calculateChecksum(const QByteArray& data);
    bool verifyChecksum(const QByteArray& frame);

    SimRegisterManager* m_registerManager;
    SimDeviceStateMachine* m_deviceState;

    static const quint8 FRAME_HEADER = 0xAA;
    static const quint8 FRAME_TAIL = 0x55;
    static const quint8 ERROR_NONE = 0x00;
    static const quint8 ERROR_UNKNOWN_COMMAND = 0x01;
    static const quint8 ERROR_INVALID_REGISTER = 0x02;
    static const quint8 ERROR_INVALID_DATA = 0x03;
    static const quint8 ERROR_CHECKSUM = 0x04;
    static const quint8 ERROR_INVALID_FRAME = 0x05;
};

#endif // SIMPROTOCOLHANDLER_H
