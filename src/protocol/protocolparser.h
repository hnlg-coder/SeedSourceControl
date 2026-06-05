#ifndef PROTOCOLPARSER_H
#define PROTOCOLPARSER_H

#include <QObject>
#include <QByteArray>
#include <QVariant>

enum class CommandType {
    ReadRegister,
    WriteRegister,
    ReadStatus,
    ControlDevice,
    Unknown
};

struct DeviceData {
    quint8 address;
    quint8 command;
    QByteArray data;
};

class IProtocolParser {
public:
    virtual ~IProtocolParser() = default;

    virtual QByteArray buildFrame(CommandType type, const QVariant& data) = 0;
    virtual bool parseFrame(const QByteArray& frame, DeviceData& data) = 0;
    virtual quint8 calculateChecksum(const QByteArray& data) = 0;
};

class SeedSourceProtocolParser : public QObject, public IProtocolParser {
    Q_OBJECT
public:
    explicit SeedSourceProtocolParser(QObject* parent = nullptr);

    QByteArray buildFrame(CommandType type, const QVariant& data) override;
    bool parseFrame(const QByteArray& frame, DeviceData& data) override;
    quint8 calculateChecksum(const QByteArray& data) override;

    static const quint8 FRAME_HEADER = 0xAA;
    static const quint8 FRAME_TAIL = 0x55;

    // 协议V1.3命令码
    static const quint8 CMD_HEARTBEAT = 0x00;
    static const quint8 CMD_READ_REG  = 0xF0;
    static const quint8 CMD_WRITE_REG = 0x0F;
    static const quint8 CMD_READ_STATUS = 0x03;
    static const quint8 CMD_CONTROL = 0x04;
    static const quint8 CMD_ALERT = 0xAE;

private:
    quint8 m_deviceAddress;
};

#endif // PROTOCOLPARSER_H
