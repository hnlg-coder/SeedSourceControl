#include "communicationworker.h"
#include <QDebug>
#include <QDateTime>
#include <QMetaObject>

CommunicationWorker::CommunicationWorker(QObject* parent)
    : QThread(parent)
    , m_serialPort(nullptr)
    , m_connected(false)
    , m_state(Idle)
    , m_pollTimer(nullptr)
    , m_connectionCheckTimer(nullptr)
    , m_threadFinished(false)
    , m_baudRate(QSerialPort::Baud9600)
    , m_dataBits(QSerialPort::Data8)
    , m_parity(QSerialPort::NoParity)
    , m_stopBits(QSerialPort::OneStop)
    , m_flowControl(QSerialPort::NoFlowControl)
{
}

CommunicationWorker::~CommunicationWorker()
{
    stopCommunication();
    if (isRunning()) {
        wait();
    }
}

void CommunicationWorker::setSerialConfig(const QString& portName, qint32 baudRate,
                                           QSerialPort::DataBits dataBits,
                                           QSerialPort::Parity parity,
                                           QSerialPort::StopBits stopBits,
                                           QSerialPort::FlowControl flowControl)
{
    QMutexLocker locker(&m_configMutex);
    m_portName = portName;
    m_baudRate = baudRate;
    m_dataBits = dataBits;
    m_parity = parity;
    m_stopBits = stopBits;
    m_flowControl = flowControl;
}

bool CommunicationWorker::isConnected() const
{
    QMutexLocker locker(&m_stateMutex);
    return m_connected;
}

void CommunicationWorker::run()
{
    // 在子线程中创建QSerialPort，确保所有串口操作在同一线程
    m_serialPort = new QSerialPort();
    {
        QMutexLocker locker(&m_configMutex);
        m_serialPort->setPortName(m_portName);
        m_serialPort->setBaudRate(m_baudRate);
        m_serialPort->setDataBits(m_dataBits);
        m_serialPort->setParity(m_parity);
        m_serialPort->setStopBits(m_stopBits);
        m_serialPort->setFlowControl(m_flowControl);
    }

    // 连接串口数据就绪信号（在子线程中）
    connect(m_serialPort, &QSerialPort::readyRead, this, &CommunicationWorker::onSerialDataReady);

    // 在子线程中创建定时器
    m_pollTimer = new QTimer();
    m_connectionCheckTimer = new QTimer();

    // 连接信号槽（在子线程中）
    connect(m_pollTimer, &QTimer::timeout, this, &CommunicationWorker::processCommandQueue);
    connect(m_connectionCheckTimer, &QTimer::timeout, this, &CommunicationWorker::checkConnection);

    // 连接主线程的请求信号到子线程的执行槽
    connect(this, &CommunicationWorker::connectRequested, this, &CommunicationWorker::doConnectDevice, Qt::QueuedConnection);
    connect(this, &CommunicationWorker::disconnectRequested, this, &CommunicationWorker::doDisconnectDevice, Qt::QueuedConnection);
    connect(this, &CommunicationWorker::sendCommandRequested, this, &CommunicationWorker::doSendCommand, Qt::QueuedConnection);

    m_threadFinished = false;
    m_pollTimer->start(10);
    m_connectionCheckTimer->start(5000);

    // 进入事件循环
    exec();

    // 清理串口
    if (m_serialPort) {
        if (m_serialPort->isOpen()) {
            m_serialPort->close();
        }
        disconnect(m_serialPort, &QSerialPort::readyRead, this, &CommunicationWorker::onSerialDataReady);
        delete m_serialPort;
        m_serialPort = nullptr;
    }

    // 清理定时器
    m_pollTimer->stop();
    m_connectionCheckTimer->stop();
    delete m_pollTimer;
    delete m_connectionCheckTimer;
    m_pollTimer = nullptr;
    m_connectionCheckTimer = nullptr;

    {
        QMutexLocker locker(&m_stateMutex);
        m_connected = false;
    }

    m_threadFinished = true;
}

void CommunicationWorker::startCommunication()
{
    if (!isRunning()) {
        start();
    }
    emit logMessage("Communication started");
}

void CommunicationWorker::stopCommunication()
{
    // 如果线程正在运行，请求断开并退出事件循环
    if (isRunning()) {
        emit disconnectRequested();
        quit();
    }

    emit logMessage("Communication stopped");
}

