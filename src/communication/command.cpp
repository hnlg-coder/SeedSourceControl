#include "command.h"
#include <QDebug>
#include <QMetaObject>

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
    // 使用QMetaObject::invokeMethod确保QTimer在ICommand所在线程启动
    // ICommand可能在主线程创建（QTimer也在主线程），但execute()可能从子线程调用
    QMetaObject::invokeMethod(m_timeoutTimer, "start", Qt::QueuedConnection,
                              Q_ARG(int, static_cast<int>(timeout())));
    emit sendRequest(request);
}

void ICommand::onResponse(const QByteArray& response)
{
    // 确保在ICommand所在线程停止定时器
    QMetaObject::invokeMethod(m_timeoutTimer, "stop", Qt::QueuedConnection);

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

        // 辅助lambda: 从字节数组中提取32位大端值
        auto extractU32 = [](const QByteArray& arr, int offset) -> quint32 {
            quint32 val = 0;
            val |= static_cast<quint8>(arr[offset]) << 24;
            val |= static_cast<quint8>(arr[offset + 1]) << 16;
            val |= static_cast<quint8>(arr[offset + 2]) << 8;
            val |= static_cast<quint8>(arr[offset + 3]);
            return val;
        };

        // 解析 STATUS 寄存器 (偏移0-3)
        if (data.data.size() >= 4) {
            statusMap["status"] = extractU32(data.data, 0);
        }
        // 解析 ALERT 寄存器 (偏移4-7)
        if (data.data.size() >= 8) {
            statusMap["alarm"] = extractU32(data.data, 4);
        }
        // 解析 CUR 寄存器 (偏移8-11)
        if (data.data.size() >= 12) {
            statusMap["current"] = extractU32(data.data, 8);
        }
        // 解析 TEMP 寄存器 (偏移12-15)
        if (data.data.size() >= 16) {
            statusMap["temperature"] = extractU32(data.data, 12);
        }
        // 解析 POWER 寄存器 (偏移16-19)
        if (data.data.size() >= 20) {
            statusMap["power"] = extractU32(data.data, 16);
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
