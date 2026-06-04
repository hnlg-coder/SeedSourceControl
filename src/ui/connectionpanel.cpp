#include "connectionpanel.h"
#include <QSerialPortInfo>
#include <QHBoxLayout>
#include <QFormLayout>

ConnectionPanel::ConnectionPanel(QWidget* parent)
    : QWidget(parent)
    , m_connected(false)
{
    setupUI();
    refreshPorts();
}

QString ConnectionPanel::selectedPort() const
{
    return m_portCombo->currentText();
}

qint32 ConnectionPanel::selectedBaudRate() const
{
    return m_baudRateCombo->currentText().toInt();
}

QSerialPort::DataBits ConnectionPanel::selectedDataBits() const
{
    return static_cast<QSerialPort::DataBits>(m_dataBitsCombo->currentData().toInt());
}

QSerialPort::Parity ConnectionPanel::selectedParity() const
{
    return static_cast<QSerialPort::Parity>(m_parityCombo->currentData().toInt());
}

QSerialPort::StopBits ConnectionPanel::selectedStopBits() const
{
    return static_cast<QSerialPort::StopBits>(m_stopBitsCombo->currentData().toInt());
}

QSerialPort::FlowControl ConnectionPanel::selectedFlowControl() const
{
    return static_cast<QSerialPort::FlowControl>(m_flowControlCombo->currentData().toInt());
}

void ConnectionPanel::setConnected(bool connected)
{
    m_connected = connected;
    
    if (connected) {
        m_connectButton->setText("Disconnect");
        m_statusLabel->setText("Status: Connected");
        m_statusLabel->setStyleSheet("color: green; font-weight: bold;");
        m_portCombo->setEnabled(false);
        m_baudRateCombo->setEnabled(false);
        m_dataBitsCombo->setEnabled(false);
        m_parityCombo->setEnabled(false);
        m_stopBitsCombo->setEnabled(false);
        m_flowControlCombo->setEnabled(false);
    } else {
        m_connectButton->setText("Connect");
        m_statusLabel->setText("Status: Disconnected");
        m_statusLabel->setStyleSheet("color: red; font-weight: bold;");
        m_portCombo->setEnabled(true);
        m_baudRateCombo->setEnabled(true);
        m_dataBitsCombo->setEnabled(true);
        m_parityCombo->setEnabled(true);
        m_stopBitsCombo->setEnabled(true);
        m_flowControlCombo->setEnabled(true);
    }
}

void ConnectionPanel::refreshPorts()
{
    m_portCombo->clear();
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo& port : ports) {
        m_portCombo->addItem(port.portName());
    }
}

void ConnectionPanel::onConnectButtonClicked()
{
    if (m_connected) {
        emit disconnectClicked();
    } else {
        emit connectClicked();
    }
}

void ConnectionPanel::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QGroupBox* groupBox = new QGroupBox("Serial Port Configuration");
    QFormLayout* formLayout = new QFormLayout();
    
    m_portCombo = new QComboBox();
    m_baudRateCombo = new QComboBox();
    m_dataBitsCombo = new QComboBox();
    m_parityCombo = new QComboBox();
    m_stopBitsCombo = new QComboBox();
    m_flowControlCombo = new QComboBox();
    
    populateBaudRates();
    populateDataBits();
    populateParity();
    populateStopBits();
    populateFlowControl();
    
    formLayout->addRow("Port:", m_portCombo);
    formLayout->addRow("Baud Rate:", m_baudRateCombo);
    formLayout->addRow("Data Bits:", m_dataBitsCombo);
    formLayout->addRow("Parity:", m_parityCombo);
    formLayout->addRow("Stop Bits:", m_stopBitsCombo);
    formLayout->addRow("Flow Control:", m_flowControlCombo);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_connectButton = new QPushButton("Connect");
    m_refreshButton = new QPushButton("Refresh");
    
    buttonLayout->addWidget(m_refreshButton);
    buttonLayout->addWidget(m_connectButton);
    buttonLayout->addStretch();
    
    m_statusLabel = new QLabel("Status: Disconnected");
    m_statusLabel->setStyleSheet("color: red; font-weight: bold;");
    
    formLayout->addRow(m_statusLabel);
    formLayout->addRow(buttonLayout);
    
    groupBox->setLayout(formLayout);
    mainLayout->addWidget(groupBox);
    mainLayout->addStretch();
    
    connect(m_refreshButton, &QPushButton::clicked, this, &ConnectionPanel::refreshPorts);
    connect(m_connectButton, &QPushButton::clicked, this, &ConnectionPanel::onConnectButtonClicked);
}

void ConnectionPanel::populateBaudRates()
{
    QList<qint32> baudRates = QSerialPortInfo::standardBaudRates();
    for (qint32 rate : baudRates) {
        m_baudRateCombo->addItem(QString::number(rate), rate);
    }
    m_baudRateCombo->setCurrentText("115200");
}

void ConnectionPanel::populateDataBits()
{
    m_dataBitsCombo->addItem("5", QSerialPort::Data5);
    m_dataBitsCombo->addItem("6", QSerialPort::Data6);
    m_dataBitsCombo->addItem("7", QSerialPort::Data7);
    m_dataBitsCombo->addItem("8", QSerialPort::Data8);
    m_dataBitsCombo->setCurrentIndex(3);
}

void ConnectionPanel::populateParity()
{
    m_parityCombo->addItem("None", QSerialPort::NoParity);
    m_parityCombo->addItem("Even", QSerialPort::EvenParity);
    m_parityCombo->addItem("Odd", QSerialPort::OddParity);
    m_parityCombo->addItem("Mark", QSerialPort::MarkParity);
    m_parityCombo->addItem("Space", QSerialPort::SpaceParity);
}

void ConnectionPanel::populateStopBits()
{
    m_stopBitsCombo->addItem("1", QSerialPort::OneStop);
    m_stopBitsCombo->addItem("1.5", QSerialPort::OneAndHalfStop);
    m_stopBitsCombo->addItem("2", QSerialPort::TwoStop);
}

void ConnectionPanel::populateFlowControl()
{
    m_flowControlCombo->addItem("None", QSerialPort::NoFlowControl);
    m_flowControlCombo->addItem("Hardware", QSerialPort::HardwareControl);
    m_flowControlCombo->addItem("Software", QSerialPort::SoftwareControl);
}
