#ifndef COMMAND_H
#define COMMAND_H

#include <QObject>
#include <QByteArray>
#include <QVariant>
#include <QSharedPointer>
#include <QTimer>
#include <QMutex>
#include <atomic>
#include "../protocol/protocolparser.h"

class ICommand : public QObject {
    Q_OBJECT
public:
    enum CommandState {
        Pending,
        Sending,
        WaitingResponse,
        Processing,
        Completed,
        Timeout,
        Error
    };
    Q_ENUM(CommandState)

    explicit ICommand(QObject* parent = nullptr);
    virtual ~ICommand() = default;

    virtual QByteArray buildRequest() = 0;
    virtual bool parseResponse(const QByteArray& response) = 0;
    virtual QString description() const = 0;

    virtual quint32 timeout() const { return 2000; }
    virtual int priority() const { return 0; }

    quint32 id() const { return m_id; }
    CommandState state() const { return m_state.load(std::memory_order_acquire); }
    QVariant result() const { QMutexLocker locker(&m_resultMutex); return m_result; }
    QString errorString() const { QMutexLocker locker(&m_resultMutex); return m_errorString; }

public slots:
    void execute();
    void onResponse(const QByteArray& response);
    void onTimeout();

signals:
    void stateChanged(CommandState state);
    void completed(bool success, const QVariant& result);
    void sendRequest(const QByteArray& request);

protected:
    void setState(CommandState state);
    void setResult(const QVariant& result);
    void setError(const QString& error);

    quint32 m_id;
    std::atomic<CommandState> m_state;
    mutable QMutex m_resultMutex;
    QVariant m_result;
    QString m_errorString;
    QTimer* m_timeoutTimer;

    static std::atomic<quint32> s_nextId;
};

class ReadRegisterCommand : public ICommand {
    Q_OBJECT
public:
    explicit ReadRegisterCommand(quint8 baseAddr, quint8 offset, IProtocolParser* parser, QObject* parent = nullptr);

    QByteArray buildRequest() override;
    bool parseResponse(const QByteArray& response) override;
    QString description() const override;

private:
    quint8 m_baseAddr;
    quint8 m_offset;
    IProtocolParser* m_parser;
};

class WriteRegisterCommand : public ICommand {
    Q_OBJECT
public:
    explicit WriteRegisterCommand(quint8 baseAddr, quint8 offset, quint32 value, IProtocolParser* parser, QObject* parent = nullptr);

    QByteArray buildRequest() override;
    bool parseResponse(const QByteArray& response) override;
    QString description() const override;

private:
    quint8 m_baseAddr;
    quint8 m_offset;
    quint32 m_value;
    IProtocolParser* m_parser;
};

class ReadStatusCommand : public ICommand {
    Q_OBJECT
public:
    explicit ReadStatusCommand(IProtocolParser* parser, QObject* parent = nullptr);

    QByteArray buildRequest() override;
    bool parseResponse(const QByteArray& response) override;
    QString description() const override;

private:
    IProtocolParser* m_parser;
};

class ControlDeviceCommand : public ICommand {
    Q_OBJECT
public:
    enum ControlAction {
        Start,
        Stop,
        Reset,
        Calibrate
    };

    explicit ControlDeviceCommand(ControlAction action, IProtocolParser* parser, QObject* parent = nullptr);

    QByteArray buildRequest() override;
    bool parseResponse(const QByteArray& response) override;
    QString description() const override;

private:
    ControlAction m_action;
    IProtocolParser* m_parser;
};

class CommandFactory {
public:
    static QSharedPointer<ICommand> createReadRegisterCommand(quint8 baseAddr, quint8 offset, IProtocolParser* parser);
    static QSharedPointer<ICommand> createWriteRegisterCommand(quint8 baseAddr, quint8 offset, quint32 value, IProtocolParser* parser);
    static QSharedPointer<ICommand> createReadStatusCommand(IProtocolParser* parser);
    static QSharedPointer<ICommand> createControlDeviceCommand(ControlDeviceCommand::ControlAction action, IProtocolParser* parser);
};

#endif // COMMAND_H
