#include "controlpanel.h"
#include <QHBoxLayout>

ControlPanel::ControlPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

double ControlPanel::targetCurrent() const
{
    return m_currentSpinBox->value();
}

double ControlPanel::targetTemperature() const
{
    return m_temperatureSpinBox->value();
}

double ControlPanel::targetPower() const
{
    return m_powerSpinBox->value();
}

void ControlPanel::setTargetCurrent(double current)
{
    m_currentSpinBox->setValue(current);
}

void ControlPanel::setTargetTemperature(double temperature)
{
    m_temperatureSpinBox->setValue(temperature);
}

void ControlPanel::setTargetPower(double power)
{
    m_powerSpinBox->setValue(power);
}

void ControlPanel::onStartClicked()
{
    emit startClicked();
}

void ControlPanel::onStopClicked()
{
    emit stopClicked();
}

void ControlPanel::onResetClicked()
{
    emit resetClicked();
}

void ControlPanel::onCalibrateClicked()
{
    emit calibrateClicked();
}

void ControlPanel::onCurrentSpinBoxChanged(double value)
{
    emit currentChanged(value);
}

void ControlPanel::onTemperatureSpinBoxChanged(double value)
{
    emit temperatureChanged(value);
}

void ControlPanel::onPowerSpinBoxChanged(double value)
{
    emit powerChanged(value);
}

void ControlPanel::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QGroupBox* controlGroup = new QGroupBox("Device Control");
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    m_startButton = new QPushButton("Start");
    m_stopButton = new QPushButton("Stop");
    m_resetButton = new QPushButton("Reset");
    m_calibrateButton = new QPushButton("Calibrate");
    
    m_startButton->setStyleSheet("background-color: green; color: white; font-weight: bold; padding: 10px;");
    m_stopButton->setStyleSheet("background-color: red; color: white; font-weight: bold; padding: 10px;");
    m_resetButton->setStyleSheet("background-color: orange; color: white; font-weight: bold; padding: 10px;");
    m_calibrateButton->setStyleSheet("background-color: blue; color: white; font-weight: bold; padding: 10px;");
    
    buttonLayout->addWidget(m_startButton);
    buttonLayout->addWidget(m_stopButton);
    buttonLayout->addWidget(m_resetButton);
    buttonLayout->addWidget(m_calibrateButton);
    
    controlGroup->setLayout(buttonLayout);
    mainLayout->addWidget(controlGroup);
    
    QGroupBox* parameterGroup = new QGroupBox("Parameter Settings");
    QFormLayout* formLayout = new QFormLayout();
    
    m_currentSpinBox = new QDoubleSpinBox();
    m_currentSpinBox->setRange(0, 1000);
    m_currentSpinBox->setDecimals(2);
    m_currentSpinBox->setSuffix(" mA");
    m_currentSpinBox->setValue(0);
    
    m_temperatureSpinBox = new QDoubleSpinBox();
    m_temperatureSpinBox->setRange(-40, 125);
    m_temperatureSpinBox->setDecimals(2);
    m_temperatureSpinBox->setSuffix(" °C");
    m_temperatureSpinBox->setValue(25);
    
    m_powerSpinBox = new QDoubleSpinBox();
    m_powerSpinBox->setRange(0, 10000);
    m_powerSpinBox->setDecimals(2);
    m_powerSpinBox->setSuffix(" mW");
    m_powerSpinBox->setValue(0);
    
    formLayout->addRow("Target Current:", m_currentSpinBox);
    formLayout->addRow("Target Temperature:", m_temperatureSpinBox);
    formLayout->addRow("Target Power:", m_powerSpinBox);
    
    parameterGroup->setLayout(formLayout);
    mainLayout->addWidget(parameterGroup);
    mainLayout->addStretch();
    
    connect(m_startButton, &QPushButton::clicked, this, &ControlPanel::onStartClicked);
    connect(m_stopButton, &QPushButton::clicked, this, &ControlPanel::onStopClicked);
    connect(m_resetButton, &QPushButton::clicked, this, &ControlPanel::onResetClicked);
    connect(m_calibrateButton, &QPushButton::clicked, this, &ControlPanel::onCalibrateClicked);
    connect(m_currentSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ControlPanel::onCurrentSpinBoxChanged);
    connect(m_temperatureSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ControlPanel::onTemperatureSpinBoxChanged);
    connect(m_powerSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ControlPanel::onPowerSpinBoxChanged);
}
