# 种子源模块控制器

基于Qt框架开发的Windows上位机软件，用于控制和管理种子源模块设备。

## 项目概述

本项目按照《种子源模块通信协议V1.3》规范开发，实现了以下功能：

- 设备连接与通信
- 参数配置与设置
- 实时数据监控
- 数据可视化
- 报警管理
- 数据记录与导出
- 多线程架构

## 技术栈

- **编程语言**: C++11
- **开发框架**: Qt 6.5+ (Windows平台)
- **编译器**: MSVC 2019/2022
- **构建工具**: CMake
- **数据库**: SQLite
- **图表库**: QCustomPlot

## 架构设计

### 分层架构

1. **协议层** (Protocol Layer)
   - 协议解析器
   - 寄存器管理器

2. **通信层** (Communication Layer)
   - 命令模式封装
   - 通信工作线程
   - 串口通信

3. **数据模型层** (Data Model Layer)
   - 设备数据模型
   - 观察者模式

4. **可视化层** (Visualization Layer)
   - 实时图表控件
   - 数据表格

5. **UI层** (User Interface Layer)
   - 连接面板
   - 状态面板
   - 控制面板
   - 日志面板
   - 报警面板

6. **数据库层** (Database Layer)
   - SQLite数据库管理
   - 数据存储与查询

7. **报警层** (Alarm Layer)
   - 报警检测与触发
   - 报警历史记录

8. **配置层** (Config Layer)
   - 全局配置管理
   - QSettings持久化

## 项目结构

```
SeedSourceControl/
├── CMakeLists.txt              # CMake构建配置
├── README.md                   # 项目说明文档
├── build_project.bat           # 构建脚本
├── deploy.bat                  # 部署脚本
├── resources.qrc               # Qt资源文件
├── styles/                     # 样式表目录
│   └── default.qss             # 默认样式表
└── src/                        # 源代码目录
    ├── main.cpp                # 程序入口（Windows GUI模式）
    ├── mainwindow.h/cpp        # 主窗口
    ├── protocol/               # 协议层
    │   ├── protocolparser.h/cpp
    │   └── registermanager.h/cpp
    ├── communication/          # 通信层
    │   ├── command.h/cpp
    │   └── communicationworker.h/cpp
    ├── model/                  # 数据模型层
    │   └── devicedatamodel.h/cpp
    ├── visualization/          # 可视化层
    │   └── realtimeplotwidget.h/cpp
    ├── ui/                     # UI组件
    │   ├── connectionpanel.h/cpp
    │   ├── statuspanel.h/cpp
    │   ├── controlpanel.h/cpp
    │   ├── realtimeplotpanel.h/cpp
    │   ├── alarmpanel.h/cpp
    │   ├── datatablepanel.h/cpp
    │   └── logpanel.h/cpp
    ├── database/               # 数据库管理
    │   └── databasemanager.h/cpp
    ├── alarm/                  # 报警管理
    │   └── alarmmanager.h/cpp
    └── config/                 # 配置管理
        └── configmanager.h/cpp
```

## 编译与运行

### 前置条件

1. 安装Qt 6.5或更高版本（MSVC 2019/2022）
2. 安装CMake 3.16或更高版本
3. 安装Visual Studio 2019或2022

### 编译步骤

1. 使用构建脚本（推荐）
   ```bash
   build_project.bat
   ```

2. 手动编译
   ```bash
   mkdir build
   cd build
   cmake .. -DCMAKE_PREFIX_PATH="D:/Qt6.9.1/6.5.3/msvc2019_64"
   cmake --build . --config Debug
   ```

### 运行程序

编译完成后，可执行文件及Qt运行时依赖会自动部署到 `build/deploy/Debug/` 目录：

```
build/deploy/Debug/SeedSourceControl.exe
```

该目录包含所有必需的Qt DLL和插件，可直接双击运行，无需系统安装Qt环境。

### 部署说明

项目集成了 `windeployqt` 自动部署：
- CMake构建完成后自动将可执行文件复制到 `deploy/Debug` 目录
- 自动部署Qt运行时依赖（DLL、平台插件、SQL驱动、图像格式插件等）
- 也可手动运行 `deploy.bat` 重新部署

## 功能模块说明

### 1. 连接面板

- 串口选择与配置
- 波特率、数据位、停止位、校验位设置
- 连接/断开按钮
- 连接状态显示

### 2. 控制面板

- 启动/停止设备
- 设备重置
- 设备校准
- 电流、温度、功率参数设置

### 3. 状态面板

- 设备状态显示
- 实时电流、温度、功率数据
- 报警状态显示

### 4. 实时图表面板

- 电流曲线
- 温度曲线
- 功率曲线
- 支持缩放和平移

### 5. 报警面板

- 实时报警显示
- 报警历史记录
- 报警清除功能

### 6. 日志面板

- 通信日志记录
- 操作日志记录
- 错误日志显示

## 通信协议

### 帧格式

```
[帧头 0xAA][地址码][命令码][数据长度 L][数据*L][校验和][帧尾 0x55]
```

### 寄存器地址

- 0x00 STATUS - 设备状态寄存器
- 0x01 ALERT - 报警状态寄存器
- 0x02 CUR - 电流控制寄存器
- 0x03 TEMP - 温度控制寄存器
- 0x04 POWER - 功率控制寄存器
- 0x05 CONFIG - 配置寄存器
- 0x06 SYSTEM - 系统控制寄存器
- 0x07 DEVINFO - 设备信息寄存器

## 设计模式

本项目使用了多种设计模式：

1. **命令模式** - 封装设备操作命令
2. **观察者模式** - 实现数据变更通知
3. **状态模式** - 管理通信状态机
4. **策略模式** - 实现不同的通信策略
5. **工厂模式** - 创建命令对象
6. **单例模式** - 管理全局配置和资源

## 多线程架构

- **主线程 (UI线程)**: 用户界面更新、用户输入处理、图表数据渲染
- **通信线程**: 串口数据收发、命令队列处理、超时重试机制
- **数据处理线程**: 数据缓存、数据预处理、数据库操作、报警检测

### 线程安全

- CommunicationWorker 中的 `m_pendingCommands`、`m_receiveBuffer` 使用 QMutex 保护
- ConfigManager 中所有配置成员和 QSettings 操作使用 QMutex 保护
- QTimer 在子线程中创建和销毁，避免跨线程访问

## 注意事项

1. 确保串口配置与设备一致
2. 首次使用前请先连接设备
3. 数据会自动保存到SQLite数据库
4. 报警阈值可通过配置文件调整
5. 程序以Windows GUI模式运行，不会弹出控制台窗口

## 版本历史

- **v1.0.0** (2025-12-20)
  - 初始版本发布
  - 实现基础功能

- **v1.0.1** (2026-06-05)
  - 修复线程安全问题：CommunicationWorker 和 ConfigManager 添加 QMutex 保护
  - 修复退出程序时调试断点错误：QTimer 线程归属修正，定时器在子线程中创建和销毁
  - 添加 windeployqt 自动部署，程序可直接双击运行
  - 设置 WIN32_EXECUTABLE，程序以 Windows GUI 模式运行，不弹出控制台窗口
  - 程序入口改为 WinMain，避免控制台窗口

## 许可证

本项目为内部项目，仅供指定人员使用。

## 联系方式

如有问题或建议，请联系开发团队。
