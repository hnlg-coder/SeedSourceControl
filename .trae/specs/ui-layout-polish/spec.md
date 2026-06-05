# UI布局优化 Spec

## Why
当前GUI界面存在三个问题：窗口无法自由缩放、工具栏冗余、串口下拉菜单右侧有横杠影响美观。

## What Changes
- 移除冗余工具栏（"连接/断开/启动/停止"按钮），操作入口保留在页签和串口面板中
- 串口配置面板 QComboBox 下拉箭头样式改为 CSS 三角箭头
- 窗口改为可自由缩放，各控件采用弹性布局策略自适应
- 遵循 Git 版本控制流程，不涉及版本回退

## Impact
- Affected specs: UI布局
- Affected code: `src/mainwindow.cpp`, `src/mainwindow.h`, `src/ui/connectionpanel.cpp`

## ADDED Requirements

### Requirement: 窗口可缩放与自适应布局
系统 SHALL 支持窗口自由缩放（最小 800×600），各控件在窗口尺寸变化时自动适配布局。

#### Scenario: 窗口从默认尺寸放大
- **WHEN** 用户拖拽窗口边缘放大窗口
- **THEN** QTabWidget 和图表控件按比例扩展填充空间，日志面板同步增高

#### Scenario: 窗口缩小到最小尺寸
- **WHEN** 用户将窗口缩小到 800×600 以内
- **THEN** 窗口停止缩小，保持最小尺寸；串口面板控件不溢出

#### Scenario: 串口面板在小窗口下适配
- **WHEN** 窗口宽度缩小时
- **THEN** 串口配置的 QComboBox 按比例收缩，不出现水平滚动条

### Requirement: 下拉菜单三角箭头样式
系统 SHALL 在串口配置面板的 QComboBox 中使用 CSS border trick 绘制的三角箭头替代原有的空白下拉区域。

#### Scenario: 查看下拉菜单外观
- **WHEN** 用户查看串口配置面板的下拉菜单
- **THEN** 右侧显示一个向下的灰色三角箭头，无横杠

### Requirement: 日志面板弹性高度
系统 SHALL 将日志面板从固定最大高度 150px 改为弹性高度范围 80-200px，随窗口缩放。

#### Scenario: 窗口放大
- **WHEN** 窗口高度增大
- **THEN** 日志面板高度随之增加，最大不超过 200px

#### Scenario: 窗口缩小
- **WHEN** 窗口高度减小
- **THEN** 日志面板高度随之减小，最小不低于 80px

## REMOVED Requirements

### Requirement: 工具栏
**Reason**: "连接/断开"与串口面板按钮重复，"启动/停止"与总览页签重复
**Migration**: 无需迁移，功能入口已存在于页签和串口面板中