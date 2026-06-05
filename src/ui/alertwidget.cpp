#include "alertwidget.h"
#include <QHeaderView>
#include <QDateTime>

AlertWidget::AlertWidget(QWidget* parent)
    : QWidget(parent)
    , m_alarmCount(0)
{
    setupUI();
}

void AlertWidget::updateData(const DeviceDataModel::RealTimeData& data)
{
    updateAlertIndicators(data.alert);
}

void AlertWidget::onAlarmTriggered(quint32 alarmCode, const QString& message)
{
    int row = m_alarmHistoryTable->rowCount();
    m_alarmHistoryTable->insertRow(row);
    m_alarmHistoryTable->setItem(row, 0, new QTableWidgetItem(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
    m_alarmHistoryTable->setItem(row, 1, new QTableWidgetItem(QString("0x%1").arg(alarmCode, 2, 16, QChar('0'))));
    m_alarmHistoryTable->setItem(row, 2, new QTableWidgetItem(message));

    // 颜色标记
    QColor bgColor(255, 230, 230);
    for (int col = 0; col < 3; ++col) {
        m_alarmHistoryTable->item(row, col)->setBackground(bgColor);
    }

    m_alarmHistoryTable->scrollToBottom();
    m_alarmCount++;
}

void AlertWidget::updateAlertIndicators(const DeviceDataModel::AlertBits& alert)
{
    auto setIndicator = [](QLabel* indicator, QLabel* label, bool active) {
        if (active) {
            indicator->setStyleSheet("background-color: #f44336; border-radius: 10px; min-width: 20px; max-width: 20px; min-height: 20px; max-height: 20px;");
            label->setStyleSheet("color: #f44336; font-weight: bold;");
        } else {
            indicator->setStyleSheet("background-color: #4CAF50; border-radius: 10px; min-width: 20px; max-width: 20px; min-height: 20px; max-height: 20px;");
            label->setStyleSheet("color: #4CAF50; font-weight: bold;");
        }
    };

    setIndicator(m_ccCtrlIndicator, m_ccCtrlLabel, alert.ccCtrl);
    setIndicator(m_ccPdIndicator, m_ccPdLabel, alert.ccPd);
    setIndicator(m_ccAdcIndicator, m_ccAdcLabel, alert.ccAdc);
    setIndicator(m_ccDacIndicator, m_ccDacLabel, alert.ccDac);
    setIndicator(m_tcCtrlIndicator, m_tcCtrlLabel, alert.tcCtrl);
    setIndicator(m_tcAdcIndicator, m_tcAdcLabel, alert.tcAdc);
    setIndicator(m_tcDacIndicator, m_tcDacLabel, alert.tcDac);
    setIndicator(m_sysPwrIndicator, m_sysPwrLabel, alert.sysPwr);
}

void AlertWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // === ALERT寄存器 [07h] 指示灯 ===
    QGroupBox* alertGroup = new QGroupBox("报警状态寄存器 [07h]");
    QGridLayout* alertLayout = new QGridLayout();

    auto createIndicator = [](QLabel** indicator, QLabel** label, const QString& name, int row, int col, QGridLayout* layout) {
        *indicator = new QLabel();
        (*indicator)->setStyleSheet("background-color: #4CAF50; border-radius: 10px; min-width: 20px; max-width: 20px; min-height: 20px; max-height: 20px;");
        (*indicator)->setAlignment(Qt::AlignCenter);
        *label = new QLabel(name);
        (*label)->setStyleSheet("color: #4CAF50; font-weight: bold;");
        layout->addWidget(*indicator, row, col * 2);
        layout->addWidget(*label, row, col * 2 + 1);
    };

    createIndicator(&m_ccCtrlIndicator, &m_ccCtrlLabel, "恒流控制 (CC_CTRL)", 0, 0, alertLayout);
    createIndicator(&m_ccPdIndicator, &m_ccPdLabel, "光电二极管 (CC_PD)", 0, 1, alertLayout);
    createIndicator(&m_ccAdcIndicator, &m_ccAdcLabel, "恒流ADC (CC_ADC)", 0, 2, alertLayout);
    createIndicator(&m_ccDacIndicator, &m_ccDacLabel, "恒流DAC (CC_DAC)", 0, 3, alertLayout);
    createIndicator(&m_tcCtrlIndicator, &m_tcCtrlLabel, "温控 (TC_CTRL)", 1, 0, alertLayout);
    createIndicator(&m_tcAdcIndicator, &m_tcAdcLabel, "温控ADC (TC_ADC)", 1, 1, alertLayout);
    createIndicator(&m_tcDacIndicator, &m_tcDacLabel, "温控DAC (TC_DAC)", 1, 2, alertLayout);
    createIndicator(&m_sysPwrIndicator, &m_sysPwrLabel, "系统电源 (SYS_PWR)", 1, 3, alertLayout);

    alertGroup->setLayout(alertLayout);
    mainLayout->addWidget(alertGroup);

    // === 报警历史 ===
    QGroupBox* historyGroup = new QGroupBox("报警历史");
    QVBoxLayout* historyLayout = new QVBoxLayout();

    m_alarmHistoryTable = new QTableWidget();
    m_alarmHistoryTable->setColumnCount(3);
    m_alarmHistoryTable->setHorizontalHeaderLabels({"时间", "报警码", "描述"});
    m_alarmHistoryTable->horizontalHeader()->setStretchLastSection(true);
    m_alarmHistoryTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_alarmHistoryTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_alarmHistoryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_alarmHistoryTable->setAlternatingRowColors(true);

    historyLayout->addWidget(m_alarmHistoryTable);
    historyGroup->setLayout(historyLayout);
    mainLayout->addWidget(historyGroup, 1);
}
