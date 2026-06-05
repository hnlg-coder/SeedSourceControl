#ifndef SIMREGISTERMANAGER_H
#define SIMREGISTERMANAGER_H

#include <QObject>
#include <QMap>
#include <QPair>
#include <QVariant>

class SimRegisterManager : public QObject {
    Q_OBJECT
public:
    explicit SimRegisterManager(QObject* parent = nullptr);

    void initialize();
    void reset();

    bool readRegister(quint8 baseAddr, quint8 offset, QVariant& value);
    bool writeRegister(quint8 baseAddr, quint8 offset, quint32 value);

    quint32 readRaw(quint8 baseAddr, quint8 offset) const;
    void writeRaw(quint8 baseAddr, quint8 offset, quint32 value);

signals:
    void logMessage(const QString& message);

private:
    QMap<QPair<quint8, quint8>, quint32> m_registers;
};

#endif // SIMREGISTERMANAGER_H
