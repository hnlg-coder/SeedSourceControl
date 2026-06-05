#include "mainwindow.h"
#include "ui/connectionpanel.h"
#include "ui/connectionstatusbar.h"
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
#include "simulator/simulationworker.h"
#include "simulator/simdevicestatemachine.h"
#include "simulator/faultinjector.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_connectionStatusBar(nullptr)
    , m_connectionDrawer(nullptr)
    , m_drawerAnimation(nullptr)
    , m_connectionPanel(nullptr)
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
    , m_simulationWorker(nullptr)
    , m_simulationMode(false)
    , m_simAction(nullptr)
    , m_simPanel(nullptr)
    , m_faultEnable(nullptr)
    , m_faultDropRate(nullptr)
    , m_faultCorruptRate(nullptr)
    , m_faultDelay(nullptr)
    , m_faultCheckSum(nullptr)
    , m_simSlewRate(nullptr)
    , m_simNoise(nullptr)
    , m_simStartupDelay(nullptr)
    , m_simTempLag(nullptr)
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

    if (m_simulationWorker) {
        m_simulationWorker->stopCommunication();
        disconnect(m_simulationWorker, nullptr, this, nullptr);
        delete m_simulationWorker;
        m_simulationWorker = nullptr;
    }

    DatabaseManager::instance()->close();
}

void MainWindow::setupUI()
{
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // === 顶部：连接状态条 ===
    m_connectionStatusBar = new ConnectionStatusBar();
    mainLayout->addWidget(m_connectionStatusBar);

    // === 中间：抽屉面板 + Tab功能区 ===
    QHBoxLayout* bodyLayout = new QHBoxLayout();
    bodyLayout->setContentsMargins(2, 2, 2, 2);
    bodyLayout->setSpacing(0);

    // 左侧抽屉面板（默认收起）
    m_connectionDrawer = new QWidget();
    m_connectionDrawer->setObjectName("connectionDrawer");
    m_connectionDrawer->setFixedWidth(0);
    m_connectionDrawer->setMaximumWidth(0);
    m_connectionDrawer->setStyleSheet(
        "QWidget#connectionDrawer { background: #f8f9fa; border-right: 1px solid #dee2e6; }");

    QVBoxLayout* drawerLayout = new QVBoxLayout(m_connectionDrawer);
    drawerLayout->setContentsMargins(0, 0, 0, 0);
    drawerLayout->setSpacing(0);

    m_connectionPanel = new ConnectionPanel();
    drawerLayout->addWidget(m_connectionPanel);

    bodyLayout->addWidget(m_connectionDrawer);

    // 右侧 Tab 功能区
    m_tabWidget = new QTabWidget();

    m_dashboardWidget = new DashboardWidget();
    m_tabWidget->addTab(m_dashboardWidget, tr("总览"));

    m_currentControlWidget = new CurrentControlWidget();
    m_tabWidget->addTab(m_currentControlWidget, tr("电流控制"));

    m_temperatureControlWidget = new TemperatureControlWidget();
    m_tabWidget->addTab(m_temperatureControlWidget, tr("温度控制"));

    m_monitorWidget = new MonitorWidget();
    m_tabWidget->addTab(m_monitorWidget, tr("功率监测"));

    m_configWidget = new ConfigWidget();
    m_tabWidget->addTab(m_configWidget, tr("设备配置"));

    m_alertWidget = new AlertWidget();
    m_tabWidget->addTab(m_alertWidget, tr("报警管理"));

    m_statisticsTab = new QWidget();
    QVBoxLayout* statLayout = new QVBoxLayout(m_statisticsTab);
    QLabel* statHeaderLabel = new QLabel(tr("数据统计 - 历史数据记录与导出"));
    statHeaderLabel->setStyleSheet("font-weight: bold; font-size: 13px; padding: 4px;");
    statLayout->addWidget(statHeaderLabel);
    m_dataTablePanel = new DataTablePanel();
    statLayout->addWidget(m_dataTablePanel);
    m_tabWidget->addTab(m_statisticsTab, tr("数据统计"));

    bodyLayout->addWidget(m_tabWidget, 1);
    mainLayout->addLayout(bodyLayout, 1);

    // 抽屉动画
    m_drawerAnimation = new QPropertyAnimation(m_connectionDrawer, "maximumWidth", this);
    m_drawerAnimation->setDuration(200);
    m_drawerAnimation->setEasingCurve(QEasingCurve::OutCubic);

    // 日志面板：创建后添加到总览页签
    m_logPanel = new LogPanel();
    m_logPanel->setMinimumHeight(120);
    m_dashboardWidget->setLogPanel(m_logPanel);

    statusBar()->showMessage(tr("就绪"));
}

