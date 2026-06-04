#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QSettings>
#include <QSerialPort>
#include <QRecursiveMutex>

/**
 * @brief 配置管理类
 * 
 * 采用单例模式，管理应用程序的全局配置信息
 */
class ConfigManager : public QObject {
    Q_OBJECT
public:
    /**
     * @brief 获取单例实例
     * @return 配置管理器指针
     */
    static ConfigManager* instance();
    
    // 串口配置获取方法
    QString serialPortName() const;                // 串口名称
    qint32 serialBaudRate() const;              // 波特率
    QSerialPort::DataBits serialDataBits() const; // 数据位
    QSerialPort::Parity serialParity() const;   // 校验位
    QSerialPort::StopBits serialStopBits() const; // 停止位
    QSerialPort::FlowControl serialFlowControl() const; // 流控制
    
    // 其他配置获取方法
    int pollInterval() const;                     // 轮询间隔
    int timeout() const;                          // 超时时间
    
    QString logFilePath() const;                    // 日志文件路径
    QString databasePath() const;                 // 数据库文件路径
    
public slots:
    // 串口配置设置方法
    void setSerialPortName(const QString& name);
    void setSerialBaudRate(qint32 rate);
    void setSerialDataBits(QSerialPort::DataBits bits);
    void setSerialParity(QSerialPort::Parity parity);
    void setSerialStopBits(QSerialPort::StopBits bits);
    void setSerialFlowControl(QSerialPort::FlowControl flow);
    
    // 其他配置设置方法
    void setPollInterval(int ms);
    void setTimeout(int ms);
    void setLogFilePath(const QString& path);
    void setDatabasePath(const QString& path);
    
    /**
     * @brief 加载配置
     */
    void loadSettings();
    
    /**
     * @brief 保存配置
     */
    void saveSettings();
    
private:
    /**
     * @brief 私有构造函数
     * @param parent 父对象指针
     */
    explicit ConfigManager(QObject* parent = nullptr);
    
    /**
     * @brief 私有析构函数
     */
    ~ConfigManager();
    
    Q_DISABLE_COPY(ConfigManager) // 禁止拷贝
    
    static ConfigManager* m_instance;    // 单例实例
    QSettings* m_settings;                  // 设置对象
    mutable QRecursiveMutex m_mutex;                // 配置互斥锁 (递归模式)
    
    // 串口配置成员变量
    QString m_serialPortName;
    qint32 m_serialBaudRate;
    QSerialPort::DataBits m_serialDataBits;
    QSerialPort::Parity m_serialParity;
    QSerialPort::StopBits m_serialStopBits;
    QSerialPort::FlowControl m_serialFlowControl;
    
    // 其他配置成员变量
    int m_pollInterval;
    int m_timeout;
    QString m_logFilePath;
    QString m_databasePath;
};

#endif // CONFIGMANAGER_H
