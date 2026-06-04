#ifndef REALTIMEPLOTPANEL_H
#define REALTIMEPLOTPANEL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QGroupBox>
#include "../visualization/realtimeplotwidget.h"

class RealtimePlotPanel : public QWidget {
    Q_OBJECT
public:
    explicit RealtimePlotPanel(QWidget* parent = nullptr);
    
public slots:
    void updateCurrent(double value);
    void updateTemperature(double value);
    void updatePower(double value);
    void clearData();
    
private:
    void setupUI();
    
    RealtimePlotWidget* m_plotWidget;
};

#endif // REALTIMEPLOTPANEL_H
