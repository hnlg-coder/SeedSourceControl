#include "mainwindow.h"
#include "ui/connectionpanel.h"
#include "ui/statuspanel.h"
#include "ui/controlpanel.h"
#include "ui/realtimeplotpanel.h"
#include "ui/alarmpanel.h"
#include "ui/logpanel.h"
#include "protocol/protocolparser.h"
#include "communication/communicationworker.h"
#include "model/devicedatamodel.h"
#include "communication/command.h"
#include <QSplitter>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QDebug>
#include "config/configmanager.h"
#include "database/databasemanager.h"
#include "alarm/alarmmanager.h"

/**
 * @brief 主窗口构造函数
 * @param parent 父窗口指针
 */
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_connected(false)
    , m_running(false)
{
    // 初始化用户界面
    setupUI();
    
    // 创建菜单栏
    createMenus();
    
    // 创建工具栏
    createToolbar();
    
    // 初始化组件
    initializeComponents();
    
    // 连接信号槽
    connectSignals();
    
    // 设置窗口标题和大小
    setWindowTitle(tr("种子源模块控制器 - v1.0"));
    resize(1200, 800);
}

/**
 * @brief 主窗口析构函数
 */
MainWindow::~MainWindow()
{
    // 先停止定时器
    if (m_pollTimer) {
        m_pollTimer->stop();
    }
    
    // 停止运行
    m_running = false;
    m_connected = false;
    
    // 停止通信工作线程（由CommunicationWorker自己处理线程退出和串口关闭）
    if (m_communicationWorker) {
        m_communicationWorker->stopCommunication();
    }
    
    // 关闭数据库
    DatabaseManager::instance()->close();
}

/**
 * @brief 设置用户界面布局
 */
void MainWindow::setupUI()
{
    // 创建主部件
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // 创建水平分割器
    QSplitter* mainSplitter = new QSplitter(Qt::Horizontal, centralWidget);
    
    // 创建垂直分割器（左侧：连接和控制，右侧：绘图和日志）
    QSplitter* leftSplitter = new QSplitter(Qt::Vertical, mainSplitter);
    QSplitter* rightSplitter = new QSplitter(Qt::Vertical, mainSplitter);
    
    // 创建各个面板
    m_connectionPanel = new ConnectionPanel(leftSplitter);
    m_controlPanel = new ControlPanel(leftSplitter);
    m_statusPanel = new StatusPanel(leftSplitter);
    m_plotPanel = new RealtimePlotPanel(rightSplitter);
    m_alarmPanel = new AlarmPanel(rightSplitter);
    m_logPanel = new LogPanel(rightSplitter);
    
    // 添加到分割器
    leftSplitter->addWidget(m_connectionPanel);
    leftSplitter->addWidget(m_controlPanel);
    leftSplitter->addWidget(m_statusPanel);
    
    rightSplitter->addWidget(m_plotPanel);
    rightSplitter->addWidget(m_alarmPanel);
    rightSplitter->addWidget(m_logPanel);
    
    // 设置分割器比例
    leftSplitter->setSizes({300, 300, 200});
    rightSplitter->setSizes({400, 200, 200});
    mainSplitter->setSizes({300, 700});
    
    // 设置主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->addWidget(mainSplitter);
    
    // 创建状态栏
    statusBar()->showMessage(tr("就绪"));
}

/**
 * @brief 创建菜单栏
 */
