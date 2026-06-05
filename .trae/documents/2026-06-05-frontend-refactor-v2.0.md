# 前端界面重构方案 v2.0

## 摘要

基于《种子源模块通信协议V1.3》PDF，对比当前上位机前端界面，发现数据展示细粒度严重不足。当前只显示5个粗糙字段（current, temperature, power, status, alarm），而协议定义了8个寄存器、数十个位域字段。需要在 `src/ui/` 中已有6个精细化Widget的基础上，修复它们对 `DeviceDataModel` 的依赖，集成到 `MainWindow` 的 QTabWidget 中，并添加实时可视化曲线和统计功能。

## 当前状态分析

### 协议 V1.3 定义的全部寄存器

| 寄存器 | 地址 | 大小 | 描述 |
|--------|------|------|------|
| STATUS | 0x00 | 4B | 设备状态位域（空闲/运行、电流状态、温度状态） |
| ALERT | 0x01 | 4B | 报警状态位域（CC_CTRL, CC_PD, CC_ADC, CC_DAC, TC_CTRL, TC_ADC, TC_DAC, SYS_PWR） |
| CUR | 0x02 | 4B | 电流控制（目标值 + 实际值） |
| TEMP | 0x03 | 4B | 温度控制（目标值 + 实际值） |
| POWER | 0x04 | 4B | 功率（激光功率 + 系统功率 + CC功率） |
| CONFIG | 0x05 | 4B | 配置位域（TC_EN, CC_EN, AE_EN, POWER_SV, CUR_SV, TEMP_SV, CUR_TH, CUR_SLOPE, BAUD_RATE） |
| SYSTEM | 0x06 | 4B | 系统控制（休眠/复位/IAP升级） |
| DEVINFO | 0x07 | 4B | 设备信息（NAME, VER_S, VER_H, SERIAL, CUR_MAX, TEMP_MIN, TEMP_MAX） |

### 当前 MainWindow 展示的数据

- `StatusPanel` → 只显示 status/alarm 原始数值，无位域解析
- `ControlPanel` → 只显示 current/temperature/power 目标值 SpinBox
- `RealtimePlotPanel` → 简单线条图，无多线程渲染，无统计
- `AlarmPanel` → 简单报警表，无 ALERT 位域指示灯

### 已存在但未集成的组件

| 组件 | 文件 | 状态 | 缺失的依赖 |
|------|------|------|------------|
| DashboardWidget | dashboardwidget.h/.cpp | 已实现 | RealTimeData 缺少 status.idle, curSet, curVal, pwrLas, pwrSys 等字段 |
| CurrentControlWidget | currentcontrolwidget.h/.cpp | 已实现 | RealTimeData 缺少 curVal, curSet, status.curStatusText() |
| TemperatureControlWidget | temperaturecontrolwidget.h/.cpp | 已实现 | RealTimeData 缺少 tempVal, tempSet, status.tempStatusText() |
| MonitorWidget | monitorwidget.h/.cpp | 已实现 | RealTimeData 缺少 curPd, curTec, pwrLas, pwrCc, pwrSys |
| ConfigWidget | configwidget.h/.cpp | 已实现 | RealTimeData 缺少 config 和 devInfo 位域结构 |
| AlertWidget | alertwidget.h/.cpp | 已实现 | RealTimeData 缺少 AlertBits 结构体 |
| EnhancedChartWidget | enhancedchartwidget.h/.cpp | 已实现 | 独立渲染线程 + QImage 离屏渲染 + StatisticsCalculator |
| StatisticsCalculator | statisticscalculator.h/.cpp | 已实现 | 滑动窗口统计（均值、方差、标准差、最大、最小） |

### 缺失的 CMakeLists.txt 源文件

以下文件已在磁盘上存在但未添加到 CMakeLists.txt：
- `src/ui/dashboardwidget.h/.cpp`
- `src/ui/configwidget.h/.cpp`
- `src/ui/monitorwidget.h/.cpp`
- `src/ui/currentcontrolwidget.h/.cpp`
- `src/ui/temperaturecontrolwidget.h/.cpp`
- `src/ui/alertwidget.h/.cpp`
- `src/visualization/enhancedchartwidget.h/.cpp`
- `src/visualization/statisticscalculator.h/.cpp`

## 设计方案

### 架构概览

