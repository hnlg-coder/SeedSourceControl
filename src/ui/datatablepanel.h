#ifndef DATATABLEPANEL_H
#define DATATABLEPANEL_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QDateTime>
#include "../model/devicedatamodel.h"

/**
 * @brief 数据表格面板类
 * 
 * 用于显示历史数据的表格组件
 */
class DataTablePanel : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针
     */
    explicit DataTablePanel(QWidget* parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~DataTablePanel();

public slots:
    /**
     * @brief 添加数据行
     * @param data 实时数据
     */
    void addDataRow(const DeviceDataModel::RealTimeData& data);
    
    /**
     * @brief 清空表格
     */
    void clearTable();
    
    /**
     * @brief 导出数据到CSV文件
     */
    void exportToCsv();

private:
    /**
     * @brief 初始化UI
     */
    void initUI();

    QTableWidget* m_dataTable;          // 数据表格
    QPushButton* m_clearButton;         // 清空按钮
    QPushButton* m_exportButton;        // 导出按钮
    QLabel* m_titleLabel;               // 标题标签
};

#endif // DATATABLEPANEL_H
