#include "currentcontrolwidget.h"

CurrentControlWidget::CurrentControlWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void CurrentControlWidget::updateData(const DeviceDataModel::RealTimeData& data)
{
    // 更新实际电流值显示
    m_curValLabel->setText(QString("%1 mA").arg(data.curVal, 0, 'f', 2));

    // 更新电流状态
    m_curStatusLabel->setText(data.status.curStatusText());
    QString color = (data.status.curStatus == 3) ? "green" : "red";
    m_curStatusLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(color));

    // 更新图表
    m_chart->updateData("实际电流", data.curVal);
    m_chart->updateData("目标电流", data.curSet);
}

void CurrentControlWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // === 控制区 ===
    QGroupBox* controlGroup = new QGroupBox("电流控制");
    QFormLayout* formLayout = new QFormLayout();

    m_curSetSpinBox = new QDoubleSpinBox();
    m_curSetSpinBox->setRange(0, 4095);
    m_curSetSpinBox->setDecimals(2);
    m_curSetSpinBox->setSuffix(" mA");
    m_curSetSpinBox->setSingleStep(0.02);
    m_curSetSpinBox->setValue(0);

    m_applyButton = new QPushButton("应用");
    m_applyButton->setStyleSheet("background-color: #2196F3; color: white; font-weight: bold; padding: 6px 20px;");

    QHBoxLayout* setLayout = new QHBoxLayout();
    setLayout->addWidget(m_curSetSpinBox);
    setLayout->addWidget(m_applyButton);

    formLayout->addRow("目标电流 (CUR_SET):", setLayout);

    m_curValLabel = new QLabel("0.00 mA");
    m_curValLabel->setStyleSheet("font-weight: bold; font-size: 16px; color: #0066CC;");
    formLayout->addRow("实际电流 (CUR_VAL):", m_curValLabel);

    m_curStatusLabel = new QLabel("关断");
    m_curStatusLabel->setStyleSheet("color: gray; font-weight: bold;");
    formLayout->addRow("电流状态:", m_curStatusLabel);

    controlGroup->setLayout(formLayout);
    mainLayout->addWidget(controlGroup);

    // === 实时曲线区 ===
    QGroupBox* chartGroup = new QGroupBox("电流实时曲线");
    QVBoxLayout* chartLayout = new QVBoxLayout();

    m_chart = new EnhancedChartWidget();
    m_chart->addSeries("实际电流", QColor(0, 102, 204), "mA");
    m_chart->addSeries("目标电流", QColor(255, 152, 0), "mA", false);
    m_chart->setTimeRange(120);
    m_chart->setAutoScaleY(true);
    m_chart->setTitle("电流监测 (mA)");
    m_chart->setShowStatistics(true);

    chartLayout->addWidget(m_chart);
    chartGroup->setLayout(chartLayout);
    mainLayout->addWidget(chartGroup, 1);

    // 连接信号
    connect(m_applyButton, &QPushButton::clicked, this, [this]() {
        emit currentSetChanged(m_curSetSpinBox->value());
    });
}
