#include "connectionstatusbar.h"
#include <QHBoxLayout>

ConnectionStatusBar::ConnectionStatusBar(QWidget* parent)
    : QWidget(parent)
    , m_connected(false)
    , m_portName(QString())
    , m_baudRate(0)
{
    setupUI();
}

void ConnectionStatusBar::setupUI()
{
    setFixedHeight(30);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 2, 8, 2);
    layout->setSpacing(6);

    m_statusLabel = new QLabel();
    m_statusLabel->setStyleSheet("font-size: 12px; color: #888;");
    layout->addWidget(m_statusLabel);

    layout->addStretch();

    m_disconnectBtn = new QPushButton(tr("断开"));
    m_disconnectBtn->setFixedHeight(22);
    m_disconnectBtn->setStyleSheet(
        "QPushButton { background: #e74c3c; color: white; border: none; border-radius: 2px; "
        "padding: 2px 12px; font-size: 11px; }"
        "QPushButton:hover { background: #c0392b; }");
    m_disconnectBtn->hide();
    layout->addWidget(m_disconnectBtn);

    m_configBtn = new QPushButton(tr("⚙ 配置"));
    m_configBtn->setFixedHeight(22);
    m_configBtn->setStyleSheet(
        "QPushButton { background: #5dade2; color: white; border: none; border-radius: 2px; "
        "padding: 2px 12px; font-size: 11px; }"
        "QPushButton:hover { background: #3498db; }"
        "QPushButton:checked { background: #2980b9; }");
    m_configBtn->setCheckable(true);
    layout->addWidget(m_configBtn);

    setConnected(false);
    setStyleSheet("ConnectionStatusBar { background: #ecf0f1; border-bottom: 1px solid #bdc3c7; }");

    connect(m_disconnectBtn, &QPushButton::clicked, this, &ConnectionStatusBar::disconnectClicked);
    connect(m_configBtn, &QPushButton::clicked, this, &ConnectionStatusBar::configToggled);
}

void ConnectionStatusBar::setConnected(bool connected, const QString& portName, qint32 baudRate)
{
    m_connected = connected;
    m_portName = portName;
    m_baudRate = baudRate;

    if (connected && !portName.isEmpty()) {
        m_statusLabel->setText(QString("\342\232\241 %1 @ %2  |  \342\227\217 %3")
                                   .arg(portName)
                                   .arg(baudRate)
                                   .arg(tr("已连接")));
        m_statusLabel->setStyleSheet("font-size: 12px; color: #27ae60; font-weight: bold;");
        m_disconnectBtn->show();
    } else {
        m_statusLabel->setText(QString("\342\232\241 %1").arg(tr("未连接")));
        m_statusLabel->setStyleSheet("font-size: 12px; color: #888;");
        m_disconnectBtn->hide();
    }
}
