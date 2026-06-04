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
     * @brief 发送命令
     * @param command 命令对象
     */
    void sendCommand(QSharedPointer<ICommand> command);
    
    /**
     * @brief 设置串口
     * @param port 串口对象指针
     */
    void setSerialPort(QSerialPort* port);
    
    /**
     * @brief 检查是否已连接
     * @return 是否已连接
     */
    bool isConnected() const { return m_connected; }
    
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
     * @brief 连接设备
     */
    void connectDevice();
    
    /**
     * @brief 断开设备
     */
    void disconnectDevice();
    
protected:
    /**
     * @brief 线程主循环
     */
    void run() override;
    
private slots:
    /**
     * @brief 串口数据准备好槽
     */
    void onSerialDataReady();
    
    /**
     * @brief 处理命令队列
     */
    void processCommandQueue();
    
    /**
     * @brief 命令完成槽
     * @param success 是否成功
     * @param result 执行结果
     */
    void onCommandCompleted(bool success, const QVariant& result);
    
    /**
     * @brief 发送下一个请求
     * @param request 请求数据
     */
    void sendNextRequest(const QByteArray& request);
    
    /**
     * @brief 检查连接状态
     */
    void checkConnection();
    
private:
    /**
     * @brief 通信状态枚举
     */
    enum CommunicationState {
        Idle,           // 空闲
        WaitingResponse,// 等待响应
        Processing      // 处理中
    };
    
    QSerialPort* m_serialPort;                     // 串口对象
    QQueue<QSharedPointer<ICommand>> m_commandQueue; // 命令队列
    QMap<quint32, QSharedPointer<ICommand>> m_pendingCommands; // 待处理命令映射
    QMutex m_queueMutex;                        // 队列互斥锁
    QMutex m_pendingMutex;                      // 待处理命令映射互斥锁
    QMutex m_bufferMutex;                       // 接收缓冲区互斥锁
    
    bool m_running;            // 是否运行中
    bool m_connected;         // 是否连接
    CommunicationState m_state;  // 通信状态
    QTimer* m_pollTimer;      // 轮询定时器
    QTimer* m_connectionCheckTimer; // 连接检查定时器
    bool m_threadFinished;    // 线程是否已完成
    
    QByteArray m_receiveBuffer; // 接收缓冲区
    
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
