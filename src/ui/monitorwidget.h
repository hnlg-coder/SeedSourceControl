#ifndef MONITORWIDGET_H
#define MONITORWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QGridLayout>
#include "../visualization/enhancedchartwidget.h"
#include "../model/devicedatamodel.h"

class MonitorWidget : public QWidget {
    Q_OBJECT
public:
    explicit MonitorWidget(QWidget* parent = nullptr);

public slots:
    void updateData(const DeviceDataModel::RealTimeData& data);
    void clearChartData() { m_currentChart->clearData(); m_powerChart->clearData(); }

private:
    void setupUI();

    QLabel* m_curPdLabel;
    QLabel* m_curTecLabel;
    QLabel* m_pwrLasLabel;
    QLabel* m_pwrCcLabel;
    QLabel* m_pwrSysLabel;

    EnhancedChartWidget* m_currentChart;
    EnhancedChartWidget* m_powerChart;
};

#endif // MONITORWIDGET_H
