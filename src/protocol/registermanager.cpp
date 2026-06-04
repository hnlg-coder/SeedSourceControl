#include "registermanager.h"
#include <QDebug>

RegisterManager* RegisterManager::m_instance = nullptr;  // 单例实例初始化

/**
 * @brief 获取寄存器管理器单例
 * @return 寄存器管理器指针
 * 
 * 使用双重检查锁定模式保证线程安全
 */
RegisterManager* RegisterManager::instance()
{
    static QMutex mutex;
    if (!m_instance) {
        QMutexLocker locker(&mutex);
        if (!m_instance) {
            m_instance = new RegisterManager();
            m_instance->initializeRegisters();  // 初始化所有寄存器
        }
    }
    return m_instance;
}

/**
 * @brief 构造函数
 * @param parent 父对象指针
 */
RegisterManager::RegisterManager(QObject* parent)
    : QObject(parent)
{
}

/**
 * @brief 析构函数
 */
RegisterManager::~RegisterManager()
{
}

/**
 * @brief 初始化所有寄存器定义
 * 
 * 根据《种子源模块通信协议V1.3》定义所有寄存器
 */
void RegisterManager::initializeRegisters()
{
    QMutexLocker locker(&m_mutex);
    
    RegisterDefinition reg;
    
    // 1. 设备状态寄存器
    reg.name = "STATUS";
    reg.baseAddress = 0x00;
    reg.offset = 0x00;
    reg.size = 4;
    reg.defaultValue = 0x00000000;
    reg.minValue = 0;
    reg.maxValue = 0xFFFFFFFF;
    reg.unit = "";
    reg.description = "设备状态寄存器";
    addRegister(reg);
    
    // 2. 报警状态寄存器
    reg.name = "ALERT";
    reg.baseAddress = 0x01;
    reg.offset = 0x00;
    reg.size = 4;
    reg.defaultValue = 0x00000000;
    reg.minValue = 0;
    reg.maxValue = 0xFFFFFFFF;
    reg.unit = "";
    reg.description = "报警状态寄存器";
    addRegister(reg);
    
    // 3. 电流控制寄存器
    reg.name = "CUR";
    reg.baseAddress = 0x02;
    reg.offset = 0x00;
    reg.size = 4;
    reg.defaultValue = 0;
    reg.minValue = 0;
    reg.maxValue = 1000;
    reg.unit = "mA";
    reg.description = "电流控制寄存器";
    addRegister(reg);
    
    // 4. 温度控制寄存器
    reg.name = "TEMP";
    reg.baseAddress = 0x03;
    reg.offset = 0x00;
    reg.size = 4;
    reg.defaultValue = 25;
    reg.minValue = -40;
    reg.maxValue = 125;
    reg.unit = "°C";
    reg.description = "温度控制寄存器";
    addRegister(reg);
    
    // 5. 功率控制寄存器
    reg.name = "POWER";
    reg.baseAddress = 0x04;
    reg.offset = 0x00;
    reg.size = 4;
    reg.defaultValue = 0;
    reg.minValue = 0;
    reg.maxValue = 10000;
    reg.unit = "mW";
    reg.description = "功率控制寄存器";
    addRegister(reg);
    
    // 6. 配置寄存器
    reg.name = "CONFIG";
    reg.baseAddress = 0x05;
    reg.offset = 0x00;
    reg.size = 4;
    reg.defaultValue = 0x00000000;
    reg.minValue = 0;
    reg.maxValue = 0xFFFFFFFF;
    reg.unit = "";
    reg.description = "配置寄存器";
    addRegister(reg);
    
    // 7. 系统控制寄存器
    reg.name = "SYSTEM";
    reg.baseAddress = 0x06;
    reg.offset = 0x00;
    reg.size = 4;
    reg.defaultValue = 0x00000000;
    reg.minValue = 0;
    reg.maxValue = 0xFFFFFFFF;
    reg.unit = "";
    reg.description = "系统控制寄存器";
    addRegister(reg);
    
    // 8. 设备信息寄存器
    reg.name = "DEVINFO";
    reg.baseAddress = 0x07;
    reg.offset = 0x00;
    reg.size = 4;
    reg.defaultValue = 0x01000000;
    reg.minValue = 0;
    reg.maxValue = 0xFFFFFFFF;
    reg.unit = "";
    reg.description = "设备信息寄存器";
    addRegister(reg);
}

/**
 * @brief 添加寄存器定义
 * @param reg 寄存器定义结构体
 */
void RegisterManager::addRegister(const RegisterDefinition& reg)
{
    quint16 address = calculateRegisterAddress(reg.baseAddress, reg.offset);
    m_registers[address] = reg;
    m_registerValues[address] = reg.defaultValue;
}

/**
 * @brief 计算寄存器实际地址
 * @param baseAddr 基地址
 * @param offset 偏移地址
 * @return 计算后的实际地址
 * 
 * 计算公式：基地址 << 8 + 偏移地址
 */
quint16 RegisterManager::calculateRegisterAddress(quint8 baseAddr, quint8 offset) const
{
    return (static_cast<quint16>(baseAddr) << 8) | offset;
}

/**
 * @brief 读取寄存器值
 * @param baseAddr 基地址
 * @param offset 偏移地址
 * @param value 输出参数，读取到的值
 * @return 读取是否成功
 */
bool RegisterManager::readRegister(quint8 baseAddr, quint8 offset, QVariant& value)
{
    QMutexLocker locker(&m_mutex);
    quint16 address = calculateRegisterAddress(baseAddr, offset);
    
    if (m_registerValues.contains(address)) {
        value = m_registerValues[address];
        return true;
    }
    
    qWarning() << "寄存器未找到:" << QString("0x%1").arg(baseAddr, 0, 16) << QString("0x%1").arg(offset, 0, 16);
    return false;
}

/**
 * @brief 写入寄存器值
 * @param baseAddr 基地址
 * @param offset 偏移地址
 * @param value 要写入的值
 * @return 写入是否成功
 * 
 * 会进行值范围检查
 */
bool RegisterManager::writeRegister(quint8 baseAddr, quint8 offset, const QVariant& value)
{
    QMutexLocker locker(&m_mutex);
    quint16 address = calculateRegisterAddress(baseAddr, offset);
    
    // 检查寄存器是否存在
    if (!m_registers.contains(address)) {
        qWarning() << "寄存器未找到:" << QString("0x%1").arg(baseAddr, 0, 16) << QString("0x%1").arg(offset, 0, 16);
        return false;
    }
    
    const RegisterDefinition& reg = m_registers[address];
    
    // 检查值类型
    bool ok;
    qlonglong numValue = value.toLongLong(&ok);
    if (!ok) {
        qWarning() << "寄存器值类型无效";
        return false;
    }
    
    // 检查值范围
    if (numValue < reg.minValue.toLongLong() || numValue > reg.maxValue.toLongLong()) {
        qWarning() << "寄存器值超出范围:" << reg.name;
        return false;
    }
    
    // 更新寄存器值
    m_registerValues[address] = value;
    qDebug() << "寄存器" << reg.name << "设置为" << value;
    
    return true;
}
