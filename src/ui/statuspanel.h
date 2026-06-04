#ifndef STATUSPANEL_H
#define STATUSPANEL_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QGroupBox>

class StatusPanel : public QWidget {
    Q_OBJECT
public:
    explicit StatusPanel(QWidget* parent = nullptr);
    
public slots:
    void updateStatus(quint32 status);
    void updateCurrent(double current);
    void updateTemperature(double temperature);
    void updatePower(double power);
    void updateAlarm(quint32 alarm);
    
private:
    void setupUI();
    
    QLabel* m_statusLabel;
    QLabel* m_currentLabel;
    QLabel* m_temperatureLabel;
    QLabel* m_powerLabel;
    QLabel* m_alarmLabel;
    
    QLabel* m_statusValueLabel;
    QLabel* m_currentValueLabel;
    QLabel* m_temperatureValueLabel;
    QLabel* m_powerValueLabel;
    QLabel* m_alarmValueLabel;
};

#endif // STATUSPANEL_H
