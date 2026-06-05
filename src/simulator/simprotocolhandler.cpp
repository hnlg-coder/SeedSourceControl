#include "simprotocolhandler.h"
#include <QDebug>

SimProtocolHandler::SimProtocolHandler(SimRegisterManager* regMgr, SimDeviceStateMachine* stateMachine, QObject* parent)
    : QObject(parent)
    , m_registerManager(regMgr)
    , m_deviceState(stateMachine)
{
}

QByteArray SimProtocolHandler::handleFrame(const QByteArray& frame)
{
    if (frame.size() < 6) {
        emit logMessage(QStringLiteral("Frame too short"));
        return QByteArray();
    }

    if (static_cast<quint8>(frame[0]) != FRAME_HEADER) {
        emit logMessage(QStringLiteral("Invalid frame header"));
        return QByteArray();
    }

    if (static_cast<quint8>(frame[frame.size() - 1]) != FRAME_TAIL) {
        emit logMessage(QStringLiteral("Invalid frame tail"));
        return QByteArray();
    }

    if (!verifyChecksum(frame)) {
        emit logMessage(QStringLiteral("Checksum verification failed"));
        quint8 deviceAddr = static_cast<quint8>(frame[1]);
        quint8 command = static_cast<quint8>(frame[2]);
        return buildErrorFrame(deviceAddr, command, ERROR_CHECKSUM);
    }

    quint8 deviceAddr = static_cast<quint8>(frame[1]);
    quint8 command = static_cast<quint8>(frame[2]);
    quint8 dataLength = static_cast<quint8>(frame[3]);
    QByteArray data = frame.mid(4, dataLength);

    emit logMessage(QStringLiteral("Received command: 0x%1").arg(command, 2, 16, QChar('0')));

    QByteArray responseData;
    bool success = handleCommand(deviceAddr, command, data, responseData);

    if (success) {
        emit commandExecuted(command);
        return buildResponseFrame(deviceAddr, command, responseData);
    } else {
        emit commandFailed(command, QStringLiteral("Command execution failed"));
        return buildErrorFrame(deviceAddr, command, ERROR_INVALID_DATA);
    }
}

bool SimProtocolHandler::handleCommand(quint8 deviceAddr, quint8 command, const QByteArray& data, QByteArray& responseData)
{
    Q_UNUSED(deviceAddr);

    switch (command) {
        case 0xF0:
            return handleReadRegister(data, responseData);
        case 0x0F:
            return handleWriteRegister(data, responseData);
        case 0x03:
            return handleReadStatus(data, responseData);
        case 0x04:
            return handleControlDevice(data, responseData);
        default:
            emit logMessage(QStringLiteral("Unknown command: 0x%1").arg(command, 2, 16, QChar('0')));
            return false;
    }
}

bool SimProtocolHandler::handleReadRegister(const QByteArray& data, QByteArray& responseData)
{
    if (data.size() < 2) {
        emit logMessage(QStringLiteral("Read register: insufficient data"));
        return false;
    }

    quint8 baseAddr = static_cast<quint8>(data[0]);
    quint8 offset = static_cast<quint8>(data[1]);

    QVariant value;
    if (m_registerManager->readRegister(baseAddr, offset, value)) {
        quint32 regValue;
        if (baseAddr == 0x03) {
            regValue = static_cast<quint32>(value.toInt());
        } else {
            regValue = value.toUInt();
        }
        responseData.append(baseAddr);
        responseData.append(offset);
        responseData.append((regValue >> 24) & 0xFF);
        responseData.append((regValue >> 16) & 0xFF);
        responseData.append((regValue >> 8) & 0xFF);
        responseData.append(regValue & 0xFF);
        emit logMessage(QStringLiteral("Read register [0x%1:0x%2] = %3")
                       .arg(baseAddr, 2, 16, QChar('0'))
                       .arg(offset, 2, 16, QChar('0'))
                       .arg(regValue));
        return true;
    } else {
        emit logMessage(QStringLiteral("Read register [0x%1:0x%2] failed")
                       .arg(baseAddr, 2, 16, QChar('0'))
                       .arg(offset, 2, 16, QChar('0')));
        return false;
    }
}

bool SimProtocolHandler::handleWriteRegister(const QByteArray& data, QByteArray& responseData)
{
    if (data.size() < 6) {
        emit logMessage(QStringLiteral("Write register: insufficient data"));
        return false;
    }

    quint8 baseAddr = static_cast<quint8>(data[0]);
    quint8 offset = static_cast<quint8>(data[1]);
    quint32 value = (static_cast<quint32>(static_cast<quint8>(data[2])) << 24) |
                   (static_cast<quint32>(static_cast<quint8>(data[3])) << 16) |
                   (static_cast<quint32>(static_cast<quint8>(data[4])) << 8) |
                   static_cast<quint32>(static_cast<quint8>(data[5]));

    if (m_registerManager->writeRegister(baseAddr, offset, value)) {
        responseData.append(baseAddr);
        responseData.append(offset);
        responseData.append(static_cast<char>(0x00));
        emit logMessage(QStringLiteral("Write register [0x%1:0x%2] = %3")
                       .arg(baseAddr, 2, 16, QChar('0'))
                       .arg(offset, 2, 16, QChar('0'))
                       .arg(value));
        return true;
    } else {
        emit logMessage(QStringLiteral("Write register [0x%1:0x%2] failed")
                       .arg(baseAddr, 2, 16, QChar('0'))
                       .arg(offset, 2, 16, QChar('0')));
        return false;
    }
}

