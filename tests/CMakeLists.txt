# 测试子项目CMake配置

# 查找Qt测试模块
find_package(Qt6 REQUIRED COMPONENTS Core Test SerialPort Sql)

# 测试可执行文件
set(TEST_SOURCES
    test_communication_worker.cpp
    ${CMAKE_SOURCE_DIR}/src/communication/communicationworker.cpp
    ${CMAKE_SOURCE_DIR}/src/communication/command.cpp
    ${CMAKE_SOURCE_DIR}/src/protocol/protocolparser.cpp
    ${CMAKE_SOURCE_DIR}/src/model/devicedatamodel.cpp
    ${CMAKE_SOURCE_DIR}/src/database/databasemanager.cpp
    ${CMAKE_SOURCE_DIR}/src/alarm/alarmmanager.cpp
    ${CMAKE_SOURCE_DIR}/src/config/configmanager.cpp
)

# 创建测试可执行文件
add_executable(test_communication_worker ${TEST_SOURCES})

# 设置包含目录
target_include_directories(test_communication_worker PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/communication
    ${CMAKE_SOURCE_DIR}/src/protocol
    ${CMAKE_SOURCE_DIR}/src/model
    ${CMAKE_SOURCE_DIR}/src/database
    ${CMAKE_SOURCE_DIR}/src/alarm
    ${CMAKE_SOURCE_DIR}/src/config
)

# 链接Qt库
target_link_libraries(test_communication_worker PRIVATE
    Qt6::Core
    Qt6::Test
    Qt6::SerialPort
    Qt6::Sql
)

# 启用C++17
target_compile_features(test_communication_worker PRIVATE cxx_std_17)

# 添加测试到CTest
add_test(NAME CommunicationWorkerTests COMMAND test_communication_worker)
