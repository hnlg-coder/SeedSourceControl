#include "dashboardwidget.h"
#include <QFormLayout>

DashboardWidget::DashboardWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void DashboardWidget::updateData(const DeviceDataModel::RealTimeData& data)
{
    // 设备状态
    if (data.status.idle) {
        m_deviceStatusValue->setText("空闲");
        m_deviceStatusValue->setStyleSheet(statusColorStyle("gray"));
    } else {
        m_deviceStatusValue->setText("运行中");
        m_deviceStatusValue->setStyleSheet(statusColorStyle("green"));
    }

    // 电流状态
    m_curStatusValue->setText(data.status.curStatusText());
    QString curColor = (data.status.curStatus == 3) ? "green" : "red";
    m_curStatusValue->setStyleSheet(statusColorStyle(curColor));

    // 温度状态
    m_tempStatusValue->setText(data.status.tempStatusText());
    QString tempColor = (data.status.tempStatus == 3) ? "green" : "red";
    m_tempStatusValue->setStyleSheet(statusColorStyle(tempColor));

    // 报警状态
    if (data.alert.hasAlert()) {
        m_alertStatusValue->setText(data.alert.alertNames().join("\n"));
        m_alertStatusValue->setStyleSheet(statusColorStyle("red"));
    } else {
        m_alertStatusValue->setText("正常");
        m_alertStatusValue->setStyleSheet(statusColorStyle("green"));
    }

    // 关键数值
    m_curSetValue->setText(QString("%1 mA").arg(data.curSet, 0, 'f', 2));
    m_curValValue->setText(QString("%1 mA").arg(data.curVal, 0, 'f', 2));
    m_tempSetValue->setText(QString("%1 °C").arg(data.tempSet, 0, 'f', 3));
    m_tempValValue->setText(QString("%1 °C").arg(data.tempVal, 0, 'f', 3));
    m_pwrLasValue->setText(QString("%1 mW").arg(data.pwrLas, 0, 'f', 2));
    m_pwrSysValue->setText(QString("%1 mW").arg(data.pwrSys, 0, 'f', 0));
}

QString DashboardWidget::statusColorStyle(const QString& color) const
{
    return QString("color: %1; font-weight: bold; font-size: 13px;").arg(color);
}

void DashboardWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // === 状态指示区 ===
    QGroupBox* statusGroup = new QGroupBox("设备状态");
    QGridLayout* statusLayout = new QGridLayout();

    statusLayout->addWidget(new QLabel("设备状态:"), 0, 0);
    m_deviceStatusValue = new QLabel("空闲");
    m_deviceStatusValue->setStyleSheet(statusColorStyle("gray"));
    statusLayout->addWidget(m_deviceStatusValue, 0, 1);

    statusLayout->addWidget(new QLabel("电流状态:"), 0, 2);
    m_curStatusValue = new QLabel("关断");
    m_curStatusValue->setStyleSheet(statusColorStyle("gray"));
    statusLayout->addWidget(m_curStatusValue, 0, 3);

    statusLayout->addWidget(new QLabel("温度状态:"), 0, 4);
    m_tempStatusValue = new QLabel("关断");
    m_tempStatusValue->setStyleSheet(statusColorStyle("gray"));
    statusLayout->addWidget(m_tempStatusValue, 0, 5);

    statusLayout->addWidget(new QLabel("报警:"), 1, 0);
    m_alertStatusValue = new QLabel("正常");
    m_alertStatusValue->setStyleSheet(statusColorStyle("green"));
    statusLayout->addWidget(m_alertStatusValue, 1, 1, 1, 5);

    statusGroup->setLayout(statusLayout);
    mainLayout->addWidget(statusGroup);

    // === 关键数值区 ===
    QGroupBox* dataGroup = new QGroupBox("实时数据");
    QGridLayout* dataLayout = new QGridLayout();

    // 电流
    dataLayout->addWidget(new QLabel("目标电流:"), 0, 0);
    m_curSetValue = new QLabel("0.00 mA");
    m_curSetValue->setStyleSheet("font-weight: bold; font-size: 14px;");
    dataLayout->addWidget(m_curSetValue, 0, 1);

    dataLayout->addWidget(new QLabel("实际电流:"), 0, 2);
    m_curValValue = new QLabel("0.00 mA");
    m_curValValue->setStyleSheet("font-weight: bold; font-size: 14px; color: #0066CC;");
    dataLayout->addWidget(m_curValValue, 0, 3);

    // 温度
    dataLayout->addWidget(new QLabel("目标温度:"), 1, 0);
    m_tempSetValue = new QLabel("25.000 °C");
    m_tempSetValue->setStyleSheet("font-weight: bold; font-size: 14px;");
    dataLayout->addWidget(m_tempSetValue, 1, 1);

    dataLayout->addWidget(new QLabel("实际温度:"), 1, 2);
    m_tempValValue = new QLabel("25.000 °C");
    m_tempValValue->setStyleSheet("font-weight: bold; font-size: 14px; color: #CC6600;");
    dataLayout->addWidget(m_tempValValue, 1, 3);

    // 功率
    dataLayout->addWidget(new QLabel("激光功率:"), 2, 0);
    m_pwrLasValue = new QLabel("0.00 mW");
    m_pwrLasValue->setStyleSheet("font-weight: bold; font-size: 14px; color: #CC0066;");
    dataLayout->addWidget(m_pwrLasValue, 2, 1);

    dataLayout->addWidget(new QLabel("系统功率:"), 2, 2);
    m_pwrSysValue = new QLabel("0 mW");
    m_pwrSysValue->setStyleSheet("font-weight: bold; font-size: 14px; color: #6600CC;");
    dataLayout->addWidget(m_pwrSysValue, 2, 3);

    dataGroup->setLayout(dataLayout);
    mainLayout->addWidget(dataGroup);

    // === 快捷控制 ===
    QGroupBox* controlGroup = new QGroupBox("快捷控制");
    QHBoxLayout* controlLayout = new QHBoxLayout();

    m_startButton = new QPushButton("启动设备");
    m_startButton->setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold; font-size: 14px; padding: 12px 40px; border-radius: 4px;");
    m_stopButton = new QPushButton("停止设备");
    m_stopButton->setStyleSheet("background-color: #f44336; color: white; font-weight: bold; font-size: 14px; padding: 12px 40px; border-radius: 4px;");

    controlLayout->addStretch();
    controlLayout->addWidget(m_startButton);
    controlLayout->addSpacing(30);
    controlLayout->addWidget(m_stopButton);
    controlLayout->addStretch();

    controlGroup->setLayout(controlLayout);
    mainLayout->addWidget(controlGroup);

    mainLayout->addStretch();

    connect(m_startButton, &QPushButton::clicked, this, &DashboardWidget::startClicked);
    connect(m_stopButton, &QPushButton::clicked, this, &DashboardWidget::stopClicked);
}
