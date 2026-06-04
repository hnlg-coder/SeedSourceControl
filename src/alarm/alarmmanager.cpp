#include "alarmmanager.h"
#include "../config/configmanager.h"
#include <QSettings>
#include <QDebug>

AlarmManager* AlarmManager::m_instance = nullptr;

AlarmManager* AlarmManager::instance()
{
    static QMutex mutex;
    if (!m_instance) {
        QMutexLocker locker(&mutex);
        if (!m_instance) {
            m_instance = new AlarmManager();
        }
    }
    return m_instance;
}

AlarmManager::AlarmManager(QObject* parent)
    : QObject(parent)
    , m_activeAlarms(0)
{
    resetThresholds();
    loadThresholds();
}

AlarmManager::~AlarmManager()
{
    saveThresholds();
}

AlarmManager::AlarmThreshold AlarmManager::currentThreshold() const
{
    QReadLocker locker(&m_thresholdLock);
    return m_currentThreshold;
}

AlarmManager::AlarmThreshold AlarmManager::temperatureThreshold() const
{
    QReadLocker locker(&m_thresholdLock);
    return m_temperatureThreshold;
}

AlarmManager::AlarmThreshold AlarmManager::powerThreshold() const
{
    QReadLocker locker(&m_thresholdLock);
    return m_powerThreshold;
}

void AlarmManager::setCurrentThreshold(const AlarmThreshold& threshold)
{
    {
        QWriteLocker locker(&m_thresholdLock);
        m_currentThreshold = threshold;
    }
    saveThresholds();
}

void AlarmManager::setTemperatureThreshold(const AlarmThreshold& threshold)
{
    {
        QWriteLocker locker(&m_thresholdLock);
        m_temperatureThreshold = threshold;
    }
    saveThresholds();
}

void AlarmManager::setPowerThreshold(const AlarmThreshold& threshold)
{
    {
        QWriteLocker locker(&m_thresholdLock);
        m_powerThreshold = threshold;
    }
    saveThresholds();
}

quint32 AlarmManager::checkAlarms(double current, double temperature, double power)
{
    AlarmThreshold currentThresh, tempThresh, powerThresh;
    {
        QReadLocker locker(&m_thresholdLock);
        currentThresh = m_currentThreshold;
        tempThresh = m_temperatureThreshold;
        powerThresh = m_powerThreshold;
    }

    quint32 alarms = 0;
    QStringList alarmMessages;

    if (currentThresh.enabled) {
        if (current > currentThresh.maxValue) {
            alarms |= OverCurrent;
            alarmMessages << QString("Over current: %1 mA (max: %2 mA)").arg(current).arg(currentThresh.maxValue);
        }
        if (current < currentThresh.minValue) {
            alarms |= OverCurrent;
            alarmMessages << QString("Under current: %1 mA (min: %2 mA)").arg(current).arg(currentThresh.minValue);
        }
    }

    if (tempThresh.enabled) {
        if (temperature > tempThresh.maxValue) {
            alarms |= OverTemperature;
            alarmMessages << QString("Over temperature: %1 °C (max: %2 °C)").arg(temperature).arg(tempThresh.maxValue);
        }
        if (temperature < tempThresh.minValue) {
            alarms |= OverTemperature;
            alarmMessages << QString("Under temperature: %1 °C (min: %2 °C)").arg(temperature).arg(tempThresh.minValue);
        }
    }

    if (powerThresh.enabled) {
        if (power > powerThresh.maxValue) {
            alarms |= OverPower;
            alarmMessages << QString("Over power: %1 mW (max: %2 mW)").arg(power).arg(powerThresh.maxValue);
        }
        if (power < powerThresh.minValue) {
            alarms |= OverPower;
            alarmMessages << QString("Under power: %1 mW (min: %2 mW)").arg(power).arg(powerThresh.minValue);
        }
    }

    if (alarms != m_activeAlarms && !alarmMessages.isEmpty()) {
        m_activeAlarms = alarms;
        emit alarmTriggered(alarms, alarmMessages.join("; "));
    } else if (alarms == 0 && m_activeAlarms != 0) {
        m_activeAlarms = 0;
        emit alarmTriggered(0, "All alarms cleared");
    }

    return alarms;
}

void AlarmManager::loadThresholds()
{
    ConfigManager* config = ConfigManager::instance();
    QString logPath = config->logFilePath();
    QFileInfo fi(logPath);
    QString configPath = fi.absolutePath() + "/alarm_config.ini";

    QSettings settings(configPath, QSettings::IniFormat);

    {
        QWriteLocker locker(&m_thresholdLock);
        m_currentThreshold.minValue = settings.value("Current/Min", 0.0).toDouble();
        m_currentThreshold.maxValue = settings.value("Current/Max", 1000.0).toDouble();
        m_currentThreshold.enabled = settings.value("Current/Enabled", false).toBool();

        m_temperatureThreshold.minValue = settings.value("Temperature/Min", -40.0).toDouble();
        m_temperatureThreshold.maxValue = settings.value("Temperature/Max", 85.0).toDouble();
        m_temperatureThreshold.enabled = settings.value("Temperature/Enabled", false).toBool();

        m_powerThreshold.minValue = settings.value("Power/Min", 0.0).toDouble();
        m_powerThreshold.maxValue = settings.value("Power/Max", 10000.0).toDouble();
        m_powerThreshold.enabled = settings.value("Power/Enabled", false).toBool();
    }
}

void AlarmManager::saveThresholds()
{
    ConfigManager* config = ConfigManager::instance();
    QString logPath = config->logFilePath();
    QFileInfo fi(logPath);
    QString configPath = fi.absolutePath() + "/alarm_config.ini";

    QSettings settings(configPath, QSettings::IniFormat);

    {
        QReadLocker locker(&m_thresholdLock);
        settings.setValue("Current/Min", m_currentThreshold.minValue);
        settings.setValue("Current/Max", m_currentThreshold.maxValue);
        settings.setValue("Current/Enabled", m_currentThreshold.enabled);

        settings.setValue("Temperature/Min", m_temperatureThreshold.minValue);
        settings.setValue("Temperature/Max", m_temperatureThreshold.maxValue);
        settings.setValue("Temperature/Enabled", m_temperatureThreshold.enabled);

        settings.setValue("Power/Min", m_powerThreshold.minValue);
        settings.setValue("Power/Max", m_powerThreshold.maxValue);
        settings.setValue("Power/Enabled", m_powerThreshold.enabled);
    }

    settings.sync();
}

void AlarmManager::resetThresholds()
{
    QWriteLocker locker(&m_thresholdLock);
    m_currentThreshold.minValue = 0.0;
    m_currentThreshold.maxValue = 1000.0;
    m_currentThreshold.enabled = false;

    m_temperatureThreshold.minValue = -40.0;
    m_temperatureThreshold.maxValue = 85.0;
    m_temperatureThreshold.enabled = false;

    m_powerThreshold.minValue = 0.0;
    m_powerThreshold.maxValue = 10000.0;
    m_powerThreshold.enabled = false;
}
