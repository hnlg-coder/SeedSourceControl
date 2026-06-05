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
    QMetaObject::invokeMethod(m_timeoutTimer, "start", Qt::QueuedConnection,
                              Q_ARG(int, static_cast<int>(timeout())));
    emit sendRequest(request);
}

void ICommand::onResponse(const QByteArray& response)
{
    CommandState expected = WaitingResponse;
    if (!m_state.compare_exchange_strong(expected, Processing,
            std::memory_order_acq_rel, std::memory_order_acquire)) {
        return;
    }

    m_timeoutTimer->stop();

    if (parseResponse(response)) {
        QMutexLocker locker(&m_resultMutex);
        m_state.store(Completed, std::memory_order_release);
        QVariant result = m_result;
        locker.unlock();
        emit completed(true, result);
    } else {
        QMutexLocker locker(&m_resultMutex);
        m_state.store(Error, std::memory_order_release);
        m_errorString = QStringLiteral("Failed to parse response");
        locker.unlock();
        emit completed(false, QVariant());
    }
}

void ICommand::onTimeout()
{
    CommandState expected = WaitingResponse;
    if (!m_state.compare_exchange_strong(expected, Timeout,
            std::memory_order_acq_rel, std::memory_order_acquire)) {
        return;
    }

    m_timeoutTimer->stop();
    {
        QMutexLocker locker(&m_resultMutex);
        m_errorString = QStringLiteral("Command timeout");
    }
    emit completed(false, QVariant());
}

void ICommand::setState(CommandState state)
{
    CommandState old = m_state.exchange(state, std::memory_order_acq_rel);
    if (old != state) {
        emit stateChanged(state);
    }
}

void ICommand::setResult(const QVariant& result)
{
    QMutexLocker locker(&m_resultMutex);
    m_result = result;
}

void ICommand::setError(const QString& error)
{
    QMutexLocker locker(&m_resultMutex);
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
    if (!m_parser) return QByteArray();
    QVariantList data;
    data << m_baseAddr << m_offset;
    return m_parser->buildFrame(CommandType::ReadRegister, data);
}

bool ReadRegisterCommand::parseResponse(const QByteArray& response)
{
    if (!m_parser) return false;
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

QString ReadRegisterCommand::description() const
{
    return QString("读取寄存器(基址=0x%1, 偏移=0x%2)")
        .arg(m_baseAddr, 2, 16, QChar('0'))
        .arg(m_offset, 2, 16, QChar('0'));
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
    if (!m_parser) return QByteArray();
    QVariantList data;
    data << m_baseAddr << m_offset << m_value;
    return m_parser->buildFrame(CommandType::WriteRegister, data);
}

bool WriteRegisterCommand::parseResponse(const QByteArray& response)
{
    if (!m_parser) return false;
    DeviceData data;
    if (m_parser->parseFrame(response, data)) {
        setResult(m_value);
        return true;
    }
    return false;
}

QString WriteRegisterCommand::description() const
{
    return QString("写入寄存器(基址=0x%1, 偏移=0x%2, 值=0x%3)")
        .arg(m_baseAddr, 2, 16, QChar('0'))
        .arg(m_offset, 2, 16, QChar('0'))
        .arg(m_value, 8, 16, QChar('0'));
}

ReadStatusCommand::ReadStatusCommand(IProtocolParser* parser, QObject* parent)
    : ICommand(parent)
    , m_parser(parser)
{
}

QByteArray ReadStatusCommand::buildRequest()
{
    if (!m_parser) return QByteArray();
    return m_parser->buildFrame(CommandType::ReadStatus, QVariant());
}

bool ReadStatusCommand::parseResponse(const QByteArray& response)
{
    if (!m_parser) return false;
    DeviceData data;
    if (m_parser->parseFrame(response, data)) {
        QVariantMap statusMap;

        auto extractU32 = [](const QByteArray& arr, int offset) -> quint32 {
            quint32 val = 0;
            val |= static_cast<quint8>(arr[offset]) << 24;
            val |= static_cast<quint8>(arr[offset + 1]) << 16;
            val |= static_cast<quint8>(arr[offset + 2]) << 8;
            val |= static_cast<quint8>(arr[offset + 3]);
            return val;
        };

        if (data.data.size() >= 4) {
            statusMap["status"] = extractU32(data.data, 0);
        }
        if (data.data.size() >= 8) {
            statusMap["alarm"] = extractU32(data.data, 4);
        }
        if (data.data.size() >= 12) {
            statusMap["current"] = extractU32(data.data, 8);
        }
        if (data.data.size() >= 16) {
            statusMap["temperature"] = extractU32(data.data, 12);
        }
        if (data.data.size() >= 20) {
            statusMap["power"] = extractU32(data.data, 16);
        }

        setResult(statusMap);
        return true;
    }
    return false;
}

QString ReadStatusCommand::description() const
{
    return QString("读取状态寄存器(status/报警/电流/温度/功率)");
}

ControlDeviceCommand::ControlDeviceCommand(ControlAction action, IProtocolParser* parser, QObject* parent)
    : ICommand(parent)
    , m_action(action)
    , m_parser(parser)
{
}

QByteArray ControlDeviceCommand::buildRequest()
{
    if (!m_parser) return QByteArray();
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
    if (!m_parser) return false;
    DeviceData data;
    if (m_parser->parseFrame(response, data)) {
        setResult(static_cast<int>(m_action));
        return true;
    }
    return false;
}

QString ControlDeviceCommand::description() const
{
    QString name;
    switch (m_action) {
        case Start:     name = QString("启动设备"); break;
        case Stop:      name = QString("停止设备"); break;
        case Reset:     name = QString("复位设备"); break;
        case Calibrate: name = QString("校准设备"); break;
    }
    return QString("控制命令: %1").arg(name);
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
