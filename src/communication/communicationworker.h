#ifndef COMMUNICATIONWORKER_H
#define COMMUNICATIONWORKER_H

#include <QThread>
#include <QSerialPort>
#include <QQueue>
#include <QMap>
#include <QMutex>
#include <QTimer>
#include "command.h"

/**
 * @brief 通信工作线程类
 *
 * 负责串口通信、命令队列管理、超时重试等功能
 * 所有串口操作（open/close/read/write）均在子线程中执行
 * 主线程通过信号通知子线程进行连接/断开/发送命令操作
 */
class CommunicationWorker : public QThread {
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param parent 父对象指针
     */
    explicit CommunicationWorker(QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~CommunicationWorker();

    /**
     * @brief 设置串口配置参数（线程安全，可在主线程调用）
     * @param portName 串口名称
     * @param baudRate 波特率
     * @param dataBits 数据位
     * @param parity 校验位
     * @param stopBits 停止位
     * @param flowControl 流控制
     */
    void setSerialConfig(const QString& portName, qint32 baudRate,
                         QSerialPort::DataBits dataBits,
                         QSerialPort::Parity parity,
                         QSerialPort::StopBits stopBits,
                         QSerialPort::FlowControl flowControl);

    /**
     * @brief 检查是否已连接（线程安全）
     * @return 是否已连接
     */
    bool isConnected() const;

signals:
    /**
     * @brief 数据接收信号
     * @param data 接收到的数据
     */
    void dataReceived(const QByteArray& data);

    /**
     * @brief 命令完成信号
     * @param cmdId 命令ID
     * @param success 是否成功
     * @param result 执行结果
     */
    void commandCompleted(quint32 cmdId, bool success, QVariant result);

    /**
     * @brief 连接状态变化信号
     * @param connected 连接状态
     */
    void connectionStateChanged(bool connected);

    /**
     * @brief 日志消息信号
     * @param message 日志消息
     */
    void logMessage(const QString& message);

    // 主线程通过以下信号通知子线程执行操作
    /**
     * @brief 请求连接设备（主线程发射，子线程接收）
     */
    void connectRequested();

    /**
     * @brief 请求断开设备（主线程发射，子线程接收）
     */
    void disconnectRequested();

    /**
     * @brief 请求发送命令（主线程发射，子线程接收）
     * @param command 命令对象
     */
    void sendCommandRequested(QSharedPointer<ICommand> command);

public slots:
    /**
     * @brief 开始通信
     */
    void startCommunication();

    /**
     * @brief 停止通信
     */
    void stopCommunication();

    /**
     * @brief 请求连接设备（线程安全，可在主线程调用）
     */
    void connectDevice();

    /**
     * @brief 请求断开设备（线程安全，可在主线程调用）
     */
    void disconnectDevice();

    /**
     * @brief 发送命令（线程安全，可在主线程调用）
     * @param command 命令对象
     */
    void sendCommand(QSharedPointer<ICommand> command);

    /**
     * @brief 设置协议解析器（主线程调用）
     */
    void setProtocolParser(IProtocolParser* parser) { m_protocolParser = parser; }

protected:
    /**
     * @brief 线程主循环
     */
    void run() override;

private slots:
    /**
     * @brief 串口数据准备好槽（在子线程中执行）
     */
    void onSerialDataReady();

    /**
     * @brief 处理命令队列（在子线程中执行）
     */
    void processCommandQueue();

    /**
     * @brief 命令完成槽（在子线程中执行）
     * @param success 是否成功
     * @param result 执行结果
     */
    void onCommandCompleted(bool success, const QVariant& result);

    /**
     * @brief 发送下一个请求（在子线程中执行）
     * @param request 请求数据
     */
    void sendNextRequest(const QByteArray& request);

    /**
     * @brief 在子线程中执行连接操作
     */
    void doConnectDevice();

    /**
     * @brief 在子线程中执行断开操作
     */
    void doDisconnectDevice();

    /**
     * @brief 在子线程中处理发送命令请求
     * @param command 命令对象
     */
    void doSendCommand(QSharedPointer<ICommand> command);

    /**
     * @brief 发送握手命令（在子线程中执行）
     */
    void startHandshake();

    /**
     * @brief 处理握手结果（在子线程中执行）
     */
    void onHandshakeResult(bool success, const QVariant& result);

    /**
     * @brief 握手超时处理（在子线程中执行）
     */
    void onHandshakeTimeout();

private:
    /**
     * @brief 通信状态枚举
     */
    enum CommunicationState {
        Idle,           // 空闲
        WaitingResponse,// 等待响应
        Processing      // 处理中
    };

    QSerialPort* m_serialPort;                     // 串口对象（在子线程中创建和管理）
    QQueue<QSharedPointer<ICommand>> m_commandQueue; // 命令队列
    QMap<quint32, QSharedPointer<ICommand>> m_pendingCommands; // 待处理命令映射
    QMutex m_queueMutex;                        // 队列互斥锁
    QMutex m_pendingMutex;                      // 待处理命令映射互斥锁
    QMutex m_bufferMutex;                       // 接收缓冲区互斥锁
    mutable QMutex m_stateMutex;                // 状态互斥锁（保护m_connected和m_state）
    QMutex m_configMutex;                        // 配置互斥锁（保护串口配置参数）

    bool m_connected;          // 是否连接
    CommunicationState m_state;  // 通信状态
    QTimer* m_pollTimer;      // 轮询定时器
    bool m_threadFinished;    // 线程是否已完成

    IProtocolParser* m_protocolParser;  // 协议解析器

    // 握手状态
    QSharedPointer<ICommand> m_handshakeCmd;  // 握手命令
    QTimer* m_handshakeTimer;                 // 握手超时定时器
    bool m_connecting;                        // 是否正在握手
    int m_handshakeRetries;                   // 握手重试计数

    QByteArray m_receiveBuffer; // 接收缓冲区

    // 串口配置参数（由主线程设置，子线程读取）
    QString m_portName;                              // 串口名称
    qint32 m_baudRate;                               // 波特率
    QSerialPort::DataBits m_dataBits;                // 数据位
    QSerialPort::Parity m_parity;                    // 校验位
    QSerialPort::StopBits m_stopBits;                // 停止位
    QSerialPort::FlowControl m_flowControl;          // 流控制

    /**
     * @brief 处理接收到的数据
     */
    void processReceivedData();

    /**
     * @brief 查找完整的帧
     * @param frame 输出参数，找到的帧
     * @return 是否找到完整帧
     */
    bool findCompleteFrame(QByteArray& frame);
};

#endif // COMMUNICATIONWORKER_H
