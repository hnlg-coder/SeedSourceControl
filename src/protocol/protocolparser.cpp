#include "protocolparser.h"
#include <QDebug>

/**
 * @brief 构造函数
 * @param parent 父对象指针
 */
SeedSourceProtocolParser::SeedSourceProtocolParser(QObject* parent)
    : QObject(parent)
    , m_deviceAddress(0x01)  // 默认设备地址为0x01
{
}

/**
 * @brief 构建协议帧
 * @param type 命令类型
 * @param data 命令数据
 * @return 构建好的完整协议帧
 * 
 * 协议帧格式：[帧头 0xAA][地址码][命令码][数据长度 L][数据*L][校验和][帧尾 0x55]
 */
QByteArray SeedSourceProtocolParser::buildFrame(CommandType type, const QVariant& data)
{
    QByteArray frame;
    frame.append(FRAME_HEADER);  // 添加帧头
    frame.append(m_deviceAddress);  // 添加设备地址
    
    quint8 commandCode = 0;  // 命令码
    QByteArray payload;      // 数据载荷
    
    // 根据命令类型构建不同的协议帧
    switch (type) {
        case CommandType::ReadRegister: {  // 读寄存器命令
            commandCode = 0x01;
            QVariantList list = data.toList();
            if (list.size() >= 2) {
                quint8 baseAddr = list[0].toUInt();  // 基地址
                quint8 offset = list[1].toUInt();     // 偏移地址
                payload.append(baseAddr);
                payload.append(offset);
            }
            break;
        }
        case CommandType::WriteRegister: {  // 写寄存器命令
            commandCode = 0x02;
            QVariantList list = data.toList();
            if (list.size() >= 3) {
                quint8 baseAddr = list[0].toUInt();    // 基地址
                quint8 offset = list[1].toUInt();       // 偏移地址
                quint32 value = list[2].toUInt();       // 要写入的值
                payload.append(baseAddr);
                payload.append(offset);
                // 将32位值按大端序写入
                payload.append((value >> 24) & 0xFF);
                payload.append((value >> 16) & 0xFF);
                payload.append((value >> 8) & 0xFF);
                payload.append(value & 0xFF);
            }
            break;
        }
        case CommandType::ReadStatus: {  // 读状态命令
            commandCode = 0x03;
            break;
        }
        case CommandType::ControlDevice: {  // 设备控制命令
            commandCode = 0x04;
            payload.append(data.toUInt() & 0xFF);  // 控制参数
            break;
        }
        default:
            return QByteArray();  // 未知命令，返回空帧
    }
    
    // 组装完整协议帧
    frame.append(commandCode);                          // 命令码
    frame.append(static_cast<quint8>(payload.size()));  // 数据长度
    frame.append(payload);                              // 数据载荷
    
    // 计算并添加校验和
    quint8 checksum = calculateChecksum(frame);
    frame.append(checksum);
    frame.append(FRAME_TAIL);  // 添加帧尾
    
    return frame;
}

/**
 * @brief 解析接收到的协议帧
 * @param frame 接收到的协议帧数据
 * @param data 解析后的数据输出
 * @return 解析是否成功
 */
bool SeedSourceProtocolParser::parseFrame(const QByteArray& frame, DeviceData& data)
{
    // 1. 检查帧长度是否足够
    if (frame.size() < 6) {  // 最小帧长度：头+地址+命令+长度+校验+尾
        return false;
    }
    
    // 2. 检查帧头是否正确
    if (static_cast<quint8>(frame[0]) != FRAME_HEADER) {
        return false;
    }
    
    // 3. 检查帧尾是否正确
    if (static_cast<quint8>(frame[frame.size() - 1]) != FRAME_TAIL) {
        return false;
    }
    
    // 4. 检查数据长度是否匹配
    quint8 dataLength = static_cast<quint8>(frame[3]);
    if (frame.size() != 5 + dataLength + 1) {  // 5=头+地址+命令+长度+校验位位置
        return false;
    }
    
    // 5. 验证校验和
    QByteArray checksumData = frame.left(frame.size() - 2);  // 取除校验和和帧尾外的数据
    quint8 calculatedChecksum = calculateChecksum(checksumData);
    quint8 receivedChecksum = static_cast<quint8>(frame[frame.size() - 2]);
    
    if (calculatedChecksum != receivedChecksum) {
        qDebug() << "校验和错误: 计算值" << calculatedChecksum << "接收值" << receivedChecksum;
        return false;
    }
    
    // 6. 提取数据
    data.address = static_cast<quint8>(frame[1]);  // 设备地址
    data.command = static_cast<quint8>(frame[2]);  // 命令码
    data.data = frame.mid(4, dataLength);         // 数据载荷
    
    return true;
}

/**
 * @brief 计算校验和
 * @param data 需要计算校验和的数据
 * @return 计算出的校验和（低8位）
 * 
 * 校验和计算方法：从帧头到数据部分的累加和，取低8位
 */
quint8 SeedSourceProtocolParser::calculateChecksum(const QByteArray& data)
{
    quint32 sum = 0;
    for (int i = 0; i < data.size(); ++i) {
        sum += static_cast<quint8>(data[i]);  // 累加所有字节
    }
    return static_cast<quint8>(sum & 0xFF);  // 返回低8位
}
