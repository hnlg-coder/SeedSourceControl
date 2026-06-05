#ifndef DEVICEDATAMODEL_H
#define DEVICEDATAMODEL_H

#include <QObject>
#include <QDateTime>
#include <QVector>
#include <QReadWriteLock>
#include <QTimer>
#include <QStringList>
#include <atomic>
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
     * @brief 设备状态位域结构体
     *
     * 对应 STATUS 寄存器 [00h] 的位域解析
     */
    struct StatusBits {
        bool idle : 1;          // bit0: 空闲/运行状态 (0=运行, 1=空闲)
        quint8 curStatus : 2;   // bit1-2: 电流状态 (0=关断, 1=启动中, 2=软启动, 3=稳定)
        quint8 tempStatus : 2;  // bit3-4: 温度状态 (0=关断, 1=启动中, 2=软启动, 3=稳定)
        quint8 reserved : 3;    // bit5-7: 保留

        StatusBits() : idle(true), curStatus(0), tempStatus(0), reserved(0) {}

        static StatusBits fromRaw(quint32 raw) {
            StatusBits s;
            s.idle = (raw & 0x01) != 0;
            s.curStatus = (raw >> 1) & 0x03;
            s.tempStatus = (raw >> 3) & 0x03;
            return s;
        }

        QString curStatusText() const {
            switch (curStatus) {
                case 0: return QStringLiteral("关断");
                case 1: return QStringLiteral("启动中");
                case 2: return QStringLiteral("软启动");
                case 3: return QStringLiteral("稳定");
                default: return QStringLiteral("未知");
            }
        }

        QString tempStatusText() const {
            switch (tempStatus) {
                case 0: return QStringLiteral("关断");
                case 1: return QStringLiteral("启动中");
                case 2: return QStringLiteral("软启动");
                case 3: return QStringLiteral("稳定");
                default: return QStringLiteral("未知");
            }
        }
    };

    /**
     * @brief 报警状态位域结构体
     *
     * 对应 ALERT 寄存器 [07h] 的位域解析
     */
    struct AlertBits {
        bool ccCtrl : 1;    // bit0: 恒流控制报警
        bool ccPd : 1;      // bit1: 光电二极管报警
        bool ccAdc : 1;     // bit2: 恒流ADC报警
        bool ccDac : 1;     // bit3: 恒流DAC报警
        bool tcCtrl : 1;    // bit4: 温控报警
        bool tcAdc : 1;     // bit5: 温控ADC报警
        bool tcDac : 1;     // bit6: 温控DAC报警
        bool sysPwr : 1;    // bit7: 系统电源报警

        AlertBits() : ccCtrl(false), ccPd(false), ccAdc(false), ccDac(false),
                      tcCtrl(false), tcAdc(false), tcDac(false), sysPwr(false) {}

        static AlertBits fromRaw(quint32 raw) {
            AlertBits a;
            a.ccCtrl = (raw & 0x01) != 0;
            a.ccPd   = (raw & 0x02) != 0;
            a.ccAdc  = (raw & 0x04) != 0;
            a.ccDac  = (raw & 0x08) != 0;
            a.tcCtrl = (raw & 0x10) != 0;
            a.tcAdc  = (raw & 0x20) != 0;
            a.tcDac  = (raw & 0x40) != 0;
            a.sysPwr = (raw & 0x80) != 0;
            return a;
        }

        bool hasAlert() const {
            return ccCtrl || ccPd || ccAdc || ccDac || tcCtrl || tcAdc || tcDac || sysPwr;
        }

        quint32 toRaw() const {
            return (ccCtrl ? 0x01 : 0) | (ccPd ? 0x02 : 0) | (ccAdc ? 0x04 : 0) | (ccDac ? 0x08 : 0)
                 | (tcCtrl ? 0x10 : 0) | (tcAdc ? 0x20 : 0) | (tcDac ? 0x40 : 0) | (sysPwr ? 0x80 : 0);
        }

        QStringList alertNames() const {
            QStringList names;
            if (ccCtrl) names << QStringLiteral("恒流控制");
            if (ccPd)   names << QStringLiteral("光电二极管");
            if (ccAdc)  names << QStringLiteral("恒流ADC");
            if (ccDac)  names << QStringLiteral("恒流DAC");
            if (tcCtrl) names << QStringLiteral("温控");
            if (tcAdc)  names << QStringLiteral("温控ADC");
            if (tcDac)  names << QStringLiteral("温控DAC");
            if (sysPwr) names << QStringLiteral("系统电源");
            return names;
        }
    };

    /**
     * @brief 配置位域结构体
     *
     * 对应 CONFIG 寄存器 [03h] 的位域解析
     */
    struct ConfigBits {
        bool tcEn : 1;           // bit0: 温度控制使能
        bool ccEn : 1;           // bit1: 恒流控制使能
        bool aeEn : 1;           // bit2: 自动使能
        bool powerSv : 1;        // bit3: 功率保护
        bool curSv : 1;          // bit4: 电流保护
        bool tempSv : 1;         // bit5: 温度保护
        quint8 reserved : 2;     // bit6-7: 保留
        double curTh;            // 电流阈值 (mA) - 来自寄存器高位
        double curSlope;         // 电流斜率 (mW/mA) - 来自寄存器高位
        quint8 baudRate;         // 波特率索引

        ConfigBits() : tcEn(false), ccEn(false), aeEn(false), powerSv(false),
                       curSv(false), tempSv(false), reserved(0),
                       curTh(0), curSlope(0), baudRate(4) {}

        static ConfigBits fromRaw(quint32 raw) {
            ConfigBits c;
            c.tcEn    = (raw & 0x01) != 0;
            c.ccEn    = (raw & 0x02) != 0;
            c.aeEn    = (raw & 0x04) != 0;
            c.powerSv = (raw & 0x08) != 0;
            c.curSv   = (raw & 0x10) != 0;
            c.tempSv  = (raw & 0x20) != 0;
            return c;
        }
    };

    /**
     * @brief 设备信息结构体
     *
     * 对应 DEVINFO 寄存器 [01h] 的位域解析
     */
    struct DevInfo {
        quint16 name;       // 产品名称编码
        quint8 verS;        // 软件版本
        quint8 verH;        // 硬件版本
        quint32 serial;     // 序列号
        double curMax;      // 最大电流 (mA)
        qint8 tempMin;      // 最低温度 (°C)
        qint8 tempMax;      // 最高温度 (°C)

        DevInfo() : name(0), verS(0), verH(0), serial(0), curMax(0), tempMin(0), tempMax(0) {}
    };

    /**
     * @brief 实时数据结构体
     *
     * 存储设备当前的状态和参数，包含协议V1.3全部寄存器数据
     */
    struct RealTimeData {
        // 电流相关
        double curSet;       // 目标电流 (mA)
        double curVal;       // 实际电流 (mA)
        double curPd;        // PD电流 (mA)
        double curTec;       // TEC电流 (mA)

        // 温度相关
        double tempSet;      // 目标温度 (°C)
        double tempVal;      // 实际温度 (°C)

        // 功率相关
        double pwrLas;       // 激光功率 (mW)
        double pwrCc;        // CC功率 (mW)
        double pwrSys;       // 系统功率 (mW)

        // 位域结构
        StatusBits status;   // 设备状态位域
        AlertBits alert;     // 报警状态位域
        ConfigBits config;   // 配置位域
        DevInfo devInfo;     // 设备信息

        // 兼容旧代码（数据库存储/数据表格显示）
        double current;      // 电流值(mA) - 等同于 curVal
        double temperature;  // 温度值(°C) - 等同于 tempVal
        double power;        // 功率值(mW) - 等同于 pwrLas
        quint32 statusRaw;   // 状态原始值（用于数据库存储）
        quint32 alarmRaw;    // 报警原始值（用于数据库存储）

        QDateTime timestamp; // 时间戳

        RealTimeData() :
            curSet(0), curVal(0), curPd(0), curTec(0),
            tempSet(25.0), tempVal(25.0),
            pwrLas(0), pwrCc(0), pwrSys(0),
            current(0), temperature(25.0), power(0),
            statusRaw(0), alarmRaw(0)
        {}
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
     * @brief 从寄存器映射更新实时数据
     * @param regMap 寄存器地址 -> 值的映射
     */
    void updateFromRegisters(const QMap<quint16, QVariant>& regMap);

    /**
     * @brief 更新实时数据（兼容旧接口）
     * @param current 电流值
     * @param temperature 温度值
     * @param power 功率值
     * @param status 设备状态原始值
     * @param alarm 报警状态原始值
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
        DataCache(int maxSize = 10000);
        void add(const RealTimeData& data);
        QVector<RealTimeData> getRecent(int count) const;
        HistoricalData getRange(const QDateTime& from, const QDateTime& to) const;
        void clear();
        void setMaxSize(int size);

    private:
        QVector<RealTimeData> m_data;
        int m_maxSize;
        mutable QReadWriteLock m_lock;
    };

    RealTimeData m_currentData;
    mutable QReadWriteLock m_dataLock;
    QScopedPointer<DataCache> m_dataCache;

    std::atomic<quint32> m_lastAlarm{0};
    void checkAlarms(const RealTimeData& data);
};

#endif // DEVICEDATAMODEL_H