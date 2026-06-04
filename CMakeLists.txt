cmake_minimum_required(VERSION 3.16)
project(SeedSourceControl VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_PREFIX_PATH "D:/Qt6.9.1/6.5.3/msvc2019_64" CACHE PATH "Qt installation path")

find_package(Qt6 REQUIRED COMPONENTS Core Gui SerialPort Widgets Sql)

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    
    src/protocol/protocolparser.h
    src/protocol/protocolparser.cpp
    src/protocol/registermanager.h
    src/protocol/registermanager.cpp
    
    src/communication/command.h
    src/communication/command.cpp
    src/communication/communicationworker.h
    src/communication/communicationworker.cpp
    
    src/model/devicedatamodel.h
    src/model/devicedatamodel.cpp
    
    src/visualization/realtimeplotwidget.h
    src/visualization/realtimeplotwidget.cpp
    
    src/ui/connectionpanel.h
    src/ui/connectionpanel.cpp
    src/ui/statuspanel.h
    src/ui/statuspanel.cpp
    src/ui/controlpanel.h
    src/ui/controlpanel.cpp
    src/ui/realtimeplotpanel.h
    src/ui/realtimeplotpanel.cpp
    src/ui/alarmpanel.h
    src/ui/alarmpanel.cpp
    src/ui/datatablepanel.h
    src/ui/datatablepanel.cpp
    src/ui/logpanel.h
    src/ui/logpanel.cpp
    
    src/database/databasemanager.h
    src/database/databasemanager.cpp
    
    src/alarm/alarmmanager.h
    src/alarm/alarmmanager.cpp
    
    src/config/configmanager.h
    src/config/configmanager.cpp
    
    resources.qrc
)

set(HEADERS
    src/protocol/protocolparser.h
    src/protocol/registermanager.h
    
    src/communication/command.h
    src/communication/communicationworker.h
    
    src/model/devicedatamodel.h
    
    src/visualization/realtimeplotwidget.h
    
    src/ui/connectionpanel.h
    src/ui/statuspanel.h
    src/ui/controlpanel.h
    src/ui/realtimeplotpanel.h
    src/ui/alarmpanel.h
    src/ui/datatablepanel.h
    src/ui/logpanel.h
    
    src/database/databasemanager.h
    
    src/alarm/alarmmanager.h
    
    src/config/configmanager.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# 设置为Windows子系统可执行文件，不显示控制台窗口
set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE ON)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::SerialPort
    Qt6::Widgets
    Qt6::Sql
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

if(WIN32)
    get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT_BIN_DIR}")

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/deploy/$<CONFIG>"
            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${PROJECT_NAME}>" "${CMAKE_CURRENT_BINARY_DIR}/deploy/$<CONFIG>/"
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                --no-translations
                --no-system-d3d-compiler
                --no-opengl-sw
                --no-quick-import
                "${CMAKE_CURRENT_BINARY_DIR}/deploy/$<CONFIG>/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
            COMMENT "Deploying ${PROJECT_NAME} with windeployqt"
        )
    else()
        message(WARNING "windeployqt not found, automatic deployment disabled")
    endif()
endif()
