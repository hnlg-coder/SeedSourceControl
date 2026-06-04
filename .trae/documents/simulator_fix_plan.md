# SeedSourceSimulator 问题修复计划

## 问题描述
运行模拟器后，命令行窗口持续不断打印写寄存器操作，导致GUI无法操作。

## 问题根因
1. DeviceStateMachine 的定时器每100ms触发一次模拟更新
2. 每次更新都会调用 writeRegister() 更新所有寄存器
3. writeRegister() 中使用 qDebug() 输出日志
4. 高频日志输出阻塞了GUI线程

## 修复方案

### 任务1：修复寄存器管理器的日志输出问题
- 修改 registermanager.cpp
- 将 qDebug() 改为 emit 信号或移除
- 只在值发生变化时才记录日志

### 任务2：优化设备状态机的模拟更新
- 修改 devicestatemachine.cpp
- 降低日志输出频率
- 或者添加条件判断，只在状态变化时更新寄存器

### 任务3：修复主窗口的线程问题
- 检查 mainwindow.cpp 中的线程启动时机
- 确保 SimulatorWorker 在正确的时机启动

### 任务4：添加完整的中文注释
- 为所有代码文件添加清晰准确的中文注释

## 修复步骤

### 步骤1：修改 RegisterManager
- 移除或修改 `writeRegister()` 中的 qDebug() 调用
- 添加值变化检测，只在值实际变化时才输出日志

### 步骤2：修改 DeviceStateMachine
- 将 qDebug() 改为 emit logMessage 信号
- 或者使用日志级别控制

### 步骤3：修复线程启动逻辑
- 确保 SimulatorWorker 在连接成功后才开始监听

### 步骤4：添加中文注释
- 为所有头文件和源文件添加详细的中文注释

## 文件修改清单

| 文件 | 修改内容 |
|------|----------|
| src/protocol/registermanager.cpp | 移除或修改 qDebug 输出 |
| src/protocol/devicestatemachine.cpp | 将 qDebug 改为信号或移除 |
| src/mainwindow.cpp | 修复线程启动时机 |
| src/protocol/registermanager.h | 添加中文注释 |
| src/protocol/devicestatemachine.h | 添加中文注释 |
| src/communication/simulatorworker.h/cpp | 添加中文注释 |
| src/protocol/simulatorprotocolhandler.h/cpp | 添加中文注释 |
| src/ui/*.h/cpp | 添加中文注释 |

## 预期结果

修复后：
- 命令行不会持续输出日志
- GUI界面可以正常操作
- 寄存器值更新逻辑正常工作
- 代码注释清晰易懂
