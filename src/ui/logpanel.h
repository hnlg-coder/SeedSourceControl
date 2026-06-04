#ifndef LOGPANEL_H
#define LOGPANEL_H

#include <QWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QFile>

class LogPanel : public QWidget {
    Q_OBJECT
public:
    enum LogLevel {
        Info,
        Warning,
        Error,
        Debug
    };
    
    explicit LogPanel(QWidget* parent = nullptr);
    ~LogPanel();
    
public slots:
    void logMessage(const QString& message, LogLevel level = Info);
    void clearLog();
    void saveLog();
    
private slots:
    void onClearButtonClicked();
    void onSaveButtonClicked();
    
private:
    void setupUI();
    QString logLevelToString(LogLevel level);
    QString getCurrentTimestamp();
    
    QTextEdit* m_logTextEdit;
    QPushButton* m_clearButton;
    QPushButton* m_saveButton;
    
    QFile m_logFile;
};

#endif // LOGPANEL_H
