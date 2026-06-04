#include "command.h"
#include <QDebug>

std::atomic<quint32> ICommand::s_nextId{1};

ICommand::ICommand(QObject* parent)
    : QObject(parent)
    , m_id(s_nextId.fetch_add(1, std::memory_order_relaxed))
    , m_state(Pending)
    , m_timeoutTimer(new QTimer(this))
{
    connect(m_timeoutTimer, &QTimer::timeout, this, &ICommand::onTimeout);
}

void ICommand::execute()
{
    setState(Sending);
    QByteArray request = buildRequest();
    
    if (request.isEmpty()) {
        setState(Error);
        setError("Failed to build request");
        emit completed(false, QVariant());
        return;
    }
    
    setState(WaitingResponse);
    m_timeoutTimer->start(timeout());
    emit sendRequest(request);
}

void ICommand::onResponse(const QByteArray& response)
{
    m_timeoutTimer->stop();

    if (m_state == WaitingResponse) {
        if (parseResponse(response)) {
            setState(Completed);
            emit completed(true, m_result);
        } else {
            setState(Error);
            setError("Failed to parse response");
            emit completed(false, QVariant());
        }
    }
}

void ICommand::onTimeout()
{
    m_timeoutTimer->stop();
    if (m_state == WaitingResponse) {
        setState(Timeout);
        setError("Command timeout");
        emit completed(false, QVariant());
    }
}

void ICommand::setState(CommandState state)
{
    if (m_state != state) {
        m_state = state;
        emit stateChanged(m_state);
    }
}

void ICommand::setResult(const QVariant& result)
{
    m_result = result;
}

void ICommand::setError(const QString& error)
{
    m_errorString = error;
}

ReadRegisterCommand::ReadRegisterCommand(quint8 baseAddr, quint8 offset, IProtocolParser* parser, QObject* parent)
    : ICommand(parent)
    , m_baseAddr(baseAddr)
    , m_offset(offset)
    , m_parser(parser)
{
}

QByteArray ReadRegisterCommand::buildRequest()
{
    QVariantList data;
    data << m_baseAddr << m_offset;
    return m_parser->buildFrame(CommandType::ReadRegister, data);
}

bool ReadRegisterCommand::parseResponse(const QByteArray& response)
{
    DeviceData data;
    if (m_parser->parseFrame(response, data)) {
        if (data.data.size() >= 4) {
            quint32 value = 0;
            value |= static_cast<quint8>(data.data[0]) << 24;
            value |= static_cast<quint8>(data.data[1]) << 16;
            value |= static_cast<quint8>(data.data[2]) << 8;
            value |= static_cast<quint8>(data.data[3]);
            setResult(value);
            return true;
        }
    }
    return false;
}

WriteRegisterCommand::WriteRegisterCommand(quint8 baseAddr, quint8 offset, quint32 value, IProtocolParser* parser, QObject* parent)
    : ICommand(parent)
    , m_baseAddr(baseAddr)
    , m_offset(offset)
    , m_value(value)
    , m_parser(parser)
{
}

QByteArray WriteRegisterCommand::buildRequest()
{
    QVariantList data;
    data << m_baseAddr << m_offset << m_value;
    return m_parser->buildFrame(CommandType::WriteRegister, data);
}

bool WriteRegisterCommand::parseResponse(const QByteArray& response)
{
    DeviceData data;
    if (m_parser->parseFrame(response, data)) {
        setResult(m_value);
        return true;
    }
    return false;
}

ReadStatusCommand::ReadStatusCommand(IProtocolParser* parser, QObject* parent)
    : ICommand(parent)
    , m_parser(parser)
{
}

QByteArray ReadStatusCommand::buildRequest()
{
    return m_parser->buildFrame(CommandType::ReadStatus, QVariant());
}

bool ReadStatusCommand::parseResponse(const QByteArray& response)
{
    DeviceData data;
    if (m_parser->parseFrame(response, data)) {
        QVariantMap statusMap;
        if (data.data.size() >= 12) {
            quint32 current = 0;
            current |= static_cast<quint8>(data.data[0]) << 24;
            current |= static_cast<quint8>(data.data[1]) << 16;
            current |= static_cast<quint8>(data.data[2]) << 8;
            current |= static_cast<quint8>(data.data[3]);
            
            quint32 temp = 0;
            temp |= static_cast<quint8>(data.data[4]) << 24;
            temp |= static_cast<quint8>(data.data[5]) << 16;
            temp |= static_cast<quint8>(data.data[6]) << 8;
            temp |= static_cast<quint8>(data.data[7]);
            
            quint32 power = 0;
            power |= static_cast<quint8>(data.data[8]) << 24;
            power |= static_cast<quint8>(data.data[9]) << 16;
            power |= static_cast<quint8>(data.data[10]) << 8;
            power |= static_cast<quint8>(data.data[11]);
            
            statusMap["current"] = current;
            statusMap["temperature"] = temp;
            statusMap["power"] = power;
        }
        setResult(statusMap);
        return true;
    }
    return false;
}

ControlDeviceCommand::ControlDeviceCommand(ControlAction action, IProtocolParser* parser, QObject* parent)
    : ICommand(parent)
    , m_action(action)
    , m_parser(parser)
{
}

QByteArray ControlDeviceCommand::buildRequest()
{
    quint8 actionCode = 0;
    switch (m_action) {
        case Start: actionCode = 0x01; break;
        case Stop: actionCode = 0x02; break;
        case Reset: actionCode = 0x03; break;
        case Calibrate: actionCode = 0x04; break;
    }
    return m_parser->buildFrame(CommandType::ControlDevice, actionCode);
}

bool ControlDeviceCommand::parseResponse(const QByteArray& response)
{
    DeviceData data;
    if (m_parser->parseFrame(response, data)) {
        setResult(m_action);
        return true;
    }
    return false;
}

QSharedPointer<ICommand> CommandFactory::createReadRegisterCommand(quint8 baseAddr, quint8 offset, IProtocolParser* parser)
{
    return QSharedPointer<ICommand>(new ReadRegisterCommand(baseAddr, offset, parser));
}

QSharedPointer<ICommand> CommandFactory::createWriteRegisterCommand(quint8 baseAddr, quint8 offset, quint32 value, IProtocolParser* parser)
{
    return QSharedPointer<ICommand>(new WriteRegisterCommand(baseAddr, offset, value, parser));
}

QSharedPointer<ICommand> CommandFactory::createReadStatusCommand(IProtocolParser* parser)
{
    return QSharedPointer<ICommand>(new ReadStatusCommand(parser));
}

QSharedPointer<ICommand> CommandFactory::createControlDeviceCommand(ControlDeviceCommand::ControlAction action, IProtocolParser* parser)
{
    return QSharedPointer<ICommand>(new ControlDeviceCommand(action, parser));
}
