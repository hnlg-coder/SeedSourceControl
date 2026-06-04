#ifndef ALARMPANEL_H
#define ALARMPANEL_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QDateTime>

class AlarmPanel : public QWidget {
    Q_OBJECT
public:
    explicit AlarmPanel(QWidget* parent = nullptr);
    
public slots:
    void addAlarm(quint32 alarmCode, const QString& message);
    void clearAlarms();
    
private slots:
    void onClearButtonClicked();
    
private:
    void setupUI();
    
    QTableWidget* m_alarmTable;
    QPushButton* m_clearButton;
};

#endif // ALARMPANEL_H
