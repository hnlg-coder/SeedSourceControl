#ifndef CONNECTIONSTATUSBAR_H
#define CONNECTIONSTATUSBAR_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>

class ConnectionStatusBar : public QWidget {
    Q_OBJECT
public:
    explicit ConnectionStatusBar(QWidget* parent = nullptr);

    void setConnected(bool connected, const QString& portName = QString(), qint32 baudRate = 0);
    bool isConnected() const { return m_connected; }

signals:
    void configToggled();
    void disconnectClicked();

private:
    void setupUI();

    QLabel* m_statusLabel;
    QPushButton* m_disconnectBtn;
    QPushButton* m_configBtn;
    bool m_connected;
    QString m_portName;
    qint32 m_baudRate;
};

#endif // CONNECTIONSTATUSBAR_H
