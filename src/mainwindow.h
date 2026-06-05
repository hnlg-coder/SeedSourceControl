#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QTabWidget>
#include <QSplitter>

// Forward declarations
class ConnectionPanel;
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

    // 顶部串口配置
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
    bool m_connected;
    bool m_running;
};

#endif // MAINWINDOW_H