void CommunicationWorker::connectDevice()
{
    // 通过信号通知子线程执行连接操作
    emit connectRequested();
}

void CommunicationWorker::disconnectDevice()
{
    // 通过信号通知子线程执行断开操作
    emit disconnectRequested();
}

void CommunicationWorker::sendCommand(QSharedPointer<ICommand> command)
{
    // 通过信号通知子线程处理命令
    emit sendCommandRequested(command);
}

void CommunicationWorker::doConnectDevice()
{
    if (!m_serialPort) return;

    if (!m_serialPort->isOpen()) {
        // 更新串口配置（主线程可能已更新配置）
        {
            QMutexLocker locker(&m_configMutex);
            m_serialPort->setPortName(m_portName);
            m_serialPort->setBaudRate(m_baudRate);
            m_serialPort->setDataBits(m_dataBits);
            m_serialPort->setParity(m_parity);
            m_serialPort->setStopBits(m_stopBits);
            m_serialPort->setFlowControl(m_flowControl);
        }

        if (m_serialPort->open(QIODevice::ReadWrite)) {
            {
                QMutexLocker locker(&m_stateMutex);
                m_connected = true;
            }
            {
                QMutexLocker locker(&m_bufferMutex);
                m_receiveBuffer.clear();
            }
            {
                QMutexLocker locker(&m_pendingMutex);
                m_pendingCommands.clear();
                m_state = Idle;
            }
            emit connectionStateChanged(true);
            emit logMessage("Device connected: " + m_serialPort->portName());
        } else {
            emit logMessage("Failed to connect: " + m_serialPort->errorString());
        }
    }
}

void CommunicationWorker::doDisconnectDevice()
{
    if (!m_serialPort) return;

    if (m_serialPort->isOpen()) {
        m_serialPort->close();
        {
            QMutexLocker locker(&m_stateMutex);
            m_connected = false;
        }
        {
            QMutexLocker locker(&m_bufferMutex);
            m_receiveBuffer.clear();
        }
        {
            QMutexLocker locker(&m_pendingMutex);
            m_pendingCommands.clear();
            m_state = Idle;
        }
        emit connectionStateChanged(false);
        emit logMessage("Device disconnected");
    }
}

void CommunicationWorker::doSendCommand(QSharedPointer<ICommand> command)
{
    QMutexLocker locker(&m_queueMutex);
    m_commandQueue.enqueue(command);
    emit logMessage(QString("排队: %1").arg(command->description()));
}

void CommunicationWorker::processCommandQueue()
{
    {
        QMutexLocker stateLocker(&m_stateMutex);
        QMutexLocker pendingLocker(&m_pendingMutex);
        if (!m_connected || m_state != Idle) {
            return;
        }
    }

    QSharedPointer<ICommand> command;
    {
        QMutexLocker locker(&m_queueMutex);
        if (m_commandQueue.isEmpty()) {
            return;
        }
        command = m_commandQueue.dequeue();
    }

    {
        QMutexLocker locker(&m_pendingMutex);
        m_pendingCommands[command->id()] = command;
        m_state = WaitingResponse;
    }

    disconnect(command.data(), &ICommand::sendRequest, this, &CommunicationWorker::sendNextRequest);
    disconnect(command.data(), &ICommand::completed, this, &CommunicationWorker::onCommandCompleted);
    connect(command.data(), &ICommand::sendRequest, this, &CommunicationWorker::sendNextRequest);
    connect(command.data(), &ICommand::completed, this, &CommunicationWorker::onCommandCompleted);

    command->execute();
    emit logMessage(QString("执行: %1").arg(command->description()));
}

void CommunicationWorker::sendNextRequest(const QByteArray& request)
{
    if (m_serialPort && m_serialPort->isOpen()) {
        m_serialPort->write(request);
        m_serialPort->flush();

        QString hexStr;
    for (int i = 0; i < request.size(); ++i) {
        hexStr += QString("%1 ").arg(static_cast<quint8>(request[i]), 2, 16, QChar('0'));
    }

    ICommand* cmd = qobject_cast<ICommand*>(sender());
    QString desc = cmd ? cmd->description() : QString("未知命令");
    emit logMessage(QString("发送: %1 [%2]").arg(desc).arg(hexStr.trimmed()));
    }
}

