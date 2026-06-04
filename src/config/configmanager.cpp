#include "configmanager.h"
#include <QStandardPaths>
#include <QDir>

ConfigManager* ConfigManager::m_instance = nullptr;

ConfigManager* ConfigManager::instance()
{
    static QMutex mutex;
    if (!m_instance) {
        QMutexLocker locker(&mutex);
        if (!m_instance) {
            m_instance = new ConfigManager();
        }
    }
    return m_instance;
}

ConfigManager::ConfigManager(QObject* parent)
    : QObject(parent)
    , m_settings(nullptr)
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configPath);
    m_settings = new QSettings(configPath + "/config.ini", QSettings::IniFormat, this);

    loadSettings();
}

ConfigManager::~ConfigManager()
{
    saveSettings();
}

QString ConfigManager::serialPortName() const
{
    QMutexLocker locker(&m_mutex);
    return m_serialPortName;
}

qint32 ConfigManager::serialBaudRate() const
{
    QMutexLocker locker(&m_mutex);
    return m_serialBaudRate;
}

QSerialPort::DataBits ConfigManager::serialDataBits() const
{
    QMutexLocker locker(&m_mutex);
    return m_serialDataBits;
}

QSerialPort::Parity ConfigManager::serialParity() const
{
    QMutexLocker locker(&m_mutex);
    return m_serialParity;
}

QSerialPort::StopBits ConfigManager::serialStopBits() const
{
    QMutexLocker locker(&m_mutex);
    return m_serialStopBits;
}

QSerialPort::FlowControl ConfigManager::serialFlowControl() const
{
    QMutexLocker locker(&m_mutex);
    return m_serialFlowControl;
}

int ConfigManager::pollInterval() const
{
    QMutexLocker locker(&m_mutex);
    return m_pollInterval;
}

int ConfigManager::timeout() const
{
    QMutexLocker locker(&m_mutex);
    return m_timeout;
}

QString ConfigManager::logFilePath() const
{
    QMutexLocker locker(&m_mutex);
    return m_logFilePath;
}

QString ConfigManager::databasePath() const
{
    QMutexLocker locker(&m_mutex);
    return m_databasePath;
}

void ConfigManager::setSerialPortName(const QString& name)
{
    QMutexLocker locker(&m_mutex);
    m_serialPortName = name;
    saveSettings();
}

void ConfigManager::setSerialBaudRate(qint32 rate)
{
    QMutexLocker locker(&m_mutex);
    m_serialBaudRate = rate;
    saveSettings();
}

void ConfigManager::setSerialDataBits(QSerialPort::DataBits bits)
{
    QMutexLocker locker(&m_mutex);
    m_serialDataBits = bits;
    saveSettings();
}

void ConfigManager::setSerialParity(QSerialPort::Parity parity)
{
    QMutexLocker locker(&m_mutex);
    m_serialParity = parity;
    saveSettings();
}

void ConfigManager::setSerialStopBits(QSerialPort::StopBits bits)
{
    QMutexLocker locker(&m_mutex);
    m_serialStopBits = bits;
    saveSettings();
}

void ConfigManager::setSerialFlowControl(QSerialPort::FlowControl flow)
{
    QMutexLocker locker(&m_mutex);
    m_serialFlowControl = flow;
    saveSettings();
}

void ConfigManager::setPollInterval(int ms)
{
    QMutexLocker locker(&m_mutex);
    m_pollInterval = ms;
    saveSettings();
}

void ConfigManager::setTimeout(int ms)
{
    QMutexLocker locker(&m_mutex);
    m_timeout = ms;
    saveSettings();
}

void ConfigManager::setLogFilePath(const QString& path)
{
    QMutexLocker locker(&m_mutex);
    m_logFilePath = path;
    saveSettings();
}

void ConfigManager::setDatabasePath(const QString& path)
{
    QMutexLocker locker(&m_mutex);
    m_databasePath = path;
    saveSettings();
}

void ConfigManager::loadSettings()
{
    QMutexLocker locker(&m_mutex);
    m_serialPortName = m_settings->value("Serial/Port", "").toString();
    m_serialBaudRate = m_settings->value("Serial/BaudRate", 115200).toInt();
    m_serialDataBits = static_cast<QSerialPort::DataBits>(m_settings->value("Serial/DataBits", QSerialPort::Data8).toInt());
    m_serialParity = static_cast<QSerialPort::Parity>(m_settings->value("Serial/Parity", QSerialPort::NoParity).toInt());
    m_serialStopBits = static_cast<QSerialPort::StopBits>(m_settings->value("Serial/StopBits", QSerialPort::OneStop).toInt());
    m_serialFlowControl = static_cast<QSerialPort::FlowControl>(m_settings->value("Serial/FlowControl", QSerialPort::NoFlowControl).toInt());

    m_pollInterval = m_settings->value("General/PollInterval", 100).toInt();
    m_timeout = m_settings->value("General/Timeout", 2000).toInt();

    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataPath);
    m_logFilePath = m_settings->value("Paths/LogFile", dataPath + "/app.log").toString();
    m_databasePath = m_settings->value("Paths/Database", dataPath + "/data.db").toString();
}

void ConfigManager::saveSettings()
{
    QMutexLocker locker(&m_mutex);
    m_settings->setValue("Serial/Port", m_serialPortName);
    m_settings->setValue("Serial/BaudRate", m_serialBaudRate);
    m_settings->setValue("Serial/DataBits", static_cast<int>(m_serialDataBits));
    m_settings->setValue("Serial/Parity", static_cast<int>(m_serialParity));
    m_settings->setValue("Serial/StopBits", static_cast<int>(m_serialStopBits));
    m_settings->setValue("Serial/FlowControl", static_cast<int>(m_serialFlowControl));

    m_settings->setValue("General/PollInterval", m_pollInterval);
    m_settings->setValue("General/Timeout", m_timeout);

    m_settings->setValue("Paths/LogFile", m_logFilePath);
    m_settings->setValue("Paths/Database", m_databasePath);

    m_settings->sync();
}
