#ifndef ENHANCEDCHARTWIDGET_H
#define ENHANCEDCHARTWIDGET_H

#include <QWidget>
#include <QMap>
#include <QColor>
#include <QVector>
#include <QMutex>
#include <QPixmap>
#include <QThread>
#include <QQueue>
#include <QWaitCondition>
#include <QDateTime>
#include <atomic>
#include "statisticscalculator.h"

/**
 * @brief 增强型实时图表控件
 *
 * 支持多数据系列、多线程渲染、统计信息叠加显示
 * 数据收集在调用线程，渲染在独立渲染线程，UI线程只负责显示
 */
class EnhancedChartWidget : public QWidget {
    Q_OBJECT
public:
    explicit EnhancedChartWidget(QWidget* parent = nullptr);
    ~EnhancedChartWidget();

    /**
     * @brief 添加数据系列
     * @param name 系列名称
     * @param color 线条颜色
     * @param unit Y轴单位
     * @param visible 是否可见
     */
    void addSeries(const QString& name, const QColor& color, const QString& unit = "", bool visible = true);

    /**
     * @brief 移除数据系列
     */
    void removeSeries(const QString& name);

    /**
     * @brief 设置时间范围（秒）
     */
    void setTimeRange(int seconds);

    /**
     * @brief 设置Y轴范围
     */
    void setYRange(double min, double max);

    /**
     * @brief 启用/禁用自动Y轴缩放
     */
    void setAutoScaleY(bool enabled);

    /**
     * @brief 设置标题
     */
    void setTitle(const QString& title);

    /**
     * @brief 启用/禁用统计信息显示
     */
    void setShowStatistics(bool enabled);

    /**
     * @brief 获取指定系列的统计计算器
     */
    StatisticsCalculator* statistics(const QString& seriesName);

    /**
     * @brief 设置系列可见性
     */
    void setSeriesVisible(const QString& name, bool visible);

public slots:
    /**
     * @brief 更新数据点
     */
    void updateData(const QString& series, double value, const QDateTime& timestamp = QDateTime());

    /**
     * @brief 清空所有数据
     */
    void clearData();

    /**
     * @brief 清空指定系列数据
     */
    void clearSeries(const QString& name);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private slots:
    void onRenderFrameReady(const QImage& frame);

private:
    struct DataSeries {
        QString name;
        QColor color;
        QString unit;
        bool visible;
        QVector<QPointF> points;        // x=msSinceEpoch, y=value
        QVector<QDateTime> timestamps;
        int maxPoints;
        StatisticsCalculator* stats;
    };

    // 渲染线程
    class RenderThread : public QThread {
    public:
        explicit RenderThread(EnhancedChartWidget* parent);
        ~RenderThread();

        void run() override;
        void stop();

        void requestRender();

        // 共享数据（由m_dataMutex保护）
        QMap<QString, DataSeries> seriesData;
        QMutex dataMutex;
        int timeRange;
        double yMin, yMax;
        bool autoScaleY;
        QString title;
        bool showStatistics;
        std::atomic<int> renderWidth;
        std::atomic<int> renderHeight;

        QSize loadRenderSize() const {
            return QSize(renderWidth.load(), renderHeight.load());
        }
        void storeRenderSize(const QSize& size) {
            renderWidth.store(size.width());
            renderHeight.store(size.height());
        }

    private:
        void renderFrame();
        QImage renderToImage(const QSize& size);

        EnhancedChartWidget* m_parent;
        QMutex m_renderMutex;
        QWaitCondition m_renderCondition;
        std::atomic<bool> m_running;
        std::atomic<bool> m_renderRequested;
    };

    friend class RenderThread;

    RenderThread* m_renderThread;
    QImage m_currentFrame;
    QMutex m_frameMutex;

    bool m_isPanning;
    QPoint m_lastMousePos;

    void startRenderThread();
};

#endif // ENHANCEDCHARTWIDGET_H