void CommunicationWorker::onSerialDataReady()
{
    if (!m_serialPort) return;

    QByteArray data = m_serialPort->readAll();
    {
        QMutexLocker locker(&m_bufferMutex);
        m_receiveBuffer.append(data);
    }

    QString hexStr;
    for (int i = 0; i < data.size(); ++i) {
        hexStr += QString("%1 ").arg(static_cast<quint8>(data[i]), 2, 16, QChar('0'));
    }
    emit logMessage(QString("接收: %1").arg(hexStr.trimmed()));

    processReceivedData();
}

void CommunicationWorker::processReceivedData()
{
    QByteArray frame;
    while (true) {
        {
            QMutexLocker locker(&m_bufferMutex);
            if (!findCompleteFrame(frame)) {
                break;
            }
        }

        if (frame.isEmpty()) {
            break;
        }

        emit dataReceived(frame);

        QSharedPointer<ICommand> command;
        {
            QMutexLocker locker(&m_pendingMutex);
            if (!m_pendingCommands.isEmpty()) {
                auto it = m_pendingCommands.begin();
                command = it.value();
            }
        }

        if (command) {
            // 使用QMetaObject::invokeMethod确保onResponse在ICommand所在线程执行
            // ICommand在主线程创建，其QTimer也在主线程，onResponse需要在同一线程
            QMetaObject::invokeMethod(command.data(), "onResponse",
                                      Qt::QueuedConnection,
                                      Q_ARG(QByteArray, frame));
        }
    }
}

bool CommunicationWorker::findCompleteFrame(QByteArray& frame)
{
    const quint8 FRAME_HEADER = 0xAA;
    const quint8 FRAME_TAIL = 0x55;
    static const int MAX_BUFFER_SIZE = 4096;

    // 丢弃帧头之前的垃圾数据
    int headerIndex = m_receiveBuffer.indexOf(FRAME_HEADER);
    if (headerIndex > 0) {
        m_receiveBuffer = m_receiveBuffer.mid(headerIndex);
        headerIndex = 0;
    }

    // 缓冲区过大且无有效帧头，清空防止内存耗尽
    if (headerIndex == -1 && m_receiveBuffer.size() > MAX_BUFFER_SIZE) {
        m_receiveBuffer.clear();
        return false;
    }

    while (headerIndex != -1) {
        if (m_receiveBuffer.size() < headerIndex + 6) {
            return false;
        }

        quint8 dataLength = static_cast<quint8>(m_receiveBuffer[headerIndex + 3]);
        int frameLength = headerIndex + 5 + dataLength + 1;

        if (m_receiveBuffer.size() < frameLength) {
            return false;
        }

        if (static_cast<quint8>(m_receiveBuffer[frameLength - 1]) == FRAME_TAIL) {
            frame = m_receiveBuffer.mid(headerIndex, frameLength - headerIndex);
            m_receiveBuffer = m_receiveBuffer.mid(frameLength);
            return true;
        }

        m_receiveBuffer = m_receiveBuffer.mid(headerIndex + 1);
        headerIndex = m_receiveBuffer.indexOf(FRAME_HEADER);
    }

    return false;
}

void CommunicationWorker::onCommandCompleted(bool success, const QVariant& result)
{
    ICommand* command = qobject_cast<ICommand*>(sender());
    if (command) {
        quint32 cmdId = command->id();
        {
            QMutexLocker locker(&m_pendingMutex);
            m_pendingCommands.remove(cmdId);
            m_state = Idle;
        }

        QString desc = command->description();
        if (success) {
            emit logMessage(QString("成功: %1").arg(desc));
        } else {
            QString err = command->errorString();
            emit logMessage(QString("失败: %1 - %2").arg(desc).arg(err));
        }
        emit commandCompleted(cmdId, success, result);
    }
}

void CommunicationWorker::checkConnection()
{
    if (m_serialPort) {
        bool wasConnected;
        bool newConnected;
        {
            QMutexLocker locker(&m_stateMutex);
            wasConnected = m_connected;
            m_connected = m_serialPort->isOpen();
            newConnected = m_connected;
        }

        if (wasConnected != newConnected) {
            emit connectionStateChanged(newConnected);
        }
    }
}
