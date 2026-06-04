#ifndef ALARMMANAGER_H
#define ALARMMANAGER_H

#include <QObject>
#include <QMap>
#include <QVariant>
#include <QReadWriteLock>
#include <QFileInfo>

class AlarmManager : public QObject {
    Q_OBJECT
public:
    static AlarmManager* instance();
    
    struct AlarmThreshold {
        double minValue;
        double maxValue;
        bool enabled;
    };
    
    enum AlarmCode {
        OverCurrent = 0x01,
        OverTemperature = 0x02,
        OverPower = 0x04,
        VoltageError = 0x08,
        CommunicationError = 0x10,
        HardwareFault = 0x20
    };
    
    AlarmThreshold currentThreshold() const;
    AlarmThreshold temperatureThreshold() const;
    AlarmThreshold powerThreshold() const;
    
    void setCurrentThreshold(const AlarmThreshold& threshold);
    void setTemperatureThreshold(const AlarmThreshold& threshold);
    void setPowerThreshold(const AlarmThreshold& threshold);
    
    quint32 checkAlarms(double current, double temperature, double power);
    
public slots:
    void loadThresholds();
    void saveThresholds();
    void resetThresholds();
    
signals:
    void alarmTriggered(quint32 alarmCode, const QString& message);
    
private:
    explicit AlarmManager(QObject* parent = nullptr);
    ~AlarmManager();
    Q_DISABLE_COPY(AlarmManager)
    
    static AlarmManager* m_instance;

    mutable QReadWriteLock m_thresholdLock;  // 阈值读写锁

    AlarmThreshold m_currentThreshold;
    AlarmThreshold m_temperatureThreshold;
    AlarmThreshold m_powerThreshold;
    
    quint32 m_activeAlarms;
};

#endif // ALARMMANAGER_H