```
MainWindow
├── 菜单栏 + 工具栏
├── ConnectionPanel（紧凑横向，串口配置，顶部固定）
├── QTabWidget（7个功能页签）
│   ├── 页签1: "总览" → DashboardWidget
│   ├── 页签2: "电流控制" → CurrentControlWidget
│   ├── 页签3: "温度控制" → TemperatureControlWidget
│   ├── 页签4: "功率监测" → MonitorWidget
│   ├── 页签5: "设备配置" → ConfigWidget
│   ├── 页签6: "报警管理" → AlertWidget
│   └── 页签7: "数据统计" → 统计面板 + DataTablePanel
├── LogPanel（底部日志）
└── 状态栏
```

### 数据流

```
设备 → CommunicationWorker → 协议解析 → DeviceDataModel → 信号通知 → 7个Widget更新
                                                                    ├── DashboardWidget::updateData()
                                                                    ├── CurrentControlWidget::updateData()
                                                                    ├── TemperatureControlWidget::updateData()
                                                                    ├── MonitorWidget::updateData()
                                                                    ├── ConfigWidget::updateData()
                                                                    ├── AlertWidget::updateData()
                                                                    └── 统计面板 + DataTablePanel
```

### 多线程架构

```
UI线程                    数据线程                    渲染线程
MainWindow            CommunicationWorker         EnhancedChartWidget::RenderThread
  │                        │                           │
  ├─pollDevice()───→  sendCommand() ──→ 设备          │
  │                    response ←──                    │
  │                    parseData()                     │
  │                    emit dataUpdated()              │
  │  ←──────────────────┤                             │
  │  updateData()        │                             │
  │  dataSnapshot() ──────────────────────────────→ renderToImage()
  │  emit frameReady()   │                             │
  │  ←──────────────────────────────────────────────  │
  │  paintEvent()        │                             │
```

线程安全策略（遵循项目已有规范）：
- 数据快照：mutex保护下拷贝数据到局部变量，再释放锁进行渲染
- 原子变量：`std::atomic<int>` 用于 renderWidth/renderHeight
- Scoped lock：`QMutexLocker` 自动管理锁生命周期
- 跨线程 QObject 使用 `nullptr` parent，手动管理生命周期

## 实施步骤

### 步骤 1: 扩展 DeviceDataModel（核心数据模型）

**文件**: `src/model/devicedatamodel.h`, `src/model/devicedatamodel.cpp`

**改动**:
1. 添加位域结构体（在 `RealTimeData` 之前定义）：
   - `StatusBits` — 设备状态位域（idle, curStatus[2bit], tempStatus[2bit]，含 curStatusText()/tempStatusText() 方法）
   - `AlertBits` — 报警位域（ccCtrl, ccPd, ccAdc, ccDac, tcCtrl, tcAdc, tcDac, sysPwr，含 hasAlert()/alertNames() 方法）
   - `ConfigBits` — 配置位域（tcEn, ccEn, aeEn, powerSv, curSv, tempSv, curTh, curSlope, baudRate）
   - `DevInfo` — 设备信息（name, verS, verH, serial, curMax, tempMin, tempMax）

2. 扩展 `RealTimeData` 结构体，新增字段：
   - `double curSet` — 目标电流
   - `double curVal` — 实际电流
   - `double curPd` — PD电流
   - `double curTec` — TEC电流
   - `double tempSet` — 目标温度
   - `double tempVal` — 实际温度
   - `double pwrLas` — 激光功率
   - `double pwrCc` — CC功率
   - `double pwrSys` — 系统功率
   - `StatusBits status` — 替换原来的 `quint32 status`
   - `AlertBits alert` — 替换原来的 `quint32 alarm`
   - `ConfigBits config` — 配置位域
   - `DevInfo devInfo` — 设备信息
   - 保留 `double current`, `double temperature`, `double power` 用于向后兼容（DataTablePanel 仍使用它们）

3. 更新 `updateData()` 签名，接受扩展参数或从 QVariantMap 解析

4. 更新 `checkAlarms()` 使用新的 `AlertBits` 结构

### 步骤 2: 更新 CMakeLists.txt

**文件**: `CMakeLists.txt`

