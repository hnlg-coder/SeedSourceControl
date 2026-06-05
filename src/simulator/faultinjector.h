#ifndef FAULTINJECTOR_H
#define FAULTINJECTOR_H

#include <QObject>
#include <QByteArray>
#include <QRandomGenerator>
#include <QTimer>

class FaultInjector : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(double dropRate READ dropRate WRITE setDropRate NOTIFY dropRateChanged)
    Q_PROPERTY(double corruptRate READ corruptRate WRITE setCorruptRate NOTIFY corruptRateChanged)
    Q_PROPERTY(int responseDelay READ responseDelay WRITE setResponseDelay NOTIFY responseDelayChanged)
    Q_PROPERTY(bool wrongChecksum READ wrongChecksum WRITE setWrongChecksum NOTIFY wrongChecksumChanged)

public:
    explicit FaultInjector(QObject* parent = nullptr);

    bool enabled() const { return m_enabled; }
    void setEnabled(bool v);

    double dropRate() const { return m_dropRate; }
    void setDropRate(double v);

    double corruptRate() const { return m_corruptRate; }
    void setCorruptRate(double v);

    int responseDelay() const { return m_responseDelay; }
    void setResponseDelay(int ms);

    bool wrongChecksum() const { return m_wrongChecksum; }
    void setWrongChecksum(bool v);

    QByteArray process(const QByteArray& response);

signals:
    void enabledChanged(bool);
    void dropRateChanged(double);
    void corruptRateChanged(double);
    void responseDelayChanged(int);
    void wrongChecksumChanged(bool);
    void faultInjected(const QString& description);
    void logMessage(const QString& message);

private:
    QByteArray corruptData(const QByteArray& data);
    QByteArray corruptChecksum(const QByteArray& data);

    bool m_enabled;
    double m_dropRate;
    double m_corruptRate;
    int m_responseDelay;
    bool m_wrongChecksum;
    QRandomGenerator m_rng;
};

#endif // FAULTINJECTOR_H
