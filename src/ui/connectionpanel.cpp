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
        m_statusLabel->setText("Connected");
        m_statusLabel->setStyleSheet("color: green; font-weight: bold; font-size: 11px;");
        m_portCombo->setEnabled(false);
        m_baudRateCombo->setEnabled(false);
        m_dataBitsCombo->setEnabled(false);
        m_parityCombo->setEnabled(false);
        m_stopBitsCombo->setEnabled(false);
        m_flowControlCombo->setEnabled(false);
    } else {
        m_connectButton->setText("Connect");
        m_statusLabel->setText("Disconnected");
        m_statusLabel->setStyleSheet("color: red; font-weight: bold; font-size: 11px;");
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
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(4, 2, 4, 2);
    mainLayout->setSpacing(4);

    QString comboStyle =
        "QComboBox {"
        "  border: 1px solid #ccc;"
        "  border-radius: 2px;"
        "  padding: 1px 4px;"
        "  background: white;"
        "  min-height: 18px;"
        "  font-size: 11px;"
        "}"
        "QComboBox::drop-down {"
        "  width: 0px;"
        "  border: none;"
        "}"
        "QComboBox::down-arrow {"
        "  image: none;"
        "}";

    m_portCombo = new QComboBox();
    m_baudRateCombo = new QComboBox();
    m_dataBitsCombo = new QComboBox();
    m_parityCombo = new QComboBox();
    m_stopBitsCombo = new QComboBox();
    m_flowControlCombo = new QComboBox();

    m_portCombo->setStyleSheet(comboStyle);
    m_baudRateCombo->setStyleSheet(comboStyle);
    m_dataBitsCombo->setStyleSheet(comboStyle);
    m_parityCombo->setStyleSheet(comboStyle);
    m_stopBitsCombo->setStyleSheet(comboStyle);
    m_flowControlCombo->setStyleSheet(comboStyle);

    populateBaudRates();
    populateDataBits();
    populateParity();
    populateStopBits();
    populateFlowControl();

    m_portCombo->setMinimumWidth(80);
    m_baudRateCombo->setMinimumWidth(80);
    m_dataBitsCombo->setMinimumWidth(50);
    m_parityCombo->setMinimumWidth(60);
    m_stopBitsCombo->setMinimumWidth(50);
    m_flowControlCombo->setMinimumWidth(70);

    mainLayout->addWidget(new QLabel("Port:"));
    mainLayout->addWidget(m_portCombo);
    mainLayout->addWidget(new QLabel("Baud:"));
    mainLayout->addWidget(m_baudRateCombo);
    mainLayout->addWidget(new QLabel("Data:"));
    mainLayout->addWidget(m_dataBitsCombo);
    mainLayout->addWidget(new QLabel("Parity:"));
    mainLayout->addWidget(m_parityCombo);
    mainLayout->addWidget(new QLabel("Stop:"));
    mainLayout->addWidget(m_stopBitsCombo);
    mainLayout->addWidget(new QLabel("Flow:"));
    mainLayout->addWidget(m_flowControlCombo);

    mainLayout->addSpacing(8);

    m_connectButton = new QPushButton("Connect");
    m_refreshButton = new QPushButton("Refresh");
    m_connectButton->setFixedHeight(24);
    m_refreshButton->setFixedHeight(24);

    mainLayout->addWidget(m_refreshButton);
    mainLayout->addWidget(m_connectButton);

    m_statusLabel = new QLabel("Disconnected");
    m_statusLabel->setStyleSheet("color: red; font-weight: bold; font-size: 11px;");

    mainLayout->addWidget(m_statusLabel);
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
