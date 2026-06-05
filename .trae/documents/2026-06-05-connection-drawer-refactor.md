# 串口设置抽屉式重构设计文档

**日期:** 2026-06-05
**版本:** v2.1

## 问题描述

主窗口顶部 ConnectionPanel 包含6个下拉框(串口、波特率、数据位、校验位、停止位、流控制)、标签、2个按钮和状态指示，全部挤在一个 QHBoxLayout 单行中。当窗口缩小至约900px以下时，控件溢出/裁剪，无法正常使用。

## 方案选型

选择方案 C：顶部紧凑状态条 + 可折叠抽屉配置面板。

| 方案 | 描述 | 结果 |
|------|------|------|
| A | 串口设置移入总览页签顶部 | ❌ |
| B | 独立"连接设置"页签 | ❌ |
| C | 紧凑状态条 + 抽屉面板 | ✅ 选中 |

### 选择理由
- 窗口最小化时体验极致紧凑
- 连接状态始终可见
- 配置不占用任何 Tab 页签空间
- 现代交互模式，符合工业软件趋势

## 架构设计

```
┌──────────────────────────────────────────────┐
│  菜单栏 (文件/帮助)                            │
├──────────────────────────────────────────────┤
│  🔌 COM3 | 115200 | ● 已连接    [断开] [⚙]   │  ← ConnectionStatusBar (固定28px)
├──────────────────────────────────────────────┤
│  ┌─ 抽屉(可折叠) ┐                            │
│  │ Port:  [▾]   │  ┌── Tab 功能区域 ──────┐  │
│  │ Baud:  [▾]   │  │  总览 / 电流控制 /   │  │
│  │ Data:  [▾]   │  │  温度控制 / ...      │  │
│  │ Parity:[▾]   │  │                       │  │
│  │ Stop:  [▾]   │  │                       │  │
│  │ Flow:  [▾]   │  │                       │  │
│  │ [连接] [刷新] │  │                       │  │
│  └──────────────┘  └───────────────────────┘  │
├──────────────────────────────────────────────┤
│  状态栏                                        │
└──────────────────────────────────────────────┘
```

## 新增组件: ConnectionStatusBar

### 头文件 (`connectionstatusbar.h`)

```cpp
class ConnectionStatusBar : public QWidget {
    Q_OBJECT
public:
    explicit ConnectionStatusBar(QWidget* parent = nullptr);
    void setConnected(bool connected, const QString& portName = "", qint32 baudRate = 0);
    bool isConnected() const;

signals:
    void configToggled();      // 配置按钮点击
    void disconnectClicked();  // 断开按钮点击

private:
    QLabel* m_iconLabel;
    QLabel* m_statusLabel;
    QPushButton* m_disconnectBtn;
    QPushButton* m_configBtn;
    bool m_connected;
};
```

### 状态显示规则

| 状态 | 显示 | 按钮 |
|------|------|------|
| 未连接 | `🔌 未连接` (灰色) | `[⚙ 配置]` |
| 已连接 | `🔌 COM3 @ 115200 | ● 已连接` (绿色) | `[断开]` `[⚙ 配置]` |

## 抽屉面板实现

- 复用现有 `ConnectionPanel` 组件
- 包裹在 `QWidget` drawer 容器中
- 使用 `QPropertyAnimation` 控制 `maximumWidth` (0 ↔ 220px)
- 动画参数: duration=200ms, easing=QEasingCurve::OutCubic
- 信号 `configToggled` 触发 toggle 动画

## ConnectionPanel 调整

- `setupUI()` 布局从 `QHBoxLayout` 改为 `QVBoxLayout` + `QFormLayout`
- API、信号、槽保持不变
- 移除全局 top-level 的压缩样式

## MainWindow 改动

### 成员变更
- 移除: 直接持有 `m_connectionPanel` 引用
- 新增: `m_connectionStatusBar` (ConnectionStatusBar*)
- 新增: `m_connectionDrawer` (QWidget*, 可折叠容器)
- 新增: `m_drawerAnimation` (QPropertyAnimation*)
- 保留: `m_connectionPanel` (ConnectionPanel*, 放入drawer内)

### setupUI() 布局变更
```cpp
QVBoxLayout* mainLayout;
  → ConnectionStatusBar (28px 固定)
  → QHBoxLayout(bodyLayout)
      → Drawer (220px maxWidth, 0 by default)
          → ConnectionPanel (FormLayout)
      → QTabWidget (stretch=1)
```

### 信号连接新增
- `m_connectionStatusBar::configToggled` → toggleDrawer()
- `onConnectionStateChanged(true)` → drawer auto-collapse, statusBar update
- `onConnectionStateChanged(false)` → drawer auto-expand, statusBar update

## 文件清单

| 操作 | 文件 | 说明 |
|------|------|------|
| 新增 | `src/ui/connectionstatusbar.h` | 状态条头文件 |
| 新增 | `src/ui/connectionstatusbar.cpp` | 状态条实现 |
| 修改 | `src/ui/connectionpanel.cpp` | 布局改为 FormLayout |
| 修改 | `src/mainwindow.h` | 新增成员变量 |
| 修改 | `src/mainwindow.cpp` | 布局重构 + 信号连接 |
| 修改 | `CMakeLists.txt` | 新增源文件 |

## 边界处理

| 场景 | 处理 |
|------|------|
| 连接失败 | 状态条恢复"未连接"，抽屉保持展开 |
| 通信中断 | connectionStateChanged(false) → 变红 + 展开抽屉 |
| 极小窗口 | 抽屉220px不变，Tab弹性压缩 |
| 连接中换配置 | setConnected(true) 已禁用所有下拉框 |
| 动画中重复toggle | 反向播放当前动画 |
