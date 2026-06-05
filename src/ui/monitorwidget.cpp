#include "monitorwidget.h"

MonitorWidget::MonitorWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void MonitorWidget::updateData(const DeviceDataModel::RealTimeData& data)
{
    m_curPdLabel->setText(QString("%1 mA").arg(data.curPd, 0, 'f', 3));
    m_curTecLabel->setText(QString("%1 mA").arg(data.curTec, 0, 'f', 0));
    m_pwrLasLabel->setText(QString("%1 mW").arg(data.pwrLas, 0, 'f', 2));
    m_pwrCcLabel->setText(QString("%1 mW").arg(data.pwrCc, 0, 'f', 0));
    m_pwrSysLabel->setText(QString("%1 mW").arg(data.pwrSys, 0, 'f', 0));

    // 更新电流图表
    m_currentChart->updateData("PD电流", data.curPd);
    m_currentChart->updateData("TEC电流", data.curTec);

    // 更新功率图表
    m_powerChart->updateData("激光功率", data.pwrLas);
    m_powerChart->updateData("CC功率", data.pwrCc);
    m_powerChart->updateData("系统功率", data.pwrSys);
}

void MonitorWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // === 数值显示区 ===
    QGroupBox* dataGroup = new QGroupBox("MONITOR寄存器 [06h] 监测数据");
    QGridLayout* dataLayout = new QGridLayout();

    auto createValueLabel = [](const QString& text) -> QLabel* {
        QLabel* label = new QLabel(text);
        label->setStyleSheet("font-weight: bold; font-size: 14px;");
        return label;
    };

    dataLayout->addWidget(new QLabel("PD电流 (CUR_PD):"), 0, 0);
    m_curPdLabel = createValueLabel("0.000 mA");
    m_curPdLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #E91E63;");
    dataLayout->addWidget(m_curPdLabel, 0, 1);

    dataLayout->addWidget(new QLabel("TEC电流 (CUR_TEC):"), 0, 2);
    m_curTecLabel = createValueLabel("0 mA");
    m_curTecLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #9C27B0;");
    dataLayout->addWidget(m_curTecLabel, 0, 3);

    dataLayout->addWidget(new QLabel("激光功率 (PWR_LAS):"), 1, 0);
    m_pwrLasLabel = createValueLabel("0.00 mW");
    m_pwrLasLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #CC0066;");
    dataLayout->addWidget(m_pwrLasLabel, 1, 1);

    dataLayout->addWidget(new QLabel("CC功率 (PWR_CC):"), 1, 2);
    m_pwrCcLabel = createValueLabel("0 mW");
    m_pwrCcLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #FF5722;");
    dataLayout->addWidget(m_pwrCcLabel, 1, 3);

    dataLayout->addWidget(new QLabel("系统功率 (PWR_SYS):"), 2, 0);
    m_pwrSysLabel = createValueLabel("0 mW");
    m_pwrSysLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #6600CC;");
    dataLayout->addWidget(m_pwrSysLabel, 2, 1);

    dataGroup->setLayout(dataLayout);
    mainLayout->addWidget(dataGroup);

    // === 电流图表 ===
    QGroupBox* currentChartGroup = new QGroupBox("电流监测曲线");
    QVBoxLayout* currentChartLayout = new QVBoxLayout();

    m_currentChart = new EnhancedChartWidget();
    m_currentChart->addSeries("PD电流", QColor(233, 30, 99), "mA");
    m_currentChart->addSeries("TEC电流", QColor(156, 39, 176), "mA");
    m_currentChart->setTimeRange(120);
    m_currentChart->setAutoScaleY(true);
    m_currentChart->setTitle("电流监测 (mA)");
    m_currentChart->setShowStatistics(true);

    currentChartLayout->addWidget(m_currentChart);
    currentChartGroup->setLayout(currentChartLayout);
    mainLayout->addWidget(currentChartGroup, 1);

    // === 功率图表 ===
    QGroupBox* powerChartGroup = new QGroupBox("功率监测曲线");
    QVBoxLayout* powerChartLayout = new QVBoxLayout();

    m_powerChart = new EnhancedChartWidget();
    m_powerChart->addSeries("激光功率", QColor(204, 0, 102), "mW");
    m_powerChart->addSeries("CC功率", QColor(255, 87, 34), "mW");
    m_powerChart->addSeries("系统功率", QColor(102, 0, 204), "mW");
    m_powerChart->setTimeRange(120);
    m_powerChart->setAutoScaleY(true);
    m_powerChart->setTitle("功率监测 (mW)");
    m_powerChart->setShowStatistics(true);

    powerChartLayout->addWidget(m_powerChart);
    powerChartGroup->setLayout(powerChartLayout);
    mainLayout->addWidget(powerChartGroup, 1);
}
