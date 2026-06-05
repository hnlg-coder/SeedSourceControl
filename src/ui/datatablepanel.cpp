#include "datatablepanel.h"
#include <QStringConverter>
#include <QMessageBox>

/**
 * @brief 构造函数
 * @param parent 父窗口指针
 */
DataTablePanel::DataTablePanel(QWidget* parent)
    : QWidget(parent)
{
    initUI();
}

/**
 * @brief 析构函数
 */
DataTablePanel::~DataTablePanel()
{
}

/**
 * @brief 初始化UI
 */
void DataTablePanel::initUI()
{
    // 创建主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // 创建标题标签
    m_titleLabel = new QLabel("数据记录", this);
    m_titleLabel->setProperty("headerLabel", true);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_titleLabel);

    // 创建数据表格
    m_dataTable = new QTableWidget(this);
    m_dataTable->setColumnCount(6);
    m_dataTable->setHorizontalHeaderLabels(QStringList() 
        << "时间" << "电流(mA)" << "温度(°C)" << "功率(mW)" << "状态" << "报警");
    
    // 设置表格属性
    m_dataTable->horizontalHeader()->setStretchLastSection(true);
    m_dataTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_dataTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_dataTable->setAlternatingRowColors(true);
    
    mainLayout->addWidget(m_dataTable);

    // 创建按钮布局
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(8);
    
    m_clearButton = new QPushButton("清空", this);
    m_clearButton->setProperty("buttonType", "primary");
    m_exportButton = new QPushButton("导出CSV", this);
    m_exportButton->setProperty("buttonType", "primary");
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_clearButton);
    buttonLayout->addWidget(m_exportButton);
    
    mainLayout->addLayout(buttonLayout);

    // 连接信号槽
    connect(m_clearButton, &QPushButton::clicked, this, &DataTablePanel::clearTable);
    connect(m_exportButton, &QPushButton::clicked, this, &DataTablePanel::exportToCsv);
}

/**
 * @brief 添加数据行
 * @param data 实时数据
 */
void DataTablePanel::addDataRow(const DeviceDataModel::RealTimeData& data)
{
    // 获取当前行数
    int row = m_dataTable->rowCount();
    
    // 插入新行
    m_dataTable->insertRow(row);
    
    // 设置数据
    m_dataTable->setItem(row, 0, new QTableWidgetItem(data.timestamp.toString("yyyy-MM-dd HH:mm:ss.zzz")));
    m_dataTable->setItem(row, 1, new QTableWidgetItem(QString::number(data.current, 'f', 2)));
    m_dataTable->setItem(row, 2, new QTableWidgetItem(QString::number(data.temperature, 'f', 2)));
    m_dataTable->setItem(row, 3, new QTableWidgetItem(QString::number(data.power, 'f', 2)));
    m_dataTable->setItem(row, 4, new QTableWidgetItem(QString::number(data.statusRaw)));
    m_dataTable->setItem(row, 5, new QTableWidgetItem(QString::number(data.alarmRaw)));
    
    // 滚动到底部
    m_dataTable->scrollToBottom();
    
    // 限制最大行数（保持不超过1000行）
    if (row >= 1000) {
        m_dataTable->removeRow(0);
    }
}

/**
 * @brief 清空表格
 */
void DataTablePanel::clearTable()
{
    m_dataTable->setRowCount(0);
}

/**
 * @brief 导出数据到CSV文件
 */
void DataTablePanel::exportToCsv()
{
    // 选择保存路径
    QString fileName = QFileDialog::getSaveFileName(
        this, 
        "导出数据", 
        QString("data_%1.csv").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")),
        "CSV Files (*.csv)");
    
    if (fileName.isEmpty()) {
        return;
    }
    
    // 打开文件
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("导出失败"),
                            tr("无法打开文件: %1").arg(file.errorString()));
        return;
    }
    
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    
    // 写入表头
    out << "时间,电流(mA),温度(°C),功率(mW),状态,报警\n";
    
    // 写入数据
    for (int row = 0; row < m_dataTable->rowCount(); ++row) {
        QStringList items;
        for (int col = 0; col < m_dataTable->columnCount(); ++col) {
            QTableWidgetItem* item = m_dataTable->item(row, col);
            if (item) {
                items << item->text();
            } else {
                items << "";
            }
        }
        out << items.join(",") << "\n";
    }
    
    file.close();
}
