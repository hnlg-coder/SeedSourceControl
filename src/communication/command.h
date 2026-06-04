#ifndef COMMAND_H
#define COMMAND_H

#include <QObject>
#include <QByteArray>
#include <QVariant>
#include <QSharedPointer>
#include <QTimer>
#include <atomic>
#include "../protocol/protocolparser.h"

/**
 * @brief 命令基类
 * 
 * 采用命令模式，定义所有设备命令的通用接口
 */
class ICommand : public QObject {
    Q_OBJECT
public:
    /**
     * @brief 命令状态枚举
     */
    enum CommandState {
        Pending,        // 待执行
        Sending,        // 发送中
        WaitingResponse,// 等待响应
        Completed,      // 已完成
        Timeout,        // 超时
        Error           // 错误
    };
    Q_ENUM(CommandState)
    
    /**
     * @brief 构造函数
     * @param parent 父对象指针
     */
    explicit ICommand(QObject* parent = nullptr);
    
    /**
     * @brief 虚析构函数
     */
    virtual ~ICommand() = default;
    
    /**
     * @brief 构建请求帧（纯虚函数）
     * @return 构建好的请求帧数据
     */
    virtual QByteArray buildRequest() = 0;
    
    /**
     * @brief 解析响应帧（纯虚函数）
     * @param response 接收到的响应帧
     * @return 解析是否成功
     */
    virtual bool parseResponse(const QByteArray& response) = 0;
    
    /**
     * @brief 获取超时时间
     * @return 超时时间（毫秒），默认为2000ms
     */
    virtual quint32 timeout() const { return 2000; }
    
    /**
     * @brief 获取命令优先级
     * @return 优先级值，数值越大优先级越高
     */
    virtual int priority() const { return 0; }
    
    // 获取命令属性的方法
    quint32 id() const { return m_id; }
    CommandState state() const { return m_state; }
    QVariant result() const { return m_result; }
    QString errorString() const { return m_errorString; }
    
public slots:
    /**
     * @brief 执行命令
     */
    void execute();
    
    /**
     * @brief 接收到响应时的处理
     * @param response 响应数据
     */
    void onResponse(const QByteArray& response);
    
    /**
     * @brief 超时处理
     */
    void onTimeout();
    
signals:
    /**
     * @brief 状态变化信号
     * @param state 新的状态
     */
    void stateChanged(CommandState state);
    
    /**
     * @brief 命令完成信号
     * @param success 是否成功
     * @param result 命令执行结果
     */
    void completed(bool success, const QVariant& result);
    
    /**
     * @brief 发送请求信号
     * @param request 请求数据
     */
    void sendRequest(const QByteArray& request);
    
protected:
    /**
     * @brief 设置命令状态
     * @param state 新状态
     */
    void setState(CommandState state);
    
    /**
     * @brief 设置命令结果
     * @param result 结果数据
     */
    void setResult(const QVariant& result);
    
    /**
     * @brief 设置错误信息
     * @param error 错误信息
     */
    void setError(const QString& error);
    
    quint32 m_id;              // 命令ID
    CommandState m_state;      // 命令状态
    QVariant m_result;         // 命令结果
    QString m_errorString;     // 错误信息
    QTimer* m_timeoutTimer;    // 超时定时器
    
    static std::atomic<quint32> s_nextId;   // 下一个命令ID
};

/**
 * @brief 读寄存器命令类
 */
class ReadRegisterCommand : public ICommand {
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param baseAddr 基地址
     * @param offset 偏移地址
     * @param parser 协议解析器指针
     * @param parent 父对象指针
     */
    explicit ReadRegisterCommand(quint8 baseAddr, quint8 offset, IProtocolParser* parser, QObject* parent = nullptr);
    
    QByteArray buildRequest() override;
    bool parseResponse(const QByteArray& response) override;
    
private:
    quint8 m_baseAddr;         // 基地址
    quint8 m_offset;           // 偏移地址
    IProtocolParser* m_parser; // 协议解析器
};

/**
 * @brief 写寄存器命令类
 */
class WriteRegisterCommand : public ICommand {
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param baseAddr 基地址
     * @param offset 偏移地址
     * @param value 要写入的值
     * @param parser 协议解析器指针
     * @param parent 父对象指针
     */
    explicit WriteRegisterCommand(quint8 baseAddr, quint8 offset, quint32 value, IProtocolParser* parser, QObject* parent = nullptr);
    
    QByteArray buildRequest() override;
    bool parseResponse(const QByteArray& response) override;
    
private:
    quint8 m_baseAddr;         // 基地址
    quint8 m_offset;           // 偏移地址
    quint32 m_value;           // 要写入的值
    IProtocolParser* m_parser; // 协议解析器
};

/**
 * @brief 读状态命令类
 */
class ReadStatusCommand : public ICommand {
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param parser 协议解析器指针
     * @param parent 父对象指针
     */
    explicit ReadStatusCommand(IProtocolParser* parser, QObject* parent = nullptr);
    
    QByteArray buildRequest() override;
    bool parseResponse(const QByteArray& response) override;
    
private:
    IProtocolParser* m_parser; // 协议解析器
};

/**
 * @brief 设备控制命令类
 */
class ControlDeviceCommand : public ICommand {
    Q_OBJECT
public:
    /**
     * @brief 控制动作枚举
     */
    enum ControlAction {
        Start,      // 启动
        Stop,       // 停止
        Reset,      // 重置
        Calibrate   // 校准
    };
    
    /**
     * @brief 构造函数
     * @param action 控制动作
     * @param parser 协议解析器指针
     * @param parent 父对象指针
     */
    explicit ControlDeviceCommand(ControlAction action, IProtocolParser* parser, QObject* parent = nullptr);
    
    QByteArray buildRequest() override;
    bool parseResponse(const QByteArray& response) override;
    
private:
    ControlAction m_action;     // 控制动作
    IProtocolParser* m_parser;  // 协议解析器
};

/**
 * @brief 命令工厂类
 * 
 * 采用工厂模式，负责创建各种命令对象
 */
class CommandFactory {
public:
    /**
     * @brief 创建读寄存器命令
     */
    static QSharedPointer<ICommand> createReadRegisterCommand(quint8 baseAddr, quint8 offset, IProtocolParser* parser);
    
    /**
     * @brief 创建写寄存器命令
     */
    static QSharedPointer<ICommand> createWriteRegisterCommand(quint8 baseAddr, quint8 offset, quint32 value, IProtocolParser* parser);
    
    /**
     * @brief 创建读状态命令
     */
    static QSharedPointer<ICommand> createReadStatusCommand(IProtocolParser* parser);
    
    /**
     * @brief 创建设备控制命令
     */
    static QSharedPointer<ICommand> createControlDeviceCommand(ControlDeviceCommand::ControlAction action, IProtocolParser* parser);
};

#endif // COMMAND_H
