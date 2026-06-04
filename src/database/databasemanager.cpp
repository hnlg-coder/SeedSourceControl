#include "databasemanager.h"
#include <QSqlError>
#include <QFile>
#include <QTextStream>
#include <QDebug>

DatabaseManager* DatabaseManager::m_instance = nullptr;

DatabaseManager* DatabaseManager::instance()
{
    static QMutex mutex;
    if (!m_instance) {
        QMutexLocker locker(&mutex);
        if (!m_instance) {
            m_instance = new DatabaseManager();
        }
    }
    return m_instance;
}

DatabaseManager::DatabaseManager(QObject* parent)
    : QObject(parent)
{
}

DatabaseManager::~DatabaseManager()
{
    close();
}

bool DatabaseManager::initialize(const QString& databasePath)
{
    if (m_database.isOpen()) {
        return true;
    }
    
    m_database = QSqlDatabase::addDatabase("QSQLITE");
    m_database.setDatabaseName(databasePath);
    
    if (!m_database.open()) {
        qCritical() << "Failed to open database:" << m_database.lastError().text();
        return false;
    }
    
    return createTables();
}

void DatabaseManager::close()
{
    if (m_database.isOpen()) {
        m_database.close();
    }
}

bool DatabaseManager::createTables()
{
    QSqlQuery query;
    
    QString createDataTable = R"(
        CREATE TABLE IF NOT EXISTS data (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp DATETIME NOT NULL,
            current REAL NOT NULL,
            temperature REAL NOT NULL,
            power REAL NOT NULL,
            status INTEGER NOT NULL,
            alarm INTEGER NOT NULL
        )
    )";
    
    if (!query.exec(createDataTable)) {
        qCritical() << "Failed to create data table:" << query.lastError().text();
        return false;
    }
    
    QString createAlarmTable = R"(
        CREATE TABLE IF NOT EXISTS alarms (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp DATETIME NOT NULL,
            code INTEGER NOT NULL,
            message TEXT NOT NULL
        )
    )";
    
    if (!query.exec(createAlarmTable)) {
        qCritical() << "Failed to create alarm table:" << query.lastError().text();
        return false;
    }
    
    QString createIndex = "CREATE INDEX IF NOT EXISTS idx_data_timestamp ON data(timestamp)";
    if (!query.exec(createIndex)) {
        qCritical() << "Failed to create index:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool DatabaseManager::saveData(const DeviceDataModel::RealTimeData& data)
{
    QSqlQuery query;
    query.prepare(R"(
        INSERT INTO data (timestamp, current, temperature, power, status, alarm)
        VALUES (?, ?, ?, ?, ?, ?)
    )");
    
    query.addBindValue(data.timestamp.toString(Qt::ISODate));
    query.addBindValue(data.current);
    query.addBindValue(data.temperature);
    query.addBindValue(data.power);
    query.addBindValue(data.status);
    query.addBindValue(data.alarm);
    
    if (!query.exec()) {
        qCritical() << "Failed to save data:" << query.lastError().text();
        return false;
    }
    
    return true;
}

QVector<DeviceDataModel::RealTimeData> DatabaseManager::loadData(const QDateTime& startTime, const QDateTime& endTime)
{
    QSqlQuery query;
    query.prepare(R"(
        SELECT timestamp, current, temperature, power, status, alarm
        FROM data
        WHERE timestamp BETWEEN ? AND ?
        ORDER BY timestamp
    )");
    
    query.addBindValue(startTime.toString(Qt::ISODate));
    query.addBindValue(endTime.toString(Qt::ISODate));
    
    QVector<DeviceDataModel::RealTimeData> result;
    
    if (query.exec()) {
        while (query.next()) {
            DeviceDataModel::RealTimeData data;
            data.timestamp = QDateTime::fromString(query.value(0).toString(), Qt::ISODate);
            data.current = query.value(1).toDouble();
            data.temperature = query.value(2).toDouble();
            data.power = query.value(3).toDouble();
            data.status = query.value(4).toUInt();
            data.alarm = query.value(5).toUInt();
            result.append(data);
        }
    } else {
        qCritical() << "Failed to load data:" << query.lastError().text();
    }
    
    return result;
}

bool DatabaseManager::saveAlarm(quint32 alarmCode, const QString& message)
{
    QSqlQuery query;
    query.prepare(R"(
        INSERT INTO alarms (timestamp, code, message)
        VALUES (?, ?, ?)
    )");
    
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    query.addBindValue(alarmCode);
    query.addBindValue(message);
    
    if (!query.exec()) {
        qCritical() << "Failed to save alarm:" << query.lastError().text();
        return false;
    }
    
    return true;
}

QVector<QPair<QDateTime, QString>> DatabaseManager::loadAlarms(const QDateTime& startTime, const QDateTime& endTime)
{
    QSqlQuery query;
    query.prepare(R"(
        SELECT timestamp, message
        FROM alarms
        WHERE timestamp BETWEEN ? AND ?
        ORDER BY timestamp
    )");
    
    query.addBindValue(startTime.toString(Qt::ISODate));
    query.addBindValue(endTime.toString(Qt::ISODate));
    
    QVector<QPair<QDateTime, QString>> result;
    
    if (query.exec()) {
        while (query.next()) {
            QDateTime timestamp = QDateTime::fromString(query.value(0).toString(), Qt::ISODate);
            QString message = query.value(1).toString();
            result.append(qMakePair(timestamp, message));
        }
    } else {
        qCritical() << "Failed to load alarms:" << query.lastError().text();
    }
    
    return result;
}

bool DatabaseManager::exportToCsv(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCritical() << "Failed to open file for export:" << filePath;
        return false;
    }
    
    QTextStream out(&file);
    out << "Timestamp,Current,Temperature,Power,Status,Alarm\n";
    
    QSqlQuery query("SELECT timestamp, current, temperature, power, status, alarm FROM data ORDER BY timestamp");
    
    if (query.exec()) {
        while (query.next()) {
            out << query.value(0).toString() << ","
                << query.value(1).toString() << ","
                << query.value(2).toString() << ","
                << query.value(3).toString() << ","
                << query.value(4).toString() << ","
                << query.value(5).toString() << "\n";
        }
    }
    
    file.close();
    return true;
}

void DatabaseManager::clearDatabase()
{
    QSqlQuery query;
    query.exec("DELETE FROM data");
    query.exec("DELETE FROM alarms");
}
