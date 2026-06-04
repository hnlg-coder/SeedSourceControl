#ifndef CONNECTIONPANEL_H
#define CONNECTIONPANEL_H

#include <QWidget>
#include <QSerialPort>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QGroupBox>

/**
 * @brief 连接面板类
 * 
 * 提供串口连接配置和连接控制的UI界面
 */
class ConnectionPanel : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针
     */
    explicit ConnectionPanel(QWidget* parent = nullptr);
    
    // 获取当前配置的串口参数
    QString selectedPort() const;                    // 串口名称
    qint32 selectedBaudRate() const;              // 波特率
    QSerialPort::DataBits selectedDataBits() const; // 数据位
    QSerialPort::Parity selectedParity() const;   // 校验位
    QSerialPort::StopBits selectedStopBits() const; // 停止位
    QSerialPort::FlowControl selectedFlowControl() const; // 流控制
    
    /**
     * @brief 设置连接状态
     * @param connected 是否已连接
     */
    void setConnected(bool connected);
    
public slots:
    /**
     * @brief 刷新串口列表
     */
    void refreshPorts();
    
signals:
    /**
     * @brief 连接按钮点击信号
     */
    void connectClicked();
    
    /**
     * @brief 断开按钮点击信号
     */
    void disconnectClicked();
    
private slots:
    /**
     * @brief 连接按钮点击槽
     */
    void onConnectButtonClicked();
    
private:
    /**
     * @brief 设置UI界面
     */
    void setupUI();
    
    /**
     * @brief 填充波特率选项
     */
    void populateBaudRates();
    
    /**
     * @brief 填充数据位选项
     */
    void populateDataBits();
    
    /**
     * @brief 填充校验位选项
     */
    void populateParity();
    
    /**
     * @brief 填充停止位选项
     */
    void populateStopBits();
    
    /**
     * @brief 填充流控制选项
     */
    void populateFlowControl();
    
    // UI控件成员变量
    QComboBox* m_portCombo;           // 串口选择下拉框
    QComboBox* m_baudRateCombo;       // 波特率下拉框
    QComboBox* m_dataBitsCombo;       // 数据位下拉框
    QComboBox* m_parityCombo;         // 校验位下拉框
    QComboBox* m_stopBitsCombo;       // 停止位下拉框
    QComboBox* m_flowControlCombo;    // 流控制下拉框
    QPushButton* m_connectButton;      // 连接按钮
    QPushButton* m_refreshButton;      // 刷新按钮
    QLabel* m_statusLabel;             // 状态标签
    
    bool m_connected; // 连接状态
};

#endif // CONNECTIONPANEL_H
