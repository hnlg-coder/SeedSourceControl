#include "devicedatamodel.h"
#include <QDebug>

DeviceDataModel::DeviceDataModel(QObject* parent)
    : QObject(parent)
    , m_dataCache(new DataCache())
    , m_lastAlarm(0)
{
}

DeviceDataModel::~DeviceDataModel()
{
}

DeviceDataModel::RealTimeData DeviceDataModel::currentData() const
{
    QReadLocker locker(&m_dataLock);
    return m_currentData;
}

DeviceDataModel::HistoricalData DeviceDataModel::historicalData(const QDateTime& from, const QDateTime& to) const
{
    return m_dataCache->getRange(from, to);
}

QVector<DeviceDataModel::RealTimeData> DeviceDataModel::recentData(int count) const
{
    return m_dataCache->getRecent(count);
}

void DeviceDataModel::setDataCacheSize(int size)
{
    m_dataCache->setMaxSize(size);
}

void DeviceDataModel::updateFromRegisters(const QMap<quint16, QVariant>& regMap)
{
    QWriteLocker locker(&m_dataLock);

    // 寄存器地址计算: baseAddr << 8 + offset
    auto getReg = [&](quint8 base, quint8 offset) -> quint32 {
        quint16 addr = (static_cast<quint16>(base) << 8) | offset;
        return regMap.value(addr, 0).toUInt();
    };

    // STATUS 寄存器 (0x00)
    quint32 statusRaw = getReg(0x00, 0x00);
    m_currentData.status = StatusBits::fromRaw(statusRaw);
    m_currentData.statusRaw = statusRaw;

    // ALERT 寄存器 (0x01)
    quint32 alarmRaw = getReg(0x01, 0x00);
    m_currentData.alert = AlertBits::fromRaw(alarmRaw);
    m_currentData.alarmRaw = alarmRaw;

    // CUR 寄存器 (0x02) - 电流值
    m_currentData.curSet = static_cast<double>(getReg(0x02, 0x00)) / 100.0;  // 假设单位为0.01mA
    m_currentData.curVal = m_currentData.curSet;  // 暂用设定值

    // TEMP 寄存器 (0x03) - 温度值
    m_currentData.tempSet = static_cast<double>(getReg(0x03, 0x00)) / 1000.0; // 假设单位为0.001°C
    m_currentData.tempVal = m_currentData.tempSet;

    // POWER 寄存器 (0x04) - 功率值
    m_currentData.pwrLas = static_cast<double>(getReg(0x04, 0x00)) / 100.0;   // 假设单位为0.01mW

    // CONFIG 寄存器 (0x05)
    m_currentData.config = ConfigBits::fromRaw(getReg(0x05, 0x00));

    // DEVINFO 寄存器 (0x07)
    quint32 devInfoRaw = getReg(0x07, 0x00);
    m_currentData.devInfo.name   = (devInfoRaw >> 16) & 0xFFFF;
    m_currentData.devInfo.verS   = (devInfoRaw >> 8) & 0xFF;
    m_currentData.devInfo.verH   = devInfoRaw & 0xFF;

    // 兼容旧字段
    m_currentData.current = m_currentData.curVal;
    m_currentData.temperature = m_currentData.tempVal;
    m_currentData.power = m_currentData.pwrLas;

    m_currentData.timestamp = QDateTime::currentDateTime();

    m_dataCache->add(m_currentData);

    RealTimeData data = m_currentData;
    locker.unlock();

    emit dataUpdated(data);
    checkAlarms(data);
}

void DeviceDataModel::updateData(double current, double temperature, double power, quint32 status, quint32 alarm)
{
    QWriteLocker locker(&m_dataLock);

    m_currentData.current = current;
    m_currentData.temperature = temperature;
    m_currentData.power = power;
    m_currentData.curVal = current;
    m_currentData.tempVal = temperature;
    m_currentData.pwrLas = power;

    // 设定值兜底：当设备未返回设定值时使用实际值
    if (m_currentData.curSet == 0) m_currentData.curSet = current;
    if (m_currentData.tempSet == 0) m_currentData.tempSet = temperature;
    m_currentData.status = StatusBits::fromRaw(status);
    m_currentData.alert = AlertBits::fromRaw(alarm);
    m_currentData.statusRaw = status;
    m_currentData.alarmRaw = alarm;
    m_currentData.timestamp = QDateTime::currentDateTime();

    m_dataCache->add(m_currentData);

    RealTimeData data = m_currentData;
    locker.unlock();

    emit dataUpdated(data);
    checkAlarms(data);
}

void DeviceDataModel::updateRegister(quint16 address, const QVariant& value)
{
    emit registerUpdated(address, value);
}

void DeviceDataModel::clearCache()
{
    m_dataCache->clear();
}

void DeviceDataModel::checkAlarms(const RealTimeData& data)
{
    quint32 rawAlarm = data.alert.toRaw();
    if (rawAlarm != m_lastAlarm) {
        m_lastAlarm = rawAlarm;

        if (data.alert.hasAlert()) {
            QStringList names = data.alert.alertNames();
            QString message = names.join(", ");
            emit alarmTriggered(rawAlarm, message);
        }
    }
}

DeviceDataModel::DataCache::DataCache(int maxSize)
    : m_maxSize(maxSize)
{
}

void DeviceDataModel::DataCache::add(const RealTimeData& data)
{
    QWriteLocker locker(&m_lock);
    m_data.append(data);

    while (m_data.size() > m_maxSize) {
        m_data.removeFirst();
    }
}

QVector<DeviceDataModel::RealTimeData> DeviceDataModel::DataCache::getRecent(int count) const
{
    QReadLocker locker(&m_lock);
    int start = qMax(0, m_data.size() - count);
    return m_data.mid(start);
}

DeviceDataModel::HistoricalData DeviceDataModel::DataCache::getRange(const QDateTime& from, const QDateTime& to) const
{
    QReadLocker locker(&m_lock);
    HistoricalData result;

    for (const auto& data : m_data) {
        if (data.timestamp >= from && data.timestamp <= to) {
            result.currents.append(data.current);
            result.temperatures.append(data.temperature);
            result.powers.append(data.power);
            result.timestamps.append(data.timestamp);
        }
    }

    return result;
}

void DeviceDataModel::DataCache::clear()
{
    QWriteLocker locker(&m_lock);
    m_data.clear();
}

void DeviceDataModel::DataCache::setMaxSize(int size)
{
    QWriteLocker locker(&m_lock);
    m_maxSize = size;
    while (m_data.size() > m_maxSize) {
        m_data.removeFirst();
    }
}