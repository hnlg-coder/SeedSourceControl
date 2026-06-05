#include "logpanel.h"
#include <QHBoxLayout>
#include <QDateTime>
#include <QFileDialog>
#include <QTextStream>
#include <QScrollBar>
#include <QMessageBox>

LogPanel::LogPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

LogPanel::~LogPanel()
{
}

void LogPanel::logMessage(const QString& message, LogLevel level)
{
    QString timestamp = getCurrentTimestamp();
    QString levelStr = logLevelToString(level);
    QString color;
    
    switch (level) {
        case Info:
            color = "black";
            break;
        case Warning:
            color = "orange";
            break;
        case Error:
            color = "red";
            break;
        case Debug:
            color = "gray";
            break;
    }
    
    QString htmlMessage = QString("<span style='color: %1'>[%2] [%3] %4</span>")
                              .arg(color, timestamp, levelStr, message);
    
    m_logTextEdit->append(htmlMessage);
    
    QScrollBar* sb = m_logTextEdit->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void LogPanel::clearLog()
{
    m_logTextEdit->clear();
}

void LogPanel::saveLog()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save Log File",
                                                     QString(), "Text Files (*.txt);;All Files (*)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << m_logTextEdit->toPlainText();
            file.close();
        } else {
            QMessageBox::warning(this, tr("保存失败"),
                                tr("无法保存日志: %1").arg(file.errorString()));
        }
    }
}

void LogPanel::onClearButtonClicked()
{
    clearLog();
}

void LogPanel::onSaveButtonClicked()
{
    saveLog();
}

void LogPanel::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QGroupBox* groupBox = new QGroupBox("Log");
    QVBoxLayout* logLayout = new QVBoxLayout();
    
    m_logTextEdit = new QTextEdit();
    m_logTextEdit->setReadOnly(true);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_clearButton = new QPushButton("Clear");
    m_saveButton = new QPushButton("Save");
    
    buttonLayout->addWidget(m_clearButton);
    buttonLayout->addWidget(m_saveButton);
    buttonLayout->addStretch();
    
    logLayout->addWidget(m_logTextEdit);
    logLayout->addLayout(buttonLayout);
    
    groupBox->setLayout(logLayout);
    mainLayout->addWidget(groupBox);
    
    connect(m_clearButton, &QPushButton::clicked, this, &LogPanel::onClearButtonClicked);
    connect(m_saveButton, &QPushButton::clicked, this, &LogPanel::onSaveButtonClicked);
}

QString LogPanel::logLevelToString(LogLevel level)
{
    switch (level) {
        case Info:
            return "INFO";
        case Warning:
            return "WARNING";
        case Error:
            return "ERROR";
        case Debug:
            return "DEBUG";
        default:
            return "UNKNOWN";
    }
}

QString LogPanel::getCurrentTimestamp()
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
}
