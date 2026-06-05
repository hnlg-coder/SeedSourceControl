# Tasks

- [x] Task 1: 移除工具栏
  - [x] 删除 `mainwindow.cpp` 中的 `createToolbar()` 函数体
  - [x] 删除 `mainwindow.cpp` 构造函数中对 `createToolbar()` 的调用
  - [x] 删除 `mainwindow.h` 中的 `createToolbar()` 声明
  - [x] 编译验证

- [x] Task 2: 修复下拉菜单三角箭头样式
  - [x] 修改 `connectionpanel.cpp` 中 `comboStyle` QSS：移除 `drop-down { width: 0px }` 和 `down-arrow { image: none }`，改为定义三角箭头的 `::drop-down` 和 `::down-arrow` 样式
  - [x] 编译验证

- [x] Task 3: 窗口可缩放与布局自适应
  - [x] 在 `mainwindow.cpp` 构造函数中添加 `setMinimumSize(800, 600)`
  - [x] 日志面板改为弹性高度：移除 `setMaximumHeight(150)`，设置 `setMinimumHeight(80)` + `setMaximumHeight(200)`
  - [x] 编译验证

- [x] Task 4: 运行验证与 Git 提交
  - [x] 部署并运行程序验证所有功能
  - [x] Git commit 提交修改

# Task Dependencies
- Task 2 和 Task 3 无依赖，可并行执行
- Task 4 依赖 Task 1、2、3 全部完成