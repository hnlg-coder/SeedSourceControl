#include "mainwindow.h"
#include "ui/connectionpanel.h"
#include "ui/logpanel.h"
#include "ui/dashboardwidget.h"
#include "ui/currentcontrolwidget.h"
#include "ui/temperaturecontrolwidget.h"
#include "ui/monitorwidget.h"
#include "ui/configwidget.h"
#include "ui/alertwidget.h"
#include "ui/datatablepanel.h"
#include "protocol/protocolparser.h"
#include "communication/communicationworker.h"
#include "model/devicedatamodel.h"
#include "communication/command.h"
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QDebug>
#include <QLabel>
#include <QDateTime>
#include "config/configmanager.h"
#include "database/databasemanager.h"
#include "alarm/alarmmanager.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_tabWidget(nullptr)
    , m_dashboardWidget(nullptr)
    , m_currentControlWidget(nullptr)
    , m_temperatureControlWidget(nullptr)
    , m_monitorWidget(nullptr)
    , m_configWidget(nullptr)
    , m_alertWidget(nullptr)
    , m_statisticsTab(nullptr)
    , m_dataTablePanel(nullptr)
    , m_connected(false)
    , m_running(false)
{
    setupUI();
    createMenus();
    initializeComponents();
    connectSignals();

    setWindowTitle(tr("种子源模块控制器 - v2.0"));
    resize(1280, 900);
    setMinimumSize(800, 600);
}

MainWindow::~MainWindow()
{
    m_running = false;
    m_connected = false;

    if (m_pollTimer) {
        m_pollTimer->stop();
        delete m_pollTimer;
        m_pollTimer = nullptr;
    }

    if (m_communicationWorker) {
        m_communicationWorker->stopCommunication();
        disconnect(m_communicationWorker, nullptr, this, nullptr);
        m_communicationWorker->wait();
        delete m_communicationWorker;
        m_communicationWorker = nullptr;
    }

    DatabaseManager::instance()->close();
}

void MainWindow::setupUI()
{
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(2);

    // === 顶部：串口连接面板 ===
    m_connectionPanel = new ConnectionPanel();
    mainLayout->addWidget(m_connectionPanel);

    // === 中间：QTabWidget 功能页签 ===
    m_tabWidget = new QTabWidget();

    // 页签1: 总览
    m_dashboardWidget = new DashboardWidget();
    m_tabWidget->addTab(m_dashboardWidget, tr("总览"));

    // 页签2: 电流控制
    m_currentControlWidget = new CurrentControlWidget();
    m_tabWidget->addTab(m_currentControlWidget, tr("电流控制"));

    // 页签3: 温度控制
    m_temperatureControlWidget = new TemperatureControlWidget();
    m_tabWidget->addTab(m_temperatureControlWidget, tr("温度控制"));

    // 页签4: 功率监测
    m_monitorWidget = new MonitorWidget();
    m_tabWidget->addTab(m_monitorWidget, tr("功率监测"));

    // 页签5: 设备配置
    m_configWidget = new ConfigWidget();
    m_tabWidget->addTab(m_configWidget, tr("设备配置"));

    // 页签6: 报警管理
    m_alertWidget = new AlertWidget();
    m_tabWidget->addTab(m_alertWidget, tr("报警管理"));

    // 页签7: 数据统计（DataTablePanel + 统计信息）
    m_statisticsTab = new QWidget();
    QVBoxLayout* statLayout = new QVBoxLayout(m_statisticsTab);
    QLabel* statHeaderLabel = new QLabel(tr("数据统计 - 历史数据记录与导出"));
    statHeaderLabel->setStyleSheet("font-weight: bold; font-size: 13px; padding: 4px;");
    statLayout->addWidget(statHeaderLabel);
    m_dataTablePanel = new DataTablePanel();
    statLayout->addWidget(m_dataTablePanel);
    m_tabWidget->addTab(m_statisticsTab, tr("数据统计"));

    mainLayout->addWidget(m_tabWidget, 1);

    // 日志面板：创建后添加到总览页签
    m_logPanel = new LogPanel();
    m_logPanel->setMinimumHeight(120);
    m_dashboardWidget->setLogPanel(m_logPanel);

    // 状态栏
    statusBar()->showMessage(tr("就绪"));
}

