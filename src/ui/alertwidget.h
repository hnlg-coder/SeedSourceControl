#ifndef ALERTWIDGET_H
#define ALERTWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QGridLayout>
#include <QTimer>
#include "../model/devicedatamodel.h"

class AlertWidget : public QWidget {
    Q_OBJECT
public:
    explicit AlertWidget(QWidget* parent = nullptr);

public slots:
    void updateData(const DeviceDataModel::RealTimeData& data);
    void onAlarmTriggered(quint32 alarmCode, const QString& message);
    void clearAlarmHistory() { m_alarmHistoryTable->setRowCount(0); m_alarmCount = 0; }

private:
    void setupUI();
    void updateAlertIndicators(const DeviceDataModel::AlertBits& alert);

    // ALERT [07h] 8位指示灯
    QLabel* m_ccCtrlIndicator;
    QLabel* m_ccPdIndicator;
    QLabel* m_ccAdcIndicator;
    QLabel* m_ccDacIndicator;
    QLabel* m_tcCtrlIndicator;
    QLabel* m_tcAdcIndicator;
    QLabel* m_tcDacIndicator;
    QLabel* m_sysPwrIndicator;

    QLabel* m_ccCtrlLabel;
    QLabel* m_ccPdLabel;
    QLabel* m_ccAdcLabel;
    QLabel* m_ccDacLabel;
    QLabel* m_tcCtrlLabel;
    QLabel* m_tcAdcLabel;
    QLabel* m_tcDacLabel;
    QLabel* m_sysPwrLabel;

    // 报警历史
    QTableWidget* m_alarmHistoryTable;
    int m_alarmCount;
};

#endif // ALERTWIDGET_H
