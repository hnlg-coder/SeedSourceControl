#include "faultinjector.h"
#include <QThread>

FaultInjector::FaultInjector(QObject* parent)
    : QObject(parent)
    , m_enabled(false)
    , m_dropRate(0)
    , m_corruptRate(0)
    , m_responseDelay(0)
    , m_wrongChecksum(false)
{
}

void FaultInjector::setEnabled(bool v)
{
    if (m_enabled != v) {
        m_enabled = v;
        emit enabledChanged(v);
        emit logMessage(v ? QStringLiteral("FaultInjector: enabled")
                          : QStringLiteral("FaultInjector: disabled"));
    }
}

void FaultInjector::setDropRate(double v)
{
    if (v < 0) v = 0;
    if (v > 1.0) v = 1.0;
    if (m_dropRate != v) {
        m_dropRate = v;
        emit dropRateChanged(v);
    }
}

void FaultInjector::setCorruptRate(double v)
{
    if (v < 0) v = 0;
    if (v > 1.0) v = 1.0;
    if (m_corruptRate != v) {
        m_corruptRate = v;
        emit corruptRateChanged(v);
    }
}

void FaultInjector::setResponseDelay(int ms)
{
    if (ms < 0) ms = 0;
    if (ms > 5000) ms = 5000;
    if (m_responseDelay != ms) {
        m_responseDelay = ms;
        emit responseDelayChanged(ms);
    }
}

void FaultInjector::setWrongChecksum(bool v)
{
    if (m_wrongChecksum != v) {
        m_wrongChecksum = v;
        emit wrongChecksumChanged(v);
    }
}

QByteArray FaultInjector::process(const QByteArray& response)
{
    if (!m_enabled || response.isEmpty()) {
        return response;
    }

    if (m_responseDelay > 0) {
        QThread::msleep(m_responseDelay);
    }

    if (m_dropRate > 0) {
        double roll = m_rng.generateDouble();
        if (roll < m_dropRate) {
            emit faultInjected(QStringLiteral("Frame dropped (rate=%1%)")
                              .arg(m_dropRate * 100, 0, 'f', 0));
            emit logMessage(QStringLiteral("FaultInjector: response frame dropped"));
            return QByteArray();
        }
    }

    QByteArray result = response;

    if (m_corruptRate > 0) {
        double roll = m_rng.generateDouble();
        if (roll < m_corruptRate) {
            result = corruptData(result);
            emit faultInjected(QStringLiteral("Data corrupted (rate=%1%)")
                              .arg(m_corruptRate * 100, 0, 'f', 0));
            emit logMessage(QStringLiteral("FaultInjector: response data corrupted"));
        }
    }

    if (m_wrongChecksum) {
        result = corruptChecksum(result);
        emit faultInjected(QStringLiteral("Checksum corrupted"));
        emit logMessage(QStringLiteral("FaultInjector: response checksum corrupted"));
    }

    return result;
}

QByteArray FaultInjector::corruptData(const QByteArray& data)
{
    if (data.size() < 7) return data;

    QByteArray result = data;
    int dataLen = static_cast<int>(static_cast<quint8>(result[3]));
    if (dataLen > 0) {
        int pos = 4 + m_rng.bounded(dataLen);
        result[pos] = static_cast<char>(result[pos] ^ 0xFF);
    }
    return result;
}

QByteArray FaultInjector::corruptChecksum(const QByteArray& data)
{
    if (data.size() < 3) return data;

    QByteArray result = data;
    int csumPos = result.size() - 2;
    if (csumPos >= 1) {
        result[csumPos] = static_cast<char>(static_cast<quint8>(result[csumPos]) ^ 0xFF);
    }
    return result;
}
