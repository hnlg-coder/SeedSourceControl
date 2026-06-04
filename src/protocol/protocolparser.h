#ifndef PROTOCOLPARSER_H
#define PROTOCOLPARSER_H

#include <QObject>
#include <QByteArray>
#include <QVariant>

/**
 * @brief 命令类型枚举
 * 
 * 定义了设备支持的各种命令类型
 */
enum class CommandType {
    ReadRegister,       // 读寄存器命令
    WriteRegister,      // 写寄存器命令
    ReadStatus,         // 读状态命令
    ControlDevice,      // 设备控制命令
    Unknown             // 未知命令
};

/**
 * @brief 设备数据结构体
 * 
 * 存储从设备接收到的数据或要发送给设备的数据
 */
struct DeviceData {
    quint8 address;     // 设备地址
    quint8 command;     // 命令码
    QByteArray data;    // 数据内容
};

/**
 * @brief 协议解析器接口类
 * 
 * 采用策略模式，定义协议解析器的通用接口
 */
class IProtocolParser {
public:
    /**
     * @brief 虚析构函数
     */
    virtual ~IProtocolParser() = default;
    
    /**
     * @brief 构建协议帧
     * @param type 命令类型
     * @param data 命令数据
     * @return 构建好的协议帧
     */
    virtual QByteArray buildFrame(CommandType type, const QVariant& data) = 0;
    
    /**
     * @brief 解析协议帧
     * @param frame 接收到的协议帧
     * @param data 解析后的数据输出
     * @return 解析是否成功
     */
    virtual bool parseFrame(const QByteArray& frame, DeviceData& data) = 0;
    
    /**
     * @brief 计算校验和
     * @param data 需要计算校验和的数据
     * @return 计算出的校验和
     */
    virtual quint8 calculateChecksum(const QByteArray& data) = 0;
};

/**
 * @brief 种子源模块协议解析器类
 * 
 * 实现《种子源模块通信协议V1.3》规范的协议解析器
 */
class SeedSourceProtocolParser : public QObject, public IProtocolParser {
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param parent 父对象指针
     */
    explicit SeedSourceProtocolParser(QObject* parent = nullptr);
    
    /**
     * @brief 构建协议帧
     * @param type 命令类型
     * @param data 命令数据
     * @return 构建好的协议帧
     */
    QByteArray buildFrame(CommandType type, const QVariant& data) override;
    
    /**
     * @brief 解析协议帧
     * @param frame 接收到的协议帧
     * @param data 解析后的数据输出
     * @return 解析是否成功
     */
    bool parseFrame(const QByteArray& frame, DeviceData& data) override;
    
    /**
     * @brief 计算校验和
     * @param data 需要计算校验和的数据
     * @return 计算出的校验和
     */
    quint8 calculateChecksum(const QByteArray& data) override;
    
    static const quint8 FRAME_HEADER = 0xAA;  // 帧头
    static const quint8 FRAME_TAIL = 0x55;    // 帧尾
    
private:
    quint8 m_deviceAddress;  // 设备地址
};

#endif // PROTOCOLPARSER_H
