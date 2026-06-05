#include "configwidget.h"
#include <QScrollArea>

ConfigWidget::ConfigWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void ConfigWidget::updateData(const DeviceDataModel::RealTimeData& data)
{
    // CONFIG
    m_tcEnCheck->setChecked(data.config.tcEn);
    m_ccEnCheck->setChecked(data.config.ccEn);
    m_aeEnCheck->setChecked(data.config.aeEn);
    m_powerSvCheck->setChecked(data.config.powerSv);
    m_curSvCheck->setChecked(data.config.curSv);
    m_tempSvCheck->setChecked(data.config.tempSv);
    m_curThSpinBox->setValue(data.config.curTh);
    m_curSlopeSpinBox->setValue(data.config.curSlope);
    m_baudRateCombo->setCurrentIndex(qMin(data.config.baudRate, static_cast<quint8>(m_baudRateCombo->count() - 1)));

    // DEVINFO
    m_devNameLabel->setText(QString::number(data.devInfo.name));
    m_verSLabel->setText(QString("V%1").arg(data.devInfo.verS));
    m_verHLabel->setText(QString("V%1").arg(data.devInfo.verH));
    m_serialLabel->setText(QString::number(data.devInfo.serial));
    m_curMaxLabel->setText(QString("%1 mA").arg(data.devInfo.curMax, 0, 'f', 1));
    m_tempMinLabel->setText(QString("%1 °C").arg(data.devInfo.tempMin));
    m_tempMaxLabel->setText(QString("%1 °C").arg(data.devInfo.tempMax));
}

