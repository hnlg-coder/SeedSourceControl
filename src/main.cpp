#ifdef _WIN32
    // Windows平台：避免弹出控制台窗口
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <windows.h>
    
    // 使用Unicode版本的main，但禁用控制台
    #ifndef UNICODE
    #define UNICODE
    #endif
#endif

#include "mainwindow.h"
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDebug>

/**
 * @brief Windows平台专用入口点，避免控制台窗口
 */
#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main(int argc, char* argv[])
#endif
{
#ifdef _WIN32
    // 显式禁用控制台分配
    int argc = 0;
    char* argv[1] = { nullptr };
#endif
    
    // 启用高DPI支持
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    
    // 创建应用程序对象
    QApplication app(argc, argv);
    
    // 设置应用程序信息
    app.setApplicationName("SeedSourceControl");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("SeedSource");
    
    // 加载样式表
    QFile styleFile(":/styles/default.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QTextStream styleStream(&styleFile);
        app.setStyleSheet(styleStream.readAll());
        styleFile.close();
        qDebug() << "样式表加载成功";
    } else {
        qWarning() << "样式表加载失败";
    }
    
    // 创建主窗口
    MainWindow window;
    window.show();
    
    // 进入事件循环
    return app.exec();
}
