# SeedSourceControl.exe windeployqt部署实现计划

&gt; **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**目标:** 为SeedSourceControl.exe添加windeployqt自动部署，使用户可以直接双击运行该软件而无需依赖系统安装的Qt环境。

**架构方案:** 在CMakeLists.txt中添加POST_BUILD自定义命令，在每次构建后自动执行windeployqt，将所有必需的Qt依赖项（DLL、插件等）复制到deploy目录中。

**技术栈:** CMake、Qt 6.5.3、windeployqt

---

## 文件映射

- **修改:** `f:\工作文档\2026\大功率可调恒流源\QT上位机\CMakeLists.txt` - 添加windeployqt部署配置

---

## 任务1: 在CMakeLists.txt中添加windeployqt配置

**文件:**
- 修改: `f:\工作文档\2026\大功率可调恒流源\QT上位机\CMakeLists.txt:102-104`

- [ ] **步骤1: 编辑CMakeLists.txt，在install()之后添加windeployqt配置**

在CMakeLists.txt的第102行之后添加以下内容：

```cmake
if(WIN32)
    get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT_BIN_DIR}")

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/deploy/$&lt;CONFIG&gt;"
            COMMAND ${CMAKE_COMMAND} -E copy "$&lt;TARGET_FILE:${PROJECT_NAME}&gt;" "${CMAKE_CURRENT_BINARY_DIR}/deploy/$&lt;CONFIG&gt;/"
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                --no-translations
                --no-system-d3d-compiler
                --no-opengl-sw
                --no-quick-import
                "${CMAKE_CURRENT_BINARY_DIR}/deploy/$&lt;CONFIG&gt;/$&lt;TARGET_FILE_NAME:${PROJECT_NAME}&gt;"
            COMMENT "Deploying ${PROJECT_NAME} with windeployqt"
        )
    else()
        message(WARNING "windeployqt not found, automatic deployment disabled")
    endif()
endif()
```

- [ ] **步骤2: 重新配置并构建项目**

运行:
```bash
cd build
cmake ..
cmake --build . --config Debug
```

预期: 构建成功，windeployqt自动执行，在`build/deploy/Debug/`目录下生成完整的可部署程序

- [ ] **步骤3: 验证部署结果**

检查`build/deploy/Debug/`目录：
- 应有SeedSourceControl.exe
- 应有所有必需的Qt DLL（Qt6Core.dll、Qt6Gui.dll等）
- 应有platforms插件目录
- 应有其他必要的Qt插件目录

- [ ] **步骤4: 测试直接运行**

双击运行`build/deploy/Debug/SeedSourceControl.exe`，确保程序可以正常启动且无Qt库缺失错误

- [ ] **步骤5: 使用git-commit技能提交更改**

---

## 自我检查

1. **规范覆盖:** ✅ 添加了windeployqt自动部署配置，满足用户直接运行的需求
2. **占位符扫描:** ✅ 无TBD/TODO，所有内容完整明确
3. **一致性检查:** ✅ 使用与模拟器项目相同的windeployqt配置方式，保持项目间一致性