void MainWindow::createMenus()
{
    QMenu* fileMenu = menuBar()->addMenu(tr("文件(&F)"));

    QAction* exitAction = new QAction(tr("退出(&X)"), this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
    fileMenu->addAction(exitAction);

    QMenu* helpMenu = menuBar()->addMenu(tr("帮助(&H)"));

    QAction* aboutAction = new QAction(tr("关于(&A)"), this);
    connect(aboutAction, &QAction::triggered, [this]() {
        QMessageBox::about(this, tr("关于"), tr("种子源模块控制器 v2.0\n基于Qt框架开发\n支持协议V1.3全寄存器数据展示"));
    });
    helpMenu->addAction(aboutAction);
}

void MainWindow::initializeComponents()
{
    ConfigManager::instance();
    DatabaseManager::instance()->initialize(ConfigManager::instance()->databasePath());
    AlarmManager::instance();

    m_protocolParser = new SeedSourceProtocolParser(this);
    m_dataModel = new DeviceDataModel(this);
    m_communicationWorker = new CommunicationWorker(this);

    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(200);

    m_connectionPanel->refreshPorts();
}

void MainWindow::connectSignals()
{
    // 连接面板信号
    connect(m_connectionPanel, &ConnectionPanel::connectClicked, this, &MainWindow::onConnectClicked);
    connect(m_connectionPanel, &ConnectionPanel::disconnectClicked, this, &MainWindow::onDisconnectClicked);

    // 总览页签 - 启停控制
    connect(m_dashboardWidget, &DashboardWidget::startClicked, this, &MainWindow::onStartClicked);
    connect(m_dashboardWidget, &DashboardWidget::stopClicked, this, &MainWindow::onStopClicked);

    // 电流控制 - 设定值变更
    connect(m_currentControlWidget, &CurrentControlWidget::currentSetChanged,
            this, &MainWindow::onCurrentSetChanged);

    // 温度控制 - 设定值变更
    connect(m_temperatureControlWidget, &TemperatureControlWidget::temperatureSetChanged,
            this, &MainWindow::onTemperatureSetChanged);

    // 设备配置 - 配置变更
    connect(m_configWidget, &ConfigWidget::configChanged, this, &MainWindow::onConfigChanged);

    // 通信工作线程信号
    connect(m_communicationWorker, &CommunicationWorker::connectionStateChanged,
            this, &MainWindow::onConnectionStateChanged);
    connect(m_communicationWorker, &CommunicationWorker::logMessage,
            this, &MainWindow::onLogMessage);

    // 命令完成信号 -> 更新数据模型
    connect(m_communicationWorker, &CommunicationWorker::commandCompleted,
            this, [this](quint32 cmdId, bool success, QVariant result) {
        Q_UNUSED(cmdId);
        if (success && result.isValid()) {
            QVariantMap statusMap = result.toMap();

            // 从ReadStatusCommand响应中提取原始寄存器值
            double current = statusMap.value("current", 0).toDouble();
            double temperature = statusMap.value("temperature", 0).toDouble();
            double power = statusMap.value("power", 0).toDouble();
            quint32 status = statusMap.value("status", 0).toUInt();
            quint32 alarm = statusMap.value("alarm", 0).toUInt();

            // 值域转换（假设协议中单位为0.01mA, 0.001°C, 0.01mW）
            double currentVal = current / 100.0;
            double tempVal = temperature / 1000.0;
            double powerLas = power / 100.0;

            m_dataModel->updateData(currentVal, tempVal, powerLas, status, alarm);
        }
    });

    // 数据模型信号 -> UI更新
    connect(m_dataModel, &DeviceDataModel::dataUpdated, this, [this](const DeviceDataModel::RealTimeData& data) {
        // 分发给所有 Widget
        m_dashboardWidget->updateData(data);
        m_currentControlWidget->updateData(data);
        m_temperatureControlWidget->updateData(data);
        m_monitorWidget->updateData(data);
        m_configWidget->updateData(data);
        m_alertWidget->updateData(data);
        m_dataTablePanel->addDataRow(data);

        // 更新状态栏
        QString statusMsg = data.status.idle
            ? tr("状态: 空闲")
            : tr("状态: 运行中 | 电流: %1 mA | 温度: %2 °C | 功率: %3 mW")
                  .arg(data.curVal, 0, 'f', 2)
                  .arg(data.tempVal, 0, 'f', 3)
                  .arg(data.pwrLas, 0, 'f', 2);
        statusBar()->showMessage(statusMsg);
    });

    connect(m_dataModel, &DeviceDataModel::alarmTriggered, this, [this](quint32 alarmCode, const QString& message) {
        m_alertWidget->onAlarmTriggered(alarmCode, message);
        m_logPanel->logMessage(tr("报警: %1").arg(message), LogPanel::Error);

        // 切换到报警页签
        int alertIdx = m_tabWidget->indexOf(m_alertWidget);
        if (alertIdx >= 0) {
            m_tabWidget->setCurrentIndex(alertIdx);
        }
    });

    // 定时器信号
    connect(m_pollTimer, &QTimer::timeout, this, &MainWindow::pollDevice);
}

void MainWindow::onConnectClicked()
{
    QString portName = m_connectionPanel->selectedPort();
    qint32 baudRate = m_connectionPanel->selectedBaudRate();
    QSerialPort::DataBits dataBits = m_connectionPanel->selectedDataBits();
    QSerialPort::Parity parity = m_connectionPanel->selectedParity();
    QSerialPort::StopBits stopBits = m_connectionPanel->selectedStopBits();
    QSerialPort::FlowControl flowControl = m_connectionPanel->selectedFlowControl();

    m_communicationWorker->setSerialConfig(portName, baudRate, dataBits, parity, stopBits, flowControl);
    m_communicationWorker->startCommunication();
    m_communicationWorker->connectDevice();

    m_logPanel->logMessage(tr("尝试连接设备: %1, 波特率: %2").arg(portName).arg(baudRate), LogPanel::Info);
}

void MainWindow::onDisconnectClicked()
{
    if (m_running) {
        onStopClicked();
    }

    m_communicationWorker->disconnectDevice();
    m_logPanel->logMessage(tr("设备断开连接"), LogPanel::Info);
}

void MainWindow::onStartClicked()
{
    if (!m_connected) {
        QMessageBox::warning(this, tr("警告"), tr("请先连接设备！"));
        return;
    }

    m_running = true;
    m_pollTimer->start();

    auto command = CommandFactory::createControlDeviceCommand(ControlDeviceCommand::Start, m_protocolParser);
    m_communicationWorker->sendCommand(command);

    m_logPanel->logMessage(tr("设备启动"), LogPanel::Info);
    statusBar()->showMessage(tr("运行中..."));
}

void MainWindow::onStopClicked()
{
    m_running = false;
    m_pollTimer->stop();

    auto command = CommandFactory::createControlDeviceCommand(ControlDeviceCommand::Stop, m_protocolParser);
    m_communicationWorker->sendCommand(command);

    m_logPanel->logMessage(tr("设备停止"), LogPanel::Info);
    statusBar()->showMessage(tr("已停止"));
}

void MainWindow::onResetClicked()
{
    auto command = CommandFactory::createControlDeviceCommand(ControlDeviceCommand::Reset, m_protocolParser);
    m_communicationWorker->sendCommand(command);

    m_logPanel->logMessage(tr("设备重置"), LogPanel::Info);
}

void MainWindow::onCalibrateClicked()
{
    auto command = CommandFactory::createControlDeviceCommand(ControlDeviceCommand::Calibrate, m_protocolParser);
    m_communicationWorker->sendCommand(command);

    m_logPanel->logMessage(tr("设备校准"), LogPanel::Info);
}

void MainWindow::onCurrentSetChanged(double value)
{
    if (m_connected && m_running) {
        auto command = CommandFactory::createWriteRegisterCommand(0x02, 0x00, static_cast<quint32>(value * 100), m_protocolParser);
        m_communicationWorker->sendCommand(command);

        m_logPanel->logMessage(tr("设置目标电流: %1 mA").arg(value), LogPanel::Info);
    }
}

void MainWindow::onTemperatureSetChanged(double value)
{
    if (m_connected && m_running) {
        auto command = CommandFactory::createWriteRegisterCommand(0x03, 0x00, static_cast<quint32>(value * 1000), m_protocolParser);
        m_communicationWorker->sendCommand(command);

        m_logPanel->logMessage(tr("设置目标温度: %1 °C").arg(value), LogPanel::Info);
    }
}

void MainWindow::onConfigChanged()
{
    DeviceDataModel::ConfigBits cfg = m_configWidget->getConfigData();

    // 构建 CONFIG 寄存器写入数据 (bit0-5 + 保留位)
    quint32 configReg = 0;
    if (cfg.tcEn)    configReg |= 0x01;
    if (cfg.ccEn)    configReg |= 0x02;
    if (cfg.aeEn)    configReg |= 0x04;
    if (cfg.powerSv) configReg |= 0x08;
    if (cfg.curSv)   configReg |= 0x10;
    if (cfg.tempSv)  configReg |= 0x20;

    if (m_connected) {
        auto command = CommandFactory::createWriteRegisterCommand(0x05, 0x00, configReg, m_protocolParser);
        m_communicationWorker->sendCommand(command);
        m_logPanel->logMessage(
            tr("配置已更新: TC_EN=%1 CC_EN=%2 AE_EN=%3").arg(cfg.tcEn).arg(cfg.ccEn).arg(cfg.aeEn),
            LogPanel::Info);
    } else {
        m_logPanel->logMessage(tr("配置变更将在设备连接后生效"), LogPanel::Warning);
    }
}

void MainWindow::onConnectionStateChanged(bool connected)
{
    m_connected = connected;
    m_connectionPanel->setConnected(connected);

    if (connected) {
        m_logPanel->logMessage(tr("设备已连接"), LogPanel::Info);
        statusBar()->showMessage(tr("已连接"));
    } else {
        m_logPanel->logMessage(tr("设备已断开"), LogPanel::Warning);
        statusBar()->showMessage(tr("已断开"));
        m_running = false;
        m_pollTimer->stop();
    }
}

void MainWindow::onLogMessage(const QString& message)
{
    m_logPanel->logMessage(message, LogPanel::Info);
}

void MainWindow::onDataUpdated()
{
    // 此方法已通过 lambda 连接处理，保留用于兼容
}

void MainWindow::onAlarmTriggered(quint32 alarmCode, const QString& message)
{
    // 此方法已通过 lambda 连接处理，保留用于兼容
}

void MainWindow::pollDevice()
{
    if (!m_connected) {
        return;
    }

    // 发送读状态命令获取所有寄存器数据
    auto command = CommandFactory::createReadStatusCommand(m_protocolParser);
    m_communicationWorker->sendCommand(command);
}