void MainWindow::createMenus()
{
    // 文件菜单
    QMenu* fileMenu = menuBar()->addMenu(tr("文件(&F)"));
    
    QAction* exitAction = new QAction(tr("退出(&X)"), this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
    fileMenu->addAction(exitAction);
    
    // 视图菜单
    QMenu* viewMenu = menuBar()->addMenu(tr("视图(&V)"));
    
    // 工具菜单
    QMenu* toolMenu = menuBar()->addMenu(tr("工具(&T)"));
    
    // 帮助菜单
    QMenu* helpMenu = menuBar()->addMenu(tr("帮助(&H)"));
    
    QAction* aboutAction = new QAction(tr("关于(&A)"), this);
    connect(aboutAction, &QAction::triggered, [this]() {
        QMessageBox::about(this, tr("关于"), tr("种子源模块控制器 v1.0\n基于Qt框架开发"));
    });
    helpMenu->addAction(aboutAction);
}

/**
 * @brief 创建工具栏
 */
void MainWindow::createToolbar()
{
    QToolBar* toolBar = addToolBar(tr("主工具栏"));
    
    QAction* connectAction = new QAction(tr("连接"), this);
    QAction* disconnectAction = new QAction(tr("断开"), this);
    QAction* startAction = new QAction(tr("启动"), this);
    QAction* stopAction = new QAction(tr("停止"), this);
    
    toolBar->addAction(connectAction);
    toolBar->addAction(disconnectAction);
    toolBar->addSeparator();
    toolBar->addAction(startAction);
    toolBar->addAction(stopAction);
    
    // 连接信号
    connect(connectAction, &QAction::triggered, this, &MainWindow::onConnectClicked);
    connect(disconnectAction, &QAction::triggered, this, &MainWindow::onDisconnectClicked);
    connect(startAction, &QAction::triggered, this, &MainWindow::onStartClicked);
    connect(stopAction, &QAction::triggered, this, &MainWindow::onStopClicked);
}

/**
 * @brief 初始化各个组件
 */
void MainWindow::initializeComponents()
{
    // 初始化配置管理器
    ConfigManager::instance();
    
    // 初始化数据库
    DatabaseManager::instance()->initialize(ConfigManager::instance()->databasePath());
    
    // 初始化报警管理器
    AlarmManager::instance();
    
    // 创建协议解析器
    m_protocolParser = new SeedSourceProtocolParser(this);
    
    // 创建数据模型
    m_dataModel = new DeviceDataModel(this);
    
    // 创建通信工作线程（QSerialPort在子线程中创建和管理）
    m_communicationWorker = new CommunicationWorker(this);
    
    // 创建轮询定时器
    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(100); // 默认100ms
    
    // 刷新串口列表
    m_connectionPanel->refreshPorts();
}

/**
 * @brief 连接信号槽
 */
void MainWindow::connectSignals()
{
    // 连接面板信号
    connect(m_connectionPanel, &ConnectionPanel::connectClicked, this, &MainWindow::onConnectClicked);
    connect(m_connectionPanel, &ConnectionPanel::disconnectClicked, this, &MainWindow::onDisconnectClicked);
    
    // 控制面板信号
    connect(m_controlPanel, &ControlPanel::startClicked, this, &MainWindow::onStartClicked);
    connect(m_controlPanel, &ControlPanel::stopClicked, this, &MainWindow::onStopClicked);
    connect(m_controlPanel, &ControlPanel::resetClicked, this, &MainWindow::onResetClicked);
    connect(m_controlPanel, &ControlPanel::calibrateClicked, this, &MainWindow::onCalibrateClicked);
    connect(m_controlPanel, &ControlPanel::currentChanged, this, &MainWindow::onCurrentChanged);
    connect(m_controlPanel, &ControlPanel::temperatureChanged, this, &MainWindow::onTemperatureChanged);
    connect(m_controlPanel, &ControlPanel::powerChanged, this, &MainWindow::onPowerChanged);
    
    // 通信工作线程信号
    connect(m_communicationWorker, &CommunicationWorker::connectionStateChanged, this, &MainWindow::onConnectionStateChanged);
    connect(m_communicationWorker, &CommunicationWorker::logMessage, this, &MainWindow::onLogMessage);
    
    // 命令完成信号 -> 更新数据模型
    connect(m_communicationWorker, &CommunicationWorker::commandCompleted, this, [this](quint32 cmdId, bool success, QVariant result) {
        if (success && result.isValid()) {
            // 如果结果是QVariantMap（ReadStatusCommand的返回），更新数据模型
            if (result.userType() == QMetaType::type("QVariantMap")) {
                QVariantMap statusMap = result.toMap();
                double current = statusMap.value("current", 0).toDouble();
                double temperature = statusMap.value("temperature", 0).toDouble();
                double power = statusMap.value("power", 0).toDouble();
                m_dataModel->updateData(current, temperature, power, 0, 0);
            }
        }
    });
    
    // 数据模型信号
    connect(m_dataModel, &DeviceDataModel::dataUpdated, this, [this](const DeviceDataModel::RealTimeData&) {
        onDataUpdated();
    });
    connect(m_dataModel, &DeviceDataModel::alarmTriggered, this, &MainWindow::onAlarmTriggered);
    
    // 定时器信号
    connect(m_pollTimer, &QTimer::timeout, this, &MainWindow::pollDevice);
}

/**
 * @brief 连接按钮点击事件处理
 */
void MainWindow::onConnectClicked()
{
    // 获取串口配置
    QString portName = m_connectionPanel->selectedPort();
    qint32 baudRate = m_connectionPanel->selectedBaudRate();
    QSerialPort::DataBits dataBits = m_connectionPanel->selectedDataBits();
    QSerialPort::Parity parity = m_connectionPanel->selectedParity();
    QSerialPort::StopBits stopBits = m_connectionPanel->selectedStopBits();
    QSerialPort::FlowControl flowControl = m_connectionPanel->selectedFlowControl();
    
    // 设置串口配置参数（线程安全）
    m_communicationWorker->setSerialConfig(portName, baudRate, dataBits, parity, stopBits, flowControl);
    
    // 启动通信工作线程
    m_communicationWorker->startCommunication();
    
    // 连接设备（通过信号通知子线程）
    m_communicationWorker->connectDevice();
    
    m_logPanel->logMessage(tr("尝试连接设备: %1, 波特率: %2").arg(portName).arg(baudRate), LogPanel::Info);
}

/**
 * @brief 断开按钮点击事件处理
 */
void MainWindow::onDisconnectClicked()
{
    // 停止运行
    if (m_running) {
        onStopClicked();
    }
    
    // 断开设备
    m_communicationWorker->disconnectDevice();
    
    m_logPanel->logMessage(tr("设备断开连接"), LogPanel::Info);
}

/**
 * @brief 启动按钮点击事件处理
 */
void MainWindow::onStartClicked()
{
    if (!m_connected) {
        QMessageBox::warning(this, tr("警告"), tr("请先连接设备！"));
        return;
    }
    
    m_running = true;
    m_pollTimer->start();
    
    // 发送启动命令
    auto command = CommandFactory::createControlDeviceCommand(ControlDeviceCommand::Start, m_protocolParser);
    m_communicationWorker->sendCommand(command);
    
    m_logPanel->logMessage(tr("设备启动"), LogPanel::Info);
    statusBar()->showMessage(tr("运行中..."));
}

/**
 * @brief 停止按钮点击事件处理
 */
void MainWindow::onStopClicked()
{
    m_running = false;
    m_pollTimer->stop();
    
    // 发送停止命令
    auto command = CommandFactory::createControlDeviceCommand(ControlDeviceCommand::Stop, m_protocolParser);
    m_communicationWorker->sendCommand(command);
    
    m_logPanel->logMessage(tr("设备停止"), LogPanel::Info);
    statusBar()->showMessage(tr("已停止"));
}

/**
 * @brief 重置按钮点击事件处理
 */
void MainWindow::onResetClicked()
{
    // 发送重置命令
    auto command = CommandFactory::createControlDeviceCommand(ControlDeviceCommand::Reset, m_protocolParser);
    m_communicationWorker->sendCommand(command);
    
    m_logPanel->logMessage(tr("设备重置"), LogPanel::Info);
}

/**
 * @brief 校准按钮点击事件处理
 */
void MainWindow::onCalibrateClicked()
{
    // 发送校准命令
    auto command = CommandFactory::createControlDeviceCommand(ControlDeviceCommand::Calibrate, m_protocolParser);
    m_communicationWorker->sendCommand(command);
    
    m_logPanel->logMessage(tr("设备校准"), LogPanel::Info);
}

/**
 * @brief 电流参数改变事件处理
 */
void MainWindow::onCurrentChanged(double value)
{
    if (m_connected && m_running) {
        // 发送写寄存器命令
        auto command = CommandFactory::createWriteRegisterCommand(0x02, 0x00, static_cast<quint32>(value), m_protocolParser);
        m_communicationWorker->sendCommand(command);
        
        m_logPanel->logMessage(tr("设置目标电流: %1 mA").arg(value), LogPanel::Info);
    }
}

/**
 * @brief 温度参数改变事件处理
 */
void MainWindow::onTemperatureChanged(double value)
{
    if (m_connected && m_running) {
        // 发送写寄存器命令
        auto command = CommandFactory::createWriteRegisterCommand(0x03, 0x00, static_cast<quint32>(value), m_protocolParser);
        m_communicationWorker->sendCommand(command);
        
        m_logPanel->logMessage(tr("设置目标温度: %1 °C").arg(value), LogPanel::Info);
    }
}

/**
 * @brief 功率参数改变事件处理
 */
void MainWindow::onPowerChanged(double value)
{
    if (m_connected && m_running) {
        // 发送写寄存器命令
        auto command = CommandFactory::createWriteRegisterCommand(0x04, 0x00, static_cast<quint32>(value), m_protocolParser);
        m_communicationWorker->sendCommand(command);
        
        m_logPanel->logMessage(tr("设置目标功率: %1 mW").arg(value), LogPanel::Info);
    }
}

/**
 * @brief 连接状态改变事件处理
 */
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

/**
 * @brief 日志消息事件处理
 */
void MainWindow::onLogMessage(const QString& message)
{
    m_logPanel->logMessage(message, LogPanel::Info);
}

/**
 * @brief 数据更新事件处理
 */
void MainWindow::onDataUpdated()
{
    if (!m_dataModel) return;
    
    DeviceDataModel::RealTimeData data = m_dataModel->currentData();
    
    // 更新状态面板
    m_statusPanel->updateStatus(data.status);
    m_statusPanel->updateCurrent(data.current);
    m_statusPanel->updateTemperature(data.temperature);
    m_statusPanel->updatePower(data.power);
    m_statusPanel->updateAlarm(data.alarm);
    
    // 更新绘图面板
    m_plotPanel->updateCurrent(data.current);
    m_plotPanel->updateTemperature(data.temperature);
    m_plotPanel->updatePower(data.power);
    
    // 保存到数据库
    DatabaseManager::instance()->saveData(data);
}

/**
 * @brief 报警触发事件处理
 */
void MainWindow::onAlarmTriggered(quint32 alarmCode, const QString& message)
{
    // 添加到报警面板
    m_alarmPanel->addAlarm(alarmCode, message);
    
    // 保存到数据库
    DatabaseManager::instance()->saveAlarm(alarmCode, message);
    
    // 记录日志
    m_logPanel->logMessage(tr("报警: %1").arg(message), LogPanel::Error);
}

/**
 * @brief 轮询设备状态
 */
void MainWindow::pollDevice()
{
    if (!m_connected) {
        return;
    }
    
    // 发送读状态命令
    auto command = CommandFactory::createReadStatusCommand(m_protocolParser);
    m_communicationWorker->sendCommand(command);
}
