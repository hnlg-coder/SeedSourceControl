#ifndef CURRENTCONTROLWIDGET_H
#define CURRENTCONTROLWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include "../visualization/enhancedchartwidget.h"
#include "../model/devicedatamodel.h"

class CurrentControlWidget : public QWidget {
    Q_OBJECT
public:
    explicit CurrentControlWidget(QWidget* parent = nullptr);

public slots:
    void updateData(const DeviceDataModel::RealTimeData& data);
    void clearChartData() { m_chart->clearData(); }

signals:
    void currentSetChanged(double value);

private:
    void setupUI();

    QDoubleSpinBox* m_curSetSpinBox;
    QLabel* m_curValLabel;
    QLabel* m_curStatusLabel;
    QPushButton* m_applyButton;

    EnhancedChartWidget* m_chart;
};

#endif // CURRENTCONTROLWIDGET_H