**改动**:
- 在 SOURCES 中添加 6 个新 Widget 的 .cpp 文件
- 在 SOURCES 中添加 `enhancedchartwidget.cpp` 和 `statisticscalculator.cpp`
- 在 HEADERS 中添加对应的 .h 文件

### 步骤 3: 重写 MainWindow

**文件**: `src/mainwindow.h`, `src/mainwindow.cpp`

**改动**:
1. 添加 `#include <QTabWidget>` 和新 Widget 的 include
2. 替换成员变量：
   - 移除：`m_statusPanel`, `m_controlPanel`, `m_plotPanel`, `m_alarmPanel`
   - 新增：`m_tabWidget`, `m_dashboardWidget`, `m_currentControlWidget`, `m_temperatureControlWidget`, `m_monitorWidget`, `m_configWidget`, `m_alertWidget`, `m_dataTablePanel`
3. 重写 `setupUI()`：
   - 顶部：`ConnectionPanel`（保持紧凑横向布局）
   - 中间：`QTabWidget` 包含 7 个页签
   - 底部：`LogPanel`
4. 重写 `connectSignals()`：
   - 连接 `DashboardWidget::startClicked/stopClicked` 到 `onStartClicked/onStopClicked`
   - 连接 `CurrentControlWidget::currentSetChanged` → 发送写寄存器命令
   - 连接 `TemperatureControlWidget::temperatureSetChanged` → 发送写寄存器命令
   - 连接 `ConfigWidget::configChanged` → 发送写配置命令
5. 重写 `onDataUpdated()`：
   - 分别调用 `m_dashboardWidget->updateData(data)`
   - `m_currentControlWidget->updateData(data)`
   - `m_temperatureControlWidget->updateData(data)`
   - `m_monitorWidget->updateData(data)`
   - `m_configWidget->updateData(data)`
   - `m_alertWidget->updateData(data)`
   - `m_dataTablePanel->addDataRow(data)`
6. 重写 `pollDevice()`：发送多个寄存器读取命令（STATUS, ALERT, CUR, TEMP, POWER, CONFIG, DEVINFO）
7. 更新析构函数：确保新 Widget 和 RenderThread 正确清理

### 步骤 4: 添加数据统计页签

**新建文件**: 在 MainWindow 中内联实现或新建 `src/ui/statisticspanel.h/.cpp`

**功能**:
- 显示各参数（电流、温度、功率）的滑动窗口统计值
- 均值、方差、标准差、最大值、最小值
- 数据来源：`EnhancedChartWidget::statistics()` 返回的 `StatisticsCalculator`
- 与 `DataTablePanel` 组合在同一页签中

### 步骤 5: 修复协议解析器数据解析

**文件**: `src/communication/command.cpp`

**改动**:
- 更新 `ReadStatusCommand` 的响应解析，解析所有寄存器数据
- 将解析后的数据映射到 `DeviceDataModel::RealTimeData` 的扩展字段

### 步骤 6: 编译验证

1. 运行 `cmake --build . --config Debug` 确保编译通过
2. 运行 windeployqt 部署
3. 启动程序验证 UI 布局和功能

### 步骤 7: Git 提交

```bash
git add -A
git commit -m "feat: 重构前端界面 v2.0 - 基于协议V1.3精细数据展示与多线程实时可视化"
```

## 假设与决策

1. **集成已有组件**：已存在的 6 个精细化 Widget 功能完整，只需修复 `DeviceDataModel` 依赖即可
2. **QTabWidget 布局**：用户选择顶部页签式布局，7个页签名称简短不会溢出
3. **统计功能**：图表上方显示简要统计（EnhancedChartWidget 已支持），额外增加独立统计页签
4. **向后兼容**：保留 `RealTimeData` 中的 `current/temperature/power` 字段，旧代码仍可用
5. **线程安全**：严格遵循项目 memory 中记录的规范（数据快照、原子变量、scoped lock、nullptr parent）
6. **通信协议**：协议帧格式 `[0xAA][address][command][length][data][checksum][0x55]` 不变

## 验证步骤

1. 编译通过，无警告
2. 程序启动显示 7 个页签，UI 布局正常
3. 串口连接/断开功能正常
4. 数据轮询能正确更新所有页签
5. 图表实时渲染流畅，多线程无死锁
6. 统计值正确计算
7. 退出程序无残留进程、无 debug 弹窗
8. 窗口可正常缩放