void ConfigWidget::setupUI()
{
    QVBoxLayout* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    QWidget* contentWidget = new QWidget();
    QHBoxLayout* mainLayout = new QHBoxLayout(contentWidget);
    mainLayout->setSpacing(12);

    // === 左栏：CONFIG寄存器 [03h] ===
    QGroupBox* configGroup = new QGroupBox("配置寄存器 [03h]");
    QFormLayout* configLayout = new QFormLayout();

    m_tcEnCheck = new QCheckBox("温度控制使能 (TC_EN)");
    m_ccEnCheck = new QCheckBox("恒流控制使能 (CC_EN)");
    m_aeEnCheck = new QCheckBox("自动使能 (AE_EN)");
    m_powerSvCheck = new QCheckBox("功率保护 (POWER_SV)");
    m_curSvCheck = new QCheckBox("电流保护 (CUR_SV)");
    m_tempSvCheck = new QCheckBox("温度保护 (TEMP_SV)");

    m_curThSpinBox = new QDoubleSpinBox();
    m_curThSpinBox->setRange(0, 655.35);
    m_curThSpinBox->setDecimals(2);
    m_curThSpinBox->setSuffix(" mA");
    m_curThSpinBox->setSingleStep(0.01);

    m_curSlopeSpinBox = new QDoubleSpinBox();
    m_curSlopeSpinBox->setRange(0, 0.819);
    m_curSlopeSpinBox->setDecimals(4);
    m_curSlopeSpinBox->setSuffix(" mW/mA");
    m_curSlopeSpinBox->setSingleStep(0.0002);

    m_baudRateCombo = new QComboBox();
    m_baudRateCombo->addItems({"9600", "19200", "38400", "57600", "115200", "230400", "460800", "921600"});

    m_applyConfigButton = new QPushButton("应用配置");
    m_applyConfigButton->setStyleSheet("background-color: #2196F3; color: white; font-weight: bold; padding: 6px 20px;");

    configLayout->addRow(m_tcEnCheck);
    configLayout->addRow(m_ccEnCheck);
    configLayout->addRow(m_aeEnCheck);
    configLayout->addRow(m_powerSvCheck);
    configLayout->addRow(m_curSvCheck);
    configLayout->addRow(m_tempSvCheck);
    configLayout->addRow("电流阈值 (CUR_TH):", m_curThSpinBox);
    configLayout->addRow("电流斜率 (CUR_SLOPE):", m_curSlopeSpinBox);
    configLayout->addRow("波特率 (BAUD_RATE):", m_baudRateCombo);
    configLayout->addRow("", m_applyConfigButton);

    configGroup->setLayout(configLayout);
    mainLayout->addWidget(configGroup);

    // === 中栏：DEVINFO寄存器 [01h] ===
    QGroupBox* devInfoGroup = new QGroupBox("设备信息 [01h] (只读)");
    QFormLayout* devInfoLayout = new QFormLayout();

    m_devNameLabel = new QLabel("-");
    m_verSLabel = new QLabel("-");
    m_verHLabel = new QLabel("-");
    m_serialLabel = new QLabel("-");
    m_curMaxLabel = new QLabel("-");
    m_tempMinLabel = new QLabel("-");
    m_tempMaxLabel = new QLabel("-");

    m_readDevInfoButton = new QPushButton("读取设备信息");
    m_readDevInfoButton->setStyleSheet("background-color: #607D8B; color: white; font-weight: bold; padding: 6px 20px;");

    devInfoLayout->addRow("产品名称 (NAME):", m_devNameLabel);
    devInfoLayout->addRow("软件版本 (VER_S):", m_verSLabel);
    devInfoLayout->addRow("硬件版本 (VER_H):", m_verHLabel);
    devInfoLayout->addRow("序列号 (SERIAL):", m_serialLabel);
    devInfoLayout->addRow("最大电流 (CUR_MAX):", m_curMaxLabel);
    devInfoLayout->addRow("最低温度 (TEMP_MIN):", m_tempMinLabel);
    devInfoLayout->addRow("最高温度 (TEMP_MAX):", m_tempMaxLabel);
    devInfoLayout->addRow("", m_readDevInfoButton);

    devInfoGroup->setLayout(devInfoLayout);
    mainLayout->addWidget(devInfoGroup);

    // === 右栏：SYSTEM寄存器 [00h] ===
    QGroupBox* systemGroup = new QGroupBox("系统控制 [00h]");
    QVBoxLayout* systemLayout = new QVBoxLayout();

    m_sleepButton = new QPushButton("休眠模式");
    m_sleepButton->setStyleSheet("background-color: #FF9800; color: white; font-weight: bold; padding: 8px 20px;");

    m_resetButton = new QPushButton("软复位");
    m_resetButton->setStyleSheet("background-color: #f44336; color: white; font-weight: bold; padding: 8px 20px;");

    m_updateButton = new QPushButton("固件升级 (IAP)");
    m_updateButton->setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold; padding: 8px 20px;");

    systemLayout->addWidget(m_sleepButton);
    systemLayout->addWidget(m_resetButton);
    systemLayout->addWidget(m_updateButton);
    systemLayout->addStretch();

    systemGroup->setLayout(systemLayout);
    mainLayout->addWidget(systemGroup);

    scrollArea->setWidget(contentWidget);
    outerLayout->addWidget(scrollArea);

    connect(m_applyConfigButton, &QPushButton::clicked, this, &ConfigWidget::configChanged);
}

DeviceDataModel::ConfigBits ConfigWidget::getConfigData() const
{
    DeviceDataModel::ConfigBits c;
    c.tcEn    = m_tcEnCheck->isChecked();
    c.ccEn    = m_ccEnCheck->isChecked();
    c.aeEn    = m_aeEnCheck->isChecked();
    c.powerSv = m_powerSvCheck->isChecked();
    c.curSv   = m_curSvCheck->isChecked();
    c.tempSv  = m_tempSvCheck->isChecked();
    c.curTh   = m_curThSpinBox->value();
    c.curSlope = m_curSlopeSpinBox->value();
    c.baudRate = static_cast<quint8>(m_baudRateCombo->currentIndex());
    return c;
}
