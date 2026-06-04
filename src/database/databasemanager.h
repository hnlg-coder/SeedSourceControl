#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDateTime>
#include "../model/devicedatamodel.h"

class DatabaseManager : public QObject {
    Q_OBJECT
public:
    static DatabaseManager* instance();
    
    bool initialize(const QString& databasePath);
    void close();
    
    bool saveData(const DeviceDataModel::RealTimeData& data);
    QVector<DeviceDataModel::RealTimeData> loadData(const QDateTime& startTime, const QDateTime& endTime);
    
    bool saveAlarm(quint32 alarmCode, const QString& message);
    QVector<QPair<QDateTime, QString>> loadAlarms(const QDateTime& startTime, const QDateTime& endTime);
    
    bool exportToCsv(const QString& filePath);
    
public slots:
    void clearDatabase();
    
private:
    explicit DatabaseManager(QObject* parent = nullptr);
    ~DatabaseManager();
    Q_DISABLE_COPY(DatabaseManager)
    
    bool createTables();
    
    static DatabaseManager* m_instance;
    QSqlDatabase m_database;
};

#endif // DATABASEMANAGER_H
