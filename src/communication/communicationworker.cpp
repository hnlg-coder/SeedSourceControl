#include "communicationworker.h"
#include <QDebug>
#include <QDateTime>

CommunicationWorker::CommunicationWorker(QObject* parent)
    : QThread(parent)
    , m_serialPort(nullptr)
    , m_running(false)
    , m_connected(false)
    , m_state(Idle)
    , m_pollTimer(nullptr)
    , m_connectionCheckTimer(nullptr)
    , m_threadFinished(false)
{
}

CommunicationWorker::~CommunicationWorker()
{
    stopCommunication();
    if (isRunning()) {
        wait();
    }
}

void CommunicationWorker::setSerialPort(QSerialPort* port)
{
    if (m_serialPort) {
        disconnect(m_serialPort, &QSerialPort::readyRead, this, &CommunicationWorker::onSerialDataReady);
    }
    m_serialPort = port;
    if (m_serialPort) {
        connect(m_serialPort, &QSerialPort::readyRead, this, &CommunicationWorker::onSerialDataReady);
    }
}

void CommunicationWorker::run()
{
    // 在子线程中创建定时器
    m_pollTimer = new QTimer();
    m_connectionCheckTimer = new QTimer();
    
    // 连接信号槽（在子线程中）
    connect(m_pollTimer, &QTimer::timeout, this, &CommunicationWorker::processCommandQueue);
    connect(m_connectionCheckTimer, &QTimer::timeout, this, &CommunicationWorker::checkConnection);
    
    m_running = true;
    m_threadFinished = false;
    m_pollTimer->start(10);
    m_connectionCheckTimer->start(5000);
    
    // 进入事件循环
    exec();
    
    // 清理定时器
    m_pollTimer->stop();
    m_connectionCheckTimer->stop();
    delete m_pollTimer;
    delete m_connectionCheckTimer;
    m_pollTimer = nullptr;
    m_connectionCheckTimer = nullptr;
    
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
    m_running = false;
    
    // 先断开串口信号
    if (m_serialPort) {
        disconnect(m_serialPort, &QSerialPort::readyRead, this, &CommunicationWorker::onSerialDataReady);
    }
    
    // 如果定时器存在，先停止它们（但不能删除，因为它们在子线程中）
    // 定时器的删除会在 run() 函数退出后处理
    
    quit();
    emit logMessage("Communication stopped");
}

void CommunicationWorker::connectDevice()
{
    if (m_serialPort && !m_serialPort->isOpen()) {
        if (m_serialPort->open(QIODevice::ReadWrite)) {
            m_connected = true;
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

void CommunicationWorker::disconnectDevice()
{
    if (m_serialPort && m_serialPort->isOpen()) {
        m_serialPort->close();
        m_connected = false;
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

void CommunicationWorker::sendCommand(QSharedPointer<ICommand> command)
{
    QMutexLocker locker(&m_queueMutex);
    m_commandQueue.enqueue(command);
    emit logMessage(QString("Command %1 queued").arg(command->id()));
}

void CommunicationWorker::processCommandQueue()
{
    bool canProcess = false;
    {
        QMutexLocker locker(&m_pendingMutex);
        canProcess = m_connected && (m_state == Idle);
    }
    
    if (!canProcess) {
        return;
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
    
    connect(command.data(), &ICommand::sendRequest, this, &CommunicationWorker::sendNextRequest);
    connect(command.data(), &ICommand::completed, this, &CommunicationWorker::onCommandCompleted);
    
    command->execute();
    emit logMessage(QString("Command %1 executing").arg(command->id()));
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
        emit logMessage(QString("TX: %1").arg(hexStr));
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
    emit logMessage(QString("RX: %1").arg(hexStr));
    
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
            command->onResponse(frame);
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
        
        emit commandCompleted(cmdId, success, result);
        emit logMessage(QString("Command %1 completed: %2").arg(cmdId).arg(success ? "success" : "failed"));
    }
}

void CommunicationWorker::checkConnection()
{
    if (m_serialPort) {
        bool wasConnected = m_connected;
        m_connected = m_serialPort->isOpen();
        
        if (wasConnected != m_connected) {
            emit connectionStateChanged(m_connected);
        }
    }
}
