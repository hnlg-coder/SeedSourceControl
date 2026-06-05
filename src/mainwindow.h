#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QTabWidget>
#include <QSlider>
#include <QCheckBox>
#include <QGroupBox>
#include <QPropertyAnimation>
#include <QSharedPointer>
#include <atomic>

class ConnectionPanel;
class ConnectionStatusBar;
class LogPanel;
class DashboardWidget;
class CurrentControlWidget;
class TemperatureControlWidget;
class MonitorWidget;
class ConfigWidget;
class AlertWidget;
class DataTablePanel;
class SeedSourceProtocolParser;
class CommunicationWorker;
class SimulationWorker;
class DeviceDataModel;
class ICommand;
class QLabel;
class QGridLayout;

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
    void onCurrentSetChanged(double value);
    void onTemperatureSetChanged(double value);
    void onConfigChanged();

    void onConnectionStateChanged(bool connected);
    void onLogMessage(const QString& message);
    void onDataUpdated();
    void onAlarmTriggered(quint32 alarmCode, const QString& message);

    void pollDevice();

private:
    void setupUI();
    void createMenus();
    void connectSignals();
    void initializeComponents();
    void toggleDrawer();
    void toggleSimulationMode();
    void setupSimulationPanel();
    void updateSimulationDevinfo();
    void sendCommand(QSharedPointer<ICommand> cmd);
    void onCommandCompleted(quint32 cmdId, bool success, QVariant result);

    ConnectionStatusBar* m_connectionStatusBar;
    QWidget* m_connectionDrawer;
    QPropertyAnimation* m_drawerAnimation;
    ConnectionPanel* m_connectionPanel;

    // 页签容器
    QTabWidget* m_tabWidget;

    // 功能页签 Widget
    DashboardWidget* m_dashboardWidget;
    CurrentControlWidget* m_currentControlWidget;
    TemperatureControlWidget* m_temperatureControlWidget;
    MonitorWidget* m_monitorWidget;
    ConfigWidget* m_configWidget;
    AlertWidget* m_alertWidget;

    // 数据统计页签
    QWidget* m_statisticsTab;
    DataTablePanel* m_dataTablePanel;

    // 底部日志
    LogPanel* m_logPanel;

    // 通信组件
    CommunicationWorker* m_communicationWorker;
    SeedSourceProtocolParser* m_protocolParser;
    DeviceDataModel* m_dataModel;

    QTimer* m_pollTimer;
    std::atomic<bool> m_connected;
    std::atomic<bool> m_running;

    // 仿真模式
    QAction* m_simAction;
    bool m_simulationMode;
    SimulationWorker* m_simulationWorker;

    QWidget* m_simPanel;
    QCheckBox* m_faultEnable;
    QSlider* m_faultDropRate;
    QSlider* m_faultCorruptRate;
    QSlider* m_faultDelay;
    QCheckBox* m_faultCheckSum;
    QSlider* m_simSlewRate;
    QSlider* m_simNoise;
    QSlider* m_simStartupDelay;
    QSlider* m_simTempLag;

    QLabel* m_devDatecode;
    QLabel* m_devHwVer;
    QLabel* m_devFwVer;
    QLabel* m_devSerial;
    QLabel* m_devMaxCur;
    QLabel* m_devMaxTemp;
    QLabel* m_devMaxPower;
};

#endif // MAINWINDOW_H