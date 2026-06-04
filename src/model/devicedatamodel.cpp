#include "devicedatamodel.h"
#include <QDebug>

DeviceDataModel::DeviceDataModel(QObject* parent)
    : QObject(parent)
    , m_dataCache(new DataCache())
    , m_lastAlarm(0)
{
    m_currentData.current = 0.0;
    m_currentData.temperature = 25.0;
    m_currentData.power = 0.0;
    m_currentData.status = 0;
    m_currentData.alarm = 0;
    m_currentData.timestamp = QDateTime::currentDateTime();
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

void DeviceDataModel::updateData(double current, double temperature, double power, quint32 status, quint32 alarm)
{
    QWriteLocker locker(&m_dataLock);
    
    m_currentData.current = current;
    m_currentData.temperature = temperature;
    m_currentData.power = power;
    m_currentData.status = status;
    m_currentData.alarm = alarm;
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
    if (data.alarm != m_lastAlarm) {
        m_lastAlarm = data.alarm;
        
        if (data.alarm != 0) {
            QStringList alarmMessages;
            if (data.alarm & 0x01) alarmMessages << "Over current";
            if (data.alarm & 0x02) alarmMessages << "Over temperature";
            if (data.alarm & 0x04) alarmMessages << "Over power";
            if (data.alarm & 0x08) alarmMessages << "Voltage error";
            if (data.alarm & 0x10) alarmMessages << "Communication error";
            if (data.alarm & 0x20) alarmMessages << "Hardware fault";
            
            QString message = alarmMessages.join(", ");
            emit alarmTriggered(data.alarm, message);
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