bool SimProtocolHandler::handleReadStatus(const QByteArray& data, QByteArray& responseData)
{
    Q_UNUSED(data);

    QVariant statusValue;
    if (m_registerManager->readRegister(0x00, 0x00, statusValue)) {
        quint32 status = statusValue.toUInt();
        responseData.append((status >> 24) & 0xFF);
        responseData.append((status >> 16) & 0xFF);
        responseData.append((status >> 8) & 0xFF);
        responseData.append(status & 0xFF);

        QVariant alertValue;
        if (m_registerManager->readRegister(0x01, 0x00, alertValue)) {
            quint32 alert = alertValue.toUInt();
            responseData.append((alert >> 24) & 0xFF);
            responseData.append((alert >> 16) & 0xFF);
            responseData.append((alert >> 8) & 0xFF);
            responseData.append(alert & 0xFF);
        }

        QVariant curValue;
        if (m_registerManager->readRegister(0x02, 0x01, curValue)) {
            quint32 cur = curValue.toUInt();
            responseData.append((cur >> 24) & 0xFF);
            responseData.append((cur >> 16) & 0xFF);
            responseData.append((cur >> 8) & 0xFF);
            responseData.append(cur & 0xFF);
        } else {
            QVariant curSetValue;
            if (m_registerManager->readRegister(0x02, 0x00, curSetValue)) {
                quint32 cur = curSetValue.toUInt();
                responseData.append((cur >> 24) & 0xFF);
                responseData.append((cur >> 16) & 0xFF);
                responseData.append((cur >> 8) & 0xFF);
                responseData.append(cur & 0xFF);
            }
        }

        QVariant tempValue;
        if (m_registerManager->readRegister(0x03, 0x01, tempValue)) {
            quint32 temp = static_cast<quint32>(tempValue.toInt());
            responseData.append((temp >> 24) & 0xFF);
            responseData.append((temp >> 16) & 0xFF);
            responseData.append((temp >> 8) & 0xFF);
            responseData.append(temp & 0xFF);
        } else {
            QVariant tempSetValue;
            if (m_registerManager->readRegister(0x03, 0x00, tempSetValue)) {
                quint32 temp = static_cast<quint32>(tempSetValue.toInt());
                responseData.append((temp >> 24) & 0xFF);
                responseData.append((temp >> 16) & 0xFF);
                responseData.append((temp >> 8) & 0xFF);
                responseData.append(temp & 0xFF);
            }
        }

        QVariant powerValue;
        if (m_registerManager->readRegister(0x04, 0x00, powerValue)) {
            quint32 power = powerValue.toUInt();
            responseData.append((power >> 24) & 0xFF);
            responseData.append((power >> 16) & 0xFF);
            responseData.append((power >> 8) & 0xFF);
            responseData.append(power & 0xFF);
        }

        emit logMessage(QStringLiteral("Read status: 0x%1").arg(status, 8, 16, QChar('0')));
        return true;
    }

    return false;
}

bool SimProtocolHandler::handleControlDevice(const QByteArray& data, QByteArray& responseData)
{
    if (data.size() < 1) {
        emit logMessage(QStringLiteral("Control device: insufficient data"));
        return false;
    }

    quint8 controlCode = static_cast<quint8>(data[0]);

    QVariant systemValue;
    if (m_registerManager->readRegister(0x06, 0x00, systemValue)) {
        quint32 systemReg = systemValue.toUInt();

        switch (controlCode) {
            case 0x01:
                systemReg |= 0x01;
                m_deviceState->start();
                emit logMessage(QStringLiteral("Device start command received"));
                break;
            case 0x02:
                systemReg &= ~0x01;
                m_deviceState->stop();
                emit logMessage(QStringLiteral("Device stop command received"));
                break;
            case 0x03:
                systemReg = 0;
                m_deviceState->reset();
                emit logMessage(QStringLiteral("Device reset command received"));
                break;
        }

        if (m_registerManager->writeRegister(0x06, 0x00, systemReg)) {
            responseData.append(controlCode);
            responseData.append(static_cast<char>(0x00));
            emit logMessage(QStringLiteral("Device control: 0x%1").arg(controlCode, 2, 16, QChar('0')));
            return true;
        }
    }

    return false;
}

QByteArray SimProtocolHandler::buildResponseFrame(quint8 deviceAddr, quint8 command, const QByteArray& data)
{
    QByteArray frame;
    frame.append(FRAME_HEADER);
    frame.append(deviceAddr);
    frame.append(command);
    frame.append(static_cast<quint8>(data.size()));
    frame.append(data);

    quint8 checksum = calculateChecksum(frame);
    frame.append(checksum);
    frame.append(FRAME_TAIL);

    return frame;
}

QByteArray SimProtocolHandler::buildErrorFrame(quint8 deviceAddr, quint8 command, quint8 errorCode)
{
    QByteArray data;
    data.append(errorCode);
    return buildResponseFrame(deviceAddr, command, data);
}

quint8 SimProtocolHandler::calculateChecksum(const QByteArray& data)
{
    quint32 sum = 0;
    for (int i = 0; i < data.size(); ++i) {
        sum += static_cast<quint8>(data[i]);
    }
    return static_cast<quint8>(sum & 0xFF);
}

bool SimProtocolHandler::verifyChecksum(const QByteArray& frame)
{
    QByteArray checksumData = frame.left(frame.size() - 2);
    quint8 calculatedChecksum = calculateChecksum(checksumData);
    quint8 receivedChecksum = static_cast<quint8>(frame[frame.size() - 2]);
    return calculatedChecksum == receivedChecksum;
}
