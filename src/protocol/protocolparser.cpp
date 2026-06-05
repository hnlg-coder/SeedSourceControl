#include "protocolparser.h"
#include <QDebug>

SeedSourceProtocolParser::SeedSourceProtocolParser(QObject* parent)
    : QObject(parent)
    , m_deviceAddress(0x01)
{
}

QByteArray SeedSourceProtocolParser::buildFrame(CommandType type, const QVariant& data)
{
    QByteArray frame;
    frame.append(FRAME_HEADER);

    quint8 addrByte = 0;
    quint8 commandCode = 0;
    QByteArray payload;

    switch (type) {
        case CommandType::ReadRegister: {
            commandCode = CMD_READ_REG;
            QVariantList list = data.toList();
            if (list.size() >= 2) {
                quint8 baseAddr = list[0].toUInt();
                quint8 offset = list[1].toUInt();
                // 协议V1.3: ADDR = (baseAddr << 4) | offset
                addrByte = static_cast<quint8>((baseAddr << 4) | (offset & 0x0F));
                // payload: 读取字节数，默认4字节
                quint8 readLen = list.size() >= 3 ? list[2].toUInt() : 4;
                payload.append(readLen);
            }
            break;
        }
        case CommandType::WriteRegister: {
            commandCode = CMD_WRITE_REG;
            QVariantList list = data.toList();
            if (list.size() >= 3) {
                quint8 baseAddr = list[0].toUInt();
                quint8 offset = list[1].toUInt();
                quint32 value = list[2].toUInt();
                addrByte = static_cast<quint8>((baseAddr << 4) | (offset & 0x0F));
                // payload: 写入字节数 + 大端序值
                quint8 writeLen = list.size() >= 4 ? list[3].toUInt() : 4;
                payload.append(writeLen);
                for (int i = writeLen - 1; i >= 0; --i) {
                    payload.append(static_cast<quint8>((value >> (i * 8)) & 0xFF));
                }
            }
            break;
        }
        case CommandType::ReadStatus: {
            commandCode = CMD_READ_STATUS;
            // 自定义批量状态读取命令（固件扩展），无payload
            break;
        }
        case CommandType::ControlDevice: {
            commandCode = CMD_CONTROL;
            payload.append(data.toUInt() & 0xFF);
            break;
        }
        default:
            return QByteArray();
    }

    frame.append(addrByte);
    frame.append(commandCode);
    frame.append(static_cast<quint8>(payload.size()));
    frame.append(payload);

    quint8 checksum = calculateChecksum(frame);
    frame.append(checksum);
    frame.append(FRAME_TAIL);

    return frame;
}

bool SeedSourceProtocolParser::parseFrame(const QByteArray& frame, DeviceData& data)
{
    if (frame.size() < 6) {
        return false;
    }

    if (static_cast<quint8>(frame[0]) != FRAME_HEADER) {
        return false;
    }

    if (static_cast<quint8>(frame[frame.size() - 1]) != FRAME_TAIL) {
        return false;
    }

    quint8 dataLength = static_cast<quint8>(frame[3]);
    if (frame.size() != 6 + dataLength) {
        return false;
    }

    QByteArray checksumData = frame.left(frame.size() - 2);
    quint8 calculatedChecksum = calculateChecksum(checksumData);
    quint8 receivedChecksum = static_cast<quint8>(frame[frame.size() - 2]);

    if (calculatedChecksum != receivedChecksum) {
        qDebug() << "checksum mismatch: calc" << calculatedChecksum << "recv" << receivedChecksum;
        return false;
    }

    data.address = static_cast<quint8>(frame[1]);
    data.command = static_cast<quint8>(frame[2]);
    data.data = frame.mid(4, dataLength);

    return true;
}

quint8 SeedSourceProtocolParser::calculateChecksum(const QByteArray& data)
{
    quint32 sum = 0;
    for (int i = 0; i < data.size(); ++i) {
        sum += static_cast<quint8>(data[i]);
    }
    return static_cast<quint8>(sum & 0xFF);
}
