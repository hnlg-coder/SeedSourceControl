#ifndef CONTROLPANEL_H
#define CONTROLPANEL_H

#include <QWidget>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QFormLayout>

class ControlPanel : public QWidget {
    Q_OBJECT
public:
    explicit ControlPanel(QWidget* parent = nullptr);
    
    double targetCurrent() const;
    double targetTemperature() const;
    double targetPower() const;
    
public slots:
    void setTargetCurrent(double current);
    void setTargetTemperature(double temperature);
    void setTargetPower(double power);
    
signals:
    void startClicked();
    void stopClicked();
    void resetClicked();
    void calibrateClicked();
    void currentChanged(double current);
    void temperatureChanged(double temperature);
    void powerChanged(double power);
    
private slots:
    void onStartClicked();
    void onStopClicked();
    void onResetClicked();
    void onCalibrateClicked();
    void onCurrentSpinBoxChanged(double value);
    void onTemperatureSpinBoxChanged(double value);
    void onPowerSpinBoxChanged(double value);
    
private:
    void setupUI();
    
    QPushButton* m_startButton;
    QPushButton* m_stopButton;
    QPushButton* m_resetButton;
    QPushButton* m_calibrateButton;
    
    QDoubleSpinBox* m_currentSpinBox;
    QDoubleSpinBox* m_temperatureSpinBox;
    QDoubleSpinBox* m_powerSpinBox;
};

#endif // CONTROLPANEL_H
