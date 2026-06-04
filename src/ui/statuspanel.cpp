#include "statuspanel.h"
#include <QFormLayout>

StatusPanel::StatusPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void StatusPanel::updateStatus(quint32 status)
{
    QString statusText;
    QString colorStyle;
    
    if (status == 0) {
        statusText = "Idle";
        colorStyle = "color: gray; font-weight: bold;";
    } else if (status & 0x01) {
        statusText = "Running";
        colorStyle = "color: green; font-weight: bold;";
    } else if (status & 0x02) {
        statusText = "Error";
        colorStyle = "color: red; font-weight: bold;";
    } else {
        statusText = QString("0x%1").arg(status, 8, 16, QChar('0'));
        colorStyle = "color: black; font-weight: bold;";
    }
    
    m_statusValueLabel->setText(statusText);
    m_statusValueLabel->setStyleSheet(colorStyle);
}

void StatusPanel::updateCurrent(double current)
{
    m_currentValueLabel->setText(QString("%1 mA").arg(current, 0, 'f', 2));
}

void StatusPanel::updateTemperature(double temperature)
{
    m_temperatureValueLabel->setText(QString("%1 °C").arg(temperature, 0, 'f', 2));
    
    QString colorStyle = "color: black; font-weight: bold;";
    if (temperature > 80) {
        colorStyle = "color: red; font-weight: bold;";
    } else if (temperature > 60) {
        colorStyle = "color: orange; font-weight: bold;";
    }
    m_temperatureValueLabel->setStyleSheet(colorStyle);
}

void StatusPanel::updatePower(double power)
{
    m_powerValueLabel->setText(QString("%1 W").arg(power, 0, 'f', 2));
}

void StatusPanel::updateAlarm(quint32 alarm)
{
    QString alarmText;
    QString colorStyle;
    
    if (alarm == 0) {
        alarmText = "No Alarm";
        colorStyle = "color: green; font-weight: bold;";
    } else {
        QStringList alarms;
        if (alarm & 0x01) alarms << "Over Current";
        if (alarm & 0x02) alarms << "Over Temp";
        if (alarm & 0x04) alarms << "Over Power";
        if (alarm & 0x08) alarms << "Voltage Error";
        if (alarm & 0x10) alarms << "Comm Error";
        if (alarm & 0x20) alarms << "HW Fault";
        
        alarmText = alarms.isEmpty() ? QString("0x%1").arg(alarm, 8, 16, QChar('0')) : alarms.join(", ");
        colorStyle = "color: red; font-weight: bold;";
    }
    
    m_alarmValueLabel->setText(alarmText);
    m_alarmValueLabel->setStyleSheet(colorStyle);
}

void StatusPanel::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QGroupBox* groupBox = new QGroupBox("Device Status");
    QFormLayout* formLayout = new QFormLayout();
    
    m_statusLabel = new QLabel("Status:");
    m_statusValueLabel = new QLabel("Idle");
    m_statusValueLabel->setStyleSheet("color: gray; font-weight: bold;");
    
    m_currentLabel = new QLabel("Current:");
    m_currentValueLabel = new QLabel("0.00 mA");
    m_currentValueLabel->setStyleSheet("font-weight: bold;");
    
    m_temperatureLabel = new QLabel("Temperature:");
    m_temperatureValueLabel = new QLabel("25.00 °C");
    m_temperatureValueLabel->setStyleSheet("font-weight: bold;");
    
    m_powerLabel = new QLabel("Power:");
    m_powerValueLabel = new QLabel("0.00 W");
    m_powerValueLabel->setStyleSheet("font-weight: bold;");
    
    m_alarmLabel = new QLabel("Alarm:");
    m_alarmValueLabel = new QLabel("No Alarm");
    m_alarmValueLabel->setStyleSheet("color: green; font-weight: bold;");
    
    formLayout->addRow(m_statusLabel, m_statusValueLabel);
    formLayout->addRow(m_currentLabel, m_currentValueLabel);
    formLayout->addRow(m_temperatureLabel, m_temperatureValueLabel);
    formLayout->addRow(m_powerLabel, m_powerValueLabel);
    formLayout->addRow(m_alarmLabel, m_alarmValueLabel);
    
    groupBox->setLayout(formLayout);
    mainLayout->addWidget(groupBox);
    mainLayout->addStretch();
}
