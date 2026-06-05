#include "temperaturecontrolwidget.h"

TemperatureControlWidget::TemperatureControlWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void TemperatureControlWidget::updateData(const DeviceDataModel::RealTimeData& data)
{
    m_tempValLabel->setText(QString("%1 °C").arg(data.tempVal, 0, 'f', 3));

    m_tempStatusLabel->setText(data.status.tempStatusText());
    QString color = (data.status.tempStatus == 3) ? "green" : "red";
    m_tempStatusLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(color));

    m_chart->updateData("实际温度", data.tempVal);
    m_chart->updateData("目标温度", data.tempSet);
}

void TemperatureControlWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QGroupBox* controlGroup = new QGroupBox("温度控制");
    QFormLayout* formLayout = new QFormLayout();

    m_tempSetSpinBox = new QDoubleSpinBox();
    m_tempSetSpinBox->setRange(-40, 125);
    m_tempSetSpinBox->setDecimals(3);
    m_tempSetSpinBox->setSuffix(" °C");
    m_tempSetSpinBox->setSingleStep(0.001);
    m_tempSetSpinBox->setValue(25.0);

    m_applyButton = new QPushButton("应用");
    m_applyButton->setStyleSheet("background-color: #2196F3; color: white; font-weight: bold; padding: 6px 20px;");

    QHBoxLayout* setLayout = new QHBoxLayout();
    setLayout->addWidget(m_tempSetSpinBox);
    setLayout->addWidget(m_applyButton);

    formLayout->addRow("目标温度 (TEMP_SET):", setLayout);

    m_tempValLabel = new QLabel("25.000 °C");
    m_tempValLabel->setStyleSheet("font-weight: bold; font-size: 16px; color: #CC6600;");
    formLayout->addRow("实际温度 (TEMP_VAL):", m_tempValLabel);

    m_tempStatusLabel = new QLabel("关断");
    m_tempStatusLabel->setStyleSheet("color: gray; font-weight: bold;");
    formLayout->addRow("温度状态:", m_tempStatusLabel);

    controlGroup->setLayout(formLayout);
    mainLayout->addWidget(controlGroup);

    QGroupBox* chartGroup = new QGroupBox("温度实时曲线");
    QVBoxLayout* chartLayout = new QVBoxLayout();

    m_chart = new EnhancedChartWidget();
    m_chart->addSeries("实际温度", QColor(204, 102, 0), "°C");
    m_chart->addSeries("目标温度", QColor(76, 175, 80), "°C", false);
    m_chart->setTimeRange(120);
    m_chart->setAutoScaleY(true);
    m_chart->setTitle("温度监测 (°C)");
    m_chart->setShowStatistics(true);

    chartLayout->addWidget(m_chart);
    chartGroup->setLayout(chartLayout);
    mainLayout->addWidget(chartGroup, 1);

    connect(m_applyButton, &QPushButton::clicked, this, [this]() {
        emit temperatureSetChanged(m_tempSetSpinBox->value());
    });
}
