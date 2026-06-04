#include "alarmpanel.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QDateTime>

AlarmPanel::AlarmPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void AlarmPanel::addAlarm(quint32 alarmCode, const QString& message)
{
    int row = m_alarmTable->rowCount();
    m_alarmTable->insertRow(row);
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString codeStr = QString("0x%1").arg(alarmCode, 8, 16, QChar('0'));
    
    QTableWidgetItem* timeItem = new QTableWidgetItem(timestamp);
    QTableWidgetItem* codeItem = new QTableWidgetItem(codeStr);
    QTableWidgetItem* msgItem = new QTableWidgetItem(message);
    
    m_alarmTable->setItem(row, 0, timeItem);
    m_alarmTable->setItem(row, 1, codeItem);
    m_alarmTable->setItem(row, 2, msgItem);
    
    m_alarmTable->scrollToBottom();
}

void AlarmPanel::clearAlarms()
{
    m_alarmTable->setRowCount(0);
}

void AlarmPanel::onClearButtonClicked()
{
    clearAlarms();
}

void AlarmPanel::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QGroupBox* groupBox = new QGroupBox("Alarm History");
    QVBoxLayout* alarmLayout = new QVBoxLayout();
    
    m_alarmTable = new QTableWidget();
    m_alarmTable->setColumnCount(3);
    m_alarmTable->setHorizontalHeaderLabels(QStringList() << "Time" << "Code" << "Message");
    m_alarmTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_alarmTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_alarmTable->horizontalHeader()->setStretchLastSection(true);
    m_alarmTable->setAlternatingRowColors(true);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_clearButton = new QPushButton("Clear");
    
    buttonLayout->addWidget(m_clearButton);
    buttonLayout->addStretch();
    
    alarmLayout->addWidget(m_alarmTable);
    alarmLayout->addLayout(buttonLayout);
    
    groupBox->setLayout(alarmLayout);
    mainLayout->addWidget(groupBox);
    
    connect(m_clearButton, &QPushButton::clicked, this, &AlarmPanel::onClearButtonClicked);
}
