#include "realtimeplotpanel.h"
#include <QDateTime>

RealtimePlotPanel::RealtimePlotPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void RealtimePlotPanel::updateCurrent(double value)
{
    m_plotWidget->updateData("Current", value);
}

void RealtimePlotPanel::updateTemperature(double value)
{
    m_plotWidget->updateData("Temperature", value);
}

void RealtimePlotPanel::updatePower(double value)
{
    m_plotWidget->updateData("Power", value);
}

void RealtimePlotPanel::clearData()
{
    m_plotWidget->clearData();
}

void RealtimePlotPanel::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QGroupBox* groupBox = new QGroupBox("Real-time Data");
    QVBoxLayout* plotLayout = new QVBoxLayout();
    
    m_plotWidget = new RealtimePlotWidget();
    m_plotWidget->addDataSeries("Current", QColor(255, 0, 0));
    m_plotWidget->addDataSeries("Temperature", QColor(0, 255, 0));
    m_plotWidget->addDataSeries("Power", QColor(0, 0, 255));
    m_plotWidget->setTimeRange(60);
    m_plotWidget->setYRange(-50, 100);
    
    plotLayout->addWidget(m_plotWidget);
    groupBox->setLayout(plotLayout);
    mainLayout->addWidget(groupBox);
}
