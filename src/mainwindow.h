#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

// Forward declarations
class ConnectionPanel;
class StatusPanel;
class ControlPanel;
class RealtimePlotPanel;
class AlarmPanel;
class LogPanel;
class SeedSourceProtocolParser;
class CommunicationWorker;
class DeviceDataModel;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void onStartClicked();
    void onStopClicked();
    void onResetClicked();
    void onCalibrateClicked();
    void onCurrentChanged(double value);
    void onTemperatureChanged(double value);
    void onPowerChanged(double value);
    
    void onConnectionStateChanged(bool connected);
    void onLogMessage(const QString& message);
    void onDataUpdated();
    void onAlarmTriggered(quint32 alarmCode, const QString& message);
    
    void pollDevice();

private:
    void setupUI();
    void createMenus();
    void createToolbar();
    void connectSignals();
    void initializeComponents();
    
    ConnectionPanel* m_connectionPanel;
    StatusPanel* m_statusPanel;
    ControlPanel* m_controlPanel;
    RealtimePlotPanel* m_plotPanel;
    AlarmPanel* m_alarmPanel;
    LogPanel* m_logPanel;
    
    CommunicationWorker* m_communicationWorker;
    SeedSourceProtocolParser* m_protocolParser;
    DeviceDataModel* m_dataModel;
    
    QTimer* m_pollTimer;
    bool m_connected;
    bool m_running;
};

#endif // MAINWINDOW_H