void MainWindow::createMenus()
{
    QMenu* fileMenu = menuBar()->addMenu(tr("文件(&F)"));

    QAction* exitAction = new QAction(tr("退出(&X)"), this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
    fileMenu->addAction(exitAction);

    QMenu* debugMenu = menuBar()->addMenu(tr("调试(&D)"));

    m_simAction = new QAction(tr("仿真模式(&S)"), this);
    m_simAction->setCheckable(true);
    m_simAction->setChecked(false);
    connect(m_simAction, &QAction::triggered, this, &MainWindow::toggleSimulationMode);
    debugMenu->addAction(m_simAction);

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
    m_communicationWorker->setProtocolParser(m_protocolParser);

    m_simulationWorker = new SimulationWorker(this);
    m_simulationWorker->setProtocolParser(m_protocolParser);

    // 注册跨线程信号槽传递的类型
    qRegisterMetaType<QSharedPointer<ICommand>>("QSharedPointer<ICommand>");

    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(200);

    m_connectionPanel->refreshPorts();
}

void MainWindow::connectSignals()
{
    // 连接面板信号
    connect(m_connectionPanel, &ConnectionPanel::connectClicked, this, &MainWindow::onConnectClicked);
    connect(m_connectionPanel, &ConnectionPanel::disconnectClicked, this, &MainWindow::onDisconnectClicked);

    // 状态条 - 配置/断开按钮
    connect(m_connectionStatusBar, &ConnectionStatusBar::configToggled,
            this, &MainWindow::toggleDrawer);
    connect(m_connectionStatusBar, &ConnectionStatusBar::disconnectClicked,
            this, &MainWindow::onDisconnectClicked);

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
    connect(m_communicationWorker, &CommunicationWorker::commandCompleted,
            this, &MainWindow::onCommandCompleted);

    // 仿真工作线程信号
    connect(m_simulationWorker, &SimulationWorker::connectionStateChanged,
            this, &MainWindow::onConnectionStateChanged);
    connect(m_simulationWorker, &SimulationWorker::logMessage,
            this, &MainWindow::onLogMessage);
    connect(m_simulationWorker, &SimulationWorker::commandCompleted,
            this, &MainWindow::onCommandCompleted);

    // 命令完成信号 -> 更新数据模型
    // (handled via onCommandCompleted slot, connected above)

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
    if (m_simulationMode) {
        m_simulationWorker->startCommunication();
        m_simulationWorker->connectDevice();
        m_logPanel->logMessage(tr("仿真模式: 连接虚拟设备"), LogPanel::Info);
        return;
    }

    QString portName = m_connectionPanel->selectedPort();
    qint32 baudRate = m_connectionPanel->selectedBaudRate();
    QSerialPort::DataBits dataBits = m_connectionPanel->selectedDataBits();
    QSerialPort::Parity parity = m_connectionPanel->selectedParity();
    QSerialPort::StopBits stopBits = m_connectionPanel->selectedStopBits();
    QSerialPort::FlowControl flowControl = m_connectionPanel->selectedFlowControl();

    m_communicationWorker->setSerialConfig(portName, baudRate, dataBits, parity, stopBits, flowControl);
    m_communicationWorker->startCommunication();
    m_communicationWorker->connectDevice();

    m_connectionStatusBar->setConnected(false);
    m_logPanel->logMessage(tr("尝试连接设备: %1, 波特率: %2").arg(portName).arg(baudRate), LogPanel::Info);
}

void MainWindow::onDisconnectClicked()
{
    if (m_running) {
        onStopClicked();
    }

    if (m_simulationMode) {
        m_simulationWorker->disconnectDevice();
    } else {
        m_communicationWorker->disconnectDevice();
    }
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
    sendCommand(command);

    m_logPanel->logMessage(tr("设备启动"), LogPanel::Info);
    statusBar()->showMessage(tr("运行中..."));
}

void MainWindow::onStopClicked()
{
    m_running = false;
    m_pollTimer->stop();

    auto command = CommandFactory::createControlDeviceCommand(ControlDeviceCommand::Stop, m_protocolParser);
    sendCommand(command);

    m_logPanel->logMessage(tr("设备停止"), LogPanel::Info);
    statusBar()->showMessage(tr("已停止"));
}

void MainWindow::onResetClicked()
{
    auto command = CommandFactory::createControlDeviceCommand(ControlDeviceCommand::Reset, m_protocolParser);
    sendCommand(command);

    m_logPanel->logMessage(tr("设备重置"), LogPanel::Info);
}

void MainWindow::onCalibrateClicked()
{
    auto command = CommandFactory::createControlDeviceCommand(ControlDeviceCommand::Calibrate, m_protocolParser);
    sendCommand(command);

    m_logPanel->logMessage(tr("设备校准"), LogPanel::Info);
}

void MainWindow::onCurrentSetChanged(double value)
{
    if (m_connected && m_running) {
        auto command = CommandFactory::createWriteRegisterCommand(0x02, 0x00, static_cast<quint32>(value * 100), m_protocolParser);
        sendCommand(command);

        m_logPanel->logMessage(tr("设置目标电流: %1 mA").arg(value), LogPanel::Info);
    }
}

void MainWindow::onTemperatureSetChanged(double value)
{
    if (m_connected && m_running) {
        auto command = CommandFactory::createWriteRegisterCommand(0x03, 0x00, static_cast<quint32>(value * 1000), m_protocolParser);
        sendCommand(command);

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
        sendCommand(command);
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
        QString portName = m_connectionPanel->selectedPort();
        qint32 baudRate = m_connectionPanel->selectedBaudRate();
        m_connectionStatusBar->setConnected(true, portName, baudRate);
        // 关闭抽屉
        m_connectionDrawer->setMaximumWidth(0);
        m_connectionDrawer->setFixedWidth(0);
        m_logPanel->logMessage(tr("设备已连接"), LogPanel::Info);
        statusBar()->showMessage(tr("已连接"));
    } else {
        m_connectionStatusBar->setConnected(false);
        // 自动展开抽屉，方便重新配置
        m_connectionDrawer->setFixedWidth(220);
        m_drawerAnimation->setStartValue(0);
        m_drawerAnimation->setEndValue(220);
        m_drawerAnimation->start();
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

void MainWindow::toggleDrawer()
{
    int drawerWidth = 220;
    if (m_connectionDrawer->maximumWidth() > 0) {
        m_drawerAnimation->setStartValue(drawerWidth);
        m_drawerAnimation->setEndValue(0);
        m_drawerAnimation->start();
    } else {
        m_connectionDrawer->setFixedWidth(drawerWidth);
        m_drawerAnimation->setStartValue(0);
        m_drawerAnimation->setEndValue(drawerWidth);
        m_drawerAnimation->start();
    }
}

void MainWindow::pollDevice()
{
    if (!m_connected) {
        return;
    }

    // 发送读状态命令获取所有寄存器数据
    auto command = CommandFactory::createReadStatusCommand(m_protocolParser);
    sendCommand(command);
}

void MainWindow::onCommandCompleted(quint32 cmdId, bool success, QVariant result)
{
    Q_UNUSED(cmdId);
    if (success && result.isValid()) {
        QVariantMap statusMap = result.toMap();

        double current = statusMap.value("current", 0).toDouble();
        double temperature = statusMap.value("temperature", 0).toDouble();
        double power = statusMap.value("power", 0).toDouble();
        quint32 status = statusMap.value("status", 0).toUInt();
        quint32 alarm = statusMap.value("alarm", 0).toUInt();

        double currentVal = current / 100.0;
        double tempVal = temperature / 1000.0;
        double powerLas = power / 100.0;

        m_dataModel->updateData(currentVal, tempVal, powerLas, status, alarm);
    }
}

void MainWindow::sendCommand(QSharedPointer<ICommand> cmd)
{
    if (m_simulationMode) {
        m_simulationWorker->sendCommand(cmd);
    } else {
        m_communicationWorker->sendCommand(cmd);
    }
}

void MainWindow::toggleSimulationMode()
{
    m_simulationMode = m_simAction->isChecked();
    m_logPanel->logMessage(
        m_simulationMode ? tr("切换至仿真模式") : tr("切换至实物模式"),
        LogPanel::Info);
    statusBar()->showMessage(
        m_simulationMode ? tr("仿真模式 - 无需物理连接") : tr("实物模式"));
    m_connectionPanel->setEnabled(!m_simulationMode);

    if (!m_simPanel) {
        setupSimulationPanel();
    }
    m_simPanel->setVisible(m_simulationMode);
}

void MainWindow::setupSimulationPanel()
{
    m_simPanel = new QWidget(this);
    m_simPanel->setStyleSheet(
        "QWidget#simPanel { background: #e8f4fd; border-bottom: 1px solid #b8d8ea; }");

    QHBoxLayout* panelLayout = new QHBoxLayout(m_simPanel);
    panelLayout->setContentsMargins(8, 4, 8, 4);
    panelLayout->setSpacing(12);

    QGroupBox* faultGroup = new QGroupBox(tr("故障注入"));
    QGridLayout* faultLayout = new QGridLayout(faultGroup);

    m_faultEnable = new QCheckBox(tr("启用故障注入"));
    faultLayout->addWidget(m_faultEnable, 0, 0, 1, 2);

    faultLayout->addWidget(new QLabel(tr("丢帧率:")), 1, 0);
    m_faultDropRate = new QSlider(Qt::Horizontal);
    m_faultDropRate->setRange(0, 100);
    m_faultDropRate->setValue(0);
    m_faultDropRate->setTickPosition(QSlider::TicksBelow);
    m_faultDropRate->setTickInterval(10);
    faultLayout->addWidget(m_faultDropRate, 1, 1);

    faultLayout->addWidget(new QLabel(tr("数据损坏率:")), 2, 0);
    m_faultCorruptRate = new QSlider(Qt::Horizontal);
    m_faultCorruptRate->setRange(0, 100);
    m_faultCorruptRate->setValue(0);
    m_faultCorruptRate->setTickPosition(QSlider::TicksBelow);
    m_faultCorruptRate->setTickInterval(10);
    faultLayout->addWidget(m_faultCorruptRate, 2, 1);

    faultLayout->addWidget(new QLabel(tr("响应延迟:")), 3, 0);
    m_faultDelay = new QSlider(Qt::Horizontal);
    m_faultDelay->setRange(0, 500);
    m_faultDelay->setValue(0);
    m_faultDelay->setTickPosition(QSlider::TicksBelow);
    m_faultDelay->setTickInterval(50);
    faultLayout->addWidget(m_faultDelay, 3, 1);

    m_faultCheckSum = new QCheckBox(tr("校验和错误"));
    faultLayout->addWidget(m_faultCheckSum, 4, 0, 1, 2);

    panelLayout->addWidget(faultGroup);

    QGroupBox* paramGroup = new QGroupBox(tr("仿真参数"));
    QGridLayout* paramLayout = new QGridLayout(paramGroup);

    paramLayout->addWidget(new QLabel(tr("电流摆率:")), 0, 0);
    m_simSlewRate = new QSlider(Qt::Horizontal);
    m_simSlewRate->setRange(1, 100);
    m_simSlewRate->setValue(10);
    m_simSlewRate->setTickPosition(QSlider::TicksBelow);
    m_simSlewRate->setTickInterval(10);
    paramLayout->addWidget(m_simSlewRate, 0, 1);

    paramLayout->addWidget(new QLabel(tr("噪声幅值:")), 1, 0);
    m_simNoise = new QSlider(Qt::Horizontal);
    m_simNoise->setRange(0, 200);
    m_simNoise->setValue(20);
    m_simNoise->setTickPosition(QSlider::TicksBelow);
    m_simNoise->setTickInterval(20);
    paramLayout->addWidget(m_simNoise, 1, 1);

    paramLayout->addWidget(new QLabel(tr("启动延迟:")), 2, 0);
    m_simStartupDelay = new QSlider(Qt::Horizontal);
    m_simStartupDelay->setRange(100, 10000);
    m_simStartupDelay->setValue(2000);
    m_simStartupDelay->setTickPosition(QSlider::TicksBelow);
    m_simStartupDelay->setTickInterval(1000);
    paramLayout->addWidget(m_simStartupDelay, 2, 1);

    paramLayout->addWidget(new QLabel(tr("温度滞后:")), 3, 0);
    m_simTempLag = new QSlider(Qt::Horizontal);
    m_simTempLag->setRange(1, 50);
    m_simTempLag->setValue(5);
    m_simTempLag->setTickPosition(QSlider::TicksBelow);
    m_simTempLag->setTickInterval(5);
    paramLayout->addWidget(m_simTempLag, 3, 1);

    panelLayout->addWidget(paramGroup);
    panelLayout->addStretch();

    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(centralWidget()->layout());
    if (mainLayout) {
        int statusBarIdx = mainLayout->indexOf(m_connectionStatusBar);
        mainLayout->insertWidget(statusBarIdx + 1, m_simPanel);
    }

    m_simPanel->setVisible(false);

    connect(m_faultEnable, &QCheckBox::toggled, this, [this](bool checked) {
        if (m_simulationWorker && m_simulationWorker->isConnected()) {
            FaultInjector* fi = m_simulationWorker->simulatorCore()->faultInjector();
            if (fi) fi->setEnabled(checked);
        }
    });
    connect(m_faultDropRate, &QSlider::valueChanged, this, [this](int v) {
        if (m_simulationWorker && m_simulationWorker->isConnected()) {
            FaultInjector* fi = m_simulationWorker->simulatorCore()->faultInjector();
            if (fi) fi->setDropRate(v / 100.0);
        }
    });
    connect(m_faultCorruptRate, &QSlider::valueChanged, this, [this](int v) {
        if (m_simulationWorker && m_simulationWorker->isConnected()) {
            FaultInjector* fi = m_simulationWorker->simulatorCore()->faultInjector();
            if (fi) fi->setCorruptRate(v / 100.0);
        }
    });
    connect(m_faultDelay, &QSlider::valueChanged, this, [this](int v) {
        if (m_simulationWorker && m_simulationWorker->isConnected()) {
            FaultInjector* fi = m_simulationWorker->simulatorCore()->faultInjector();
            if (fi) fi->setResponseDelay(v * 10);
        }
    });
    connect(m_faultCheckSum, &QCheckBox::toggled, this, [this](bool checked) {
        if (m_simulationWorker && m_simulationWorker->isConnected()) {
            FaultInjector* fi = m_simulationWorker->simulatorCore()->faultInjector();
            if (fi) fi->setWrongChecksum(checked);
        }
    });
    connect(m_simSlewRate, &QSlider::valueChanged, this, [this](int v) {
        if (m_simulationWorker && m_simulationWorker->isConnected()) {
            SimDeviceStateMachine* sm = m_simulationWorker->simulatorCore()->stateMachine();
            if (sm) sm->setCurrentSlewRate(v / 100.0);
        }
    });
    connect(m_simNoise, &QSlider::valueChanged, this, [this](int v) {
        if (m_simulationWorker && m_simulationWorker->isConnected()) {
            SimDeviceStateMachine* sm = m_simulationWorker->simulatorCore()->stateMachine();
            if (sm) sm->setNoiseAmplitude(v / 1000.0);
        }
    });
    connect(m_simStartupDelay, &QSlider::valueChanged, this, [this](int v) {
        if (m_simulationWorker && m_simulationWorker->isConnected()) {
            SimDeviceStateMachine* sm = m_simulationWorker->simulatorCore()->stateMachine();
            if (sm) sm->setStartupDelayMs(v);
        }
    });
    connect(m_simTempLag, &QSlider::valueChanged, this, [this](int v) {
        if (m_simulationWorker && m_simulationWorker->isConnected()) {
            SimDeviceStateMachine* sm = m_simulationWorker->simulatorCore()->stateMachine();
            if (sm) sm->setTempResponseLag(v / 100.0);
        }
    });
}