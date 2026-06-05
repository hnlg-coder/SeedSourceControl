#ifndef TEMPERATURECONTROLWIDGET_H
#define TEMPERATURECONTROLWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include "../visualization/enhancedchartwidget.h"
#include "../model/devicedatamodel.h"

class TemperatureControlWidget : public QWidget {
    Q_OBJECT
public:
    explicit TemperatureControlWidget(QWidget* parent = nullptr);

public slots:
    void updateData(const DeviceDataModel::RealTimeData& data);
    void clearChartData() { m_chart->clearData(); }

signals:
    void temperatureSetChanged(double value);

private:
    void setupUI();

    QDoubleSpinBox* m_tempSetSpinBox;
    QLabel* m_tempValLabel;
    QLabel* m_tempStatusLabel;
    QPushButton* m_applyButton;

    EnhancedChartWidget* m_chart;
};

#endif // TEMPERATURECONTROLWIDGET_H
