#ifndef DEVICEDATAMODEL_H
#define DEVICEDATAMODEL_H

#include <QObject>
#include <QDateTime>
#include <QVector>
#include <QReadWriteLock>
#include <QTimer>
#include "../protocol/registermanager.h"

/**
 * @brief 设备数据模型类
 * 
 * 采用观察者模式，管理设备的实时数据和历史数据
 */
class DeviceDataModel : public QObject {
    Q_OBJECT
public:
    /**
     * @brief 实时数据结构体
     * 
     * 存储设备当前的状态和参数
     */
    struct RealTimeData {
        double current;       // 电流值(mA)
        double temperature;   // 温度值(°C)
        double power;         // 功率值(mW)
        quint32 status;       // 设备状态
        quint32 alarm;        // 报警状态
        QDateTime timestamp;  // 时间戳
    };
    
    /**
     * @brief 历史数据结构体
     * 
     * 存储一定时间范围内的历史数据
     */
    struct HistoricalData {
        QVector<double> currents;      // 电流数组
        QVector<double> temperatures;  // 温度数组
        QVector<double> powers;        // 功率数组
        QVector<QDateTime> timestamps; // 时间戳数组
    };
    
    /**
     * @brief 构造函数
     * @param parent 父对象指针
     */
    explicit DeviceDataModel(QObject* parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~DeviceDataModel();
    
    /**
     * @brief 获取当前实时数据
     * @return 实时数据
     */
    RealTimeData currentData() const;
    
    /**
     * @brief 获取指定时间范围的历史数据
     * @param from 开始时间
     * @param to 结束时间
     * @return 历史数据
     */
    HistoricalData historicalData(const QDateTime& from, const QDateTime& to) const;
    
    /**
     * @brief 获取最近的指定数量的数据
     * @param count 数据数量
     * @return 数据数组
     */
    QVector<RealTimeData> recentData(int count) const;
    
    /**
     * @brief 设置数据缓存大小
     * @param size 缓存大小
     */
    void setDataCacheSize(int size);
    
public slots:
    /**
     * @brief 更新实时数据
     * @param current 电流值
     * @param temperature 温度值
     * @param power 功率值
     * @param status 设备状态
     * @param alarm 报警状态
     */
    void updateData(double current, double temperature, double power, quint32 status, quint32 alarm);
    
    /**
     * @brief 更新寄存器值
     * @param address 寄存器地址
     * @param value 寄存器值
     */
    void updateRegister(quint16 address, const QVariant& value);
    
    /**
     * @brief 清空缓存
     */
    void clearCache();
    
signals:
    /**
     * @brief 数据更新信号
     * @param data 更新后的数据
     */
    void dataUpdated(const RealTimeData& data);
    
    /**
     * @brief 寄存器更新信号
     * @param address 寄存器地址
     * @param value 寄存器值
     */
    void registerUpdated(quint16 address, const QVariant& value);
    
    /**
     * @brief 报警触发信号
     * @param alarmCode 报警代码
     * @param message 报警信息
     */
    void alarmTriggered(quint32 alarmCode, const QString& message);
    
private:
    /**
     * @brief 数据缓存类
     * 
     * 线程安全的数据缓存，支持添加、查询和清除操作
     */
    class DataCache {
    public:
        /**
         * @brief 构造函数
         * @param maxSize 最大缓存大小
         */
        DataCache(int maxSize = 10000);
        
        /**
         * @brief 添加数据
         * @param data 要添加的数据
         */
        void add(const RealTimeData& data);
        
        /**
         * @brief 获取最近的数据
         * @param count 数据数量
         * @return 数据数组
         */
        QVector<RealTimeData> getRecent(int count) const;
        
        /**
         * @brief 获取指定时间范围的数据
         * @param from 开始时间
         * @param to 结束时间
         * @return 历史数据
         */
        HistoricalData getRange(const QDateTime& from, const QDateTime& to) const;
        
        /**
         * @brief 清空缓存
         */
        void clear();
        
        /**
         * @brief 设置最大缓存大小
         * @param size 缓存大小
         */
        void setMaxSize(int size);
        
    private:
        QVector<RealTimeData> m_data;       // 数据数组
        int m_maxSize;                     // 最大缓存大小
        mutable QReadWriteLock m_lock;     // 读写锁，保证线程安全
    };
    
    RealTimeData m_currentData;             // 当前实时数据
    mutable QReadWriteLock m_dataLock;      // 数据读写锁
    QScopedPointer<DataCache> m_dataCache;  // 数据缓存
    
    quint32 m_lastAlarm;                    // 上次报警状态
    /**
     * @brief 检查报警
     * @param data 当前数据
     */
    void checkAlarms(const RealTimeData& data);
};

#endif // DEVICEDATAMODEL_H
