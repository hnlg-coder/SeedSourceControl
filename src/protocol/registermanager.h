#ifndef REGISTERMANAGER_H
#define REGISTERMANAGER_H

#include <QObject>
#include <QMap>
#include <QVariant>
#include <QMutex>

/**
 * @brief 寄存器管理器类
 * 
 * 采用单例模式，管理种子源模块的所有寄存器，提供寄存器读写接口
 */
class RegisterManager : public QObject {
    Q_OBJECT
public:
    /**
     * @brief 获取单例实例
     * @return 寄存器管理器单例指针
     */
    static RegisterManager* instance();
    
    /**
     * @brief 读取寄存器值
     * @param baseAddr 基地址
     * @param offset 偏移地址
     * @param value 输出参数，读取到的寄存器值
     * @return 读取是否成功
     */
    bool readRegister(quint8 baseAddr, quint8 offset, QVariant& value);
    
    /**
     * @brief 写入寄存器值
     * @param baseAddr 基地址
     * @param offset 偏移地址
     * @param value 要写入的值
     * @return 写入是否成功
     */
    bool writeRegister(quint8 baseAddr, quint8 offset, const QVariant& value);
    
    /**
     * @brief 计算寄存器实际地址
     * @param baseAddr 基地址
     * @param offset 偏移地址
     * @return 计算后的寄存器地址 (基地址 << 8 + 偏移地址)
     */
    quint16 calculateRegisterAddress(quint8 baseAddr, quint8 offset) const;
    
    /**
     * @brief 初始化所有寄存器定义
     */
    void initializeRegisters();
    
private:
    /**
     * @brief 私有构造函数（单例模式）
     * @param parent 父对象指针
     */
    explicit RegisterManager(QObject* parent = nullptr);
    
    /**
     * @brief 私有析构函数
     */
    ~RegisterManager();
    
    Q_DISABLE_COPY(RegisterManager)  // 禁用拷贝构造和赋值
    
    /**
     * @brief 寄存器定义结构体
     * 
     * 存储寄存器的完整定义信息
     */
    struct RegisterDefinition {
        QString name;          // 寄存器名称
        quint8 baseAddress;    // 基地址
        quint8 offset;         // 偏移地址
        quint8 size;           // 寄存器大小（字节数）
        QVariant defaultValue; // 默认值
        QVariant minValue;     // 最小值
        QVariant maxValue;     // 最大值
        QString unit;          // 单位
        QString description;   // 描述信息
    };
    
    /**
     * @brief 添加寄存器定义
     * @param reg 寄存器定义
     */
    void addRegister(const RegisterDefinition& reg);
    
    static RegisterManager* m_instance;                    // 单例实例
    QMap<quint16, RegisterDefinition> m_registers;         // 寄存器定义映射
    QMap<quint16, QVariant> m_registerValues;              // 寄存器值映射
    mutable QMutex m_mutex;                                // 互斥锁，保证线程安全
};

#endif // REGISTERMANAGER_H
