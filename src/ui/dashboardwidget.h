#ifndef DASHBOARDWIDGET_H
#define DASHBOARDWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QProgressBar>
#include "../model/devicedatamodel.h"

class DashboardWidget : public QWidget {
    Q_OBJECT
public:
    explicit DashboardWidget(QWidget* parent = nullptr);

public slots:
    void updateData(const DeviceDataModel::RealTimeData& data);

signals:
    void startClicked();
    void stopClicked();

private:
    void setupUI();
    QString statusColorStyle(const QString& color) const;

    // 状态指示
    QLabel* m_deviceStatusValue;
    QLabel* m_curStatusValue;
    QLabel* m_tempStatusValue;
    QLabel* m_alertStatusValue;

    // 关键数值
    QLabel* m_curSetValue;
    QLabel* m_curValValue;
    QLabel* m_tempSetValue;
    QLabel* m_tempValValue;
    QLabel* m_pwrLasValue;
    QLabel* m_pwrSysValue;

    // 快捷控制
    QPushButton* m_startButton;
    QPushButton* m_stopButton;
};

#endif // DASHBOARDWIDGET_H
