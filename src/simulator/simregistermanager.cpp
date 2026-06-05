#include "simregistermanager.h"

SimRegisterManager::SimRegisterManager(QObject* parent)
    : QObject(parent)
{
}

void SimRegisterManager::initialize()
{
    // STATUS register [0x00:0x00] - default: idle
    m_registers[{0x00, 0x00}] = 0x00000001;

    // ALERT register [0x01:0x00] - default: no alarm
    m_registers[{0x01, 0x00}] = 0x00000000;

    // CUR register [0x02:0x00] - current set point (raw: 0.01mA units)
    m_registers[{0x02, 0x00}] = 0;

    // TEMP register [0x03:0x00] - temperature set point (raw: 0.001 C units)
    m_registers[{0x03, 0x00}] = 0;

    // POWER register [0x04:0x00] - power reading (raw: 0.01mW units)
    m_registers[{0x04, 0x00}] = 0;

    // CONFIG register [0x05:0x00]
    m_registers[{0x05, 0x00}] = 0x0000003F;

    // SYSTEM register [0x06:0x00]
    m_registers[{0x06, 0x00}] = 0;

    // DEVINFO registers [0x07:0x00] ~ [0x07:0x0F]
    m_registers[{0x07, 0x00}] = 0x20240101; // date code
    m_registers[{0x07, 0x01}] = 0x00010001; // hw version
    m_registers[{0x07, 0x02}] = 0x00020001; // fw version
    m_registers[{0x07, 0x03}] = 0x30303031; // serial "0001"
    m_registers[{0x07, 0x04}] = 0x00000000;
    m_registers[{0x07, 0x05}] = 0x00000000;
    m_registers[{0x07, 0x06}] = 0x0000012C; // max current 300mA
    m_registers[{0x07, 0x07}] = 0x0000003C; // max temp 60 C
    m_registers[{0x07, 0x08}] = 0x0000C350; // max power 50000mW
    for (int i = 9; i <= 15; i++) {
        m_registers[{0x07, static_cast<quint8>(i)}] = 0;
    }

    emit logMessage(QStringLiteral("SimRegisterManager: registers initialized"));
}

void SimRegisterManager::reset()
{
    m_registers.clear();
    initialize();
    emit logMessage(QStringLiteral("SimRegisterManager: registers reset"));
}

bool SimRegisterManager::readRegister(quint8 baseAddr, quint8 offset, QVariant& value)
{
    auto it = m_registers.find({baseAddr, offset});
    if (it != m_registers.end()) {
        value = it.value();
        return true;
    }
    return false;
}

bool SimRegisterManager::writeRegister(quint8 baseAddr, quint8 offset, quint32 value)
{
    m_registers[{baseAddr, offset}] = value;
    return true;
}

quint32 SimRegisterManager::readRaw(quint8 baseAddr, quint8 offset) const
{
    auto it = m_registers.find({baseAddr, offset});
    if (it != m_registers.end()) {
        return it.value();
    }
    return 0;
}

void SimRegisterManager::writeRaw(quint8 baseAddr, quint8 offset, quint32 value)
{
    m_registers[{baseAddr, offset}] = value;
}
