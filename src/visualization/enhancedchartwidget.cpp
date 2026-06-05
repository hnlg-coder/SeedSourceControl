#include "enhancedchartwidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QDateTime>
#include <QDebug>
#include <cmath>

// ===== RenderThread实现 =====

EnhancedChartWidget::RenderThread::RenderThread(EnhancedChartWidget* parent)
    : QThread(parent)
    , m_parent(parent)
    , m_running(false)
    , m_renderRequested(false)
    , timeRange(60)
    , yMin(0)
    , yMax(100)
    , autoScaleY(true)
    , showStatistics(true)
{
}

EnhancedChartWidget::RenderThread::~RenderThread()
{
    stop();
    wait();
}

void EnhancedChartWidget::RenderThread::run()
{
    m_running = true;
    while (m_running) {
        m_renderMutex.lock();
        if (!m_renderRequested) {
            m_renderCondition.wait(&m_renderMutex, 100);
        }
        m_renderRequested = false;
        m_renderMutex.unlock();

        if (!m_running) break;

        renderFrame();
    }
}

void EnhancedChartWidget::RenderThread::stop()
{
    m_running = false;
    m_renderMutex.lock();
    m_renderRequested = true;
    m_renderCondition.wakeAll();
    m_renderMutex.unlock();
}

void EnhancedChartWidget::RenderThread::requestRender()
{
    m_renderMutex.lock();
    m_renderRequested = true;
    m_renderCondition.wakeAll();
    m_renderMutex.unlock();
}

void EnhancedChartWidget::RenderThread::renderFrame()
{
    QSize size = loadRenderSize();
    if (size.width() < 10 || size.height() < 10) return;

    QImage image = renderToImage(size);
    if (!image.isNull()) {
        QMetaObject::invokeMethod(m_parent, "onRenderFrameReady",
                                  Qt::QueuedConnection,
                                  Q_ARG(QImage, image));
    }
}

QImage EnhancedChartWidget::RenderThread::renderToImage(const QSize& size)
{
    QImage image(size, QImage::Format_RGB32);
    image.fill(Qt::white);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);

    int margin = 60;
    int rightMargin = showStatistics ? 200 : 30;
    QRectF plotRect(margin, 40, size.width() - margin - rightMargin, size.height() - 80);

    painter.fillRect(image.rect(), QColor(250, 250, 250));

    if (!title.isEmpty()) {
        painter.setPen(Qt::black);
        painter.setFont(QFont("Microsoft YaHei", 11, QFont::Bold));
        painter.drawText(QRect(0, 5, size.width(), 30), Qt::AlignCenter, title);
    }

    // 网格
    painter.setPen(QPen(QColor(220, 220, 220), 1, Qt::DashLine));
    int numVLines = 6;
    int numHLines = 6;
    for (int i = 0; i <= numVLines; ++i) {
        double x = plotRect.left() + i * plotRect.width() / numVLines;
        painter.drawLine(QPointF(x, plotRect.top()), QPointF(x, plotRect.bottom()));
    }
    for (int i = 0; i <= numHLines; ++i) {
        double y = plotRect.top() + i * plotRect.height() / numHLines;
        painter.drawLine(QPointF(plotRect.left(), y), QPointF(plotRect.right(), y));
    }

    // 坐标轴
    painter.setPen(QPen(Qt::black, 1));
    painter.drawLine(plotRect.bottomLeft(), plotRect.topLeft());
    painter.drawLine(plotRect.bottomLeft(), plotRect.bottomRight());

    // 在锁内拷贝数据快照，避免长时间持锁
    dataMutex.lock();
    double localYMin = yMin;
    double localYMax = yMax;
    bool localAutoScale = autoScaleY;
    int localTimeRange = timeRange;

    // 拷贝所有系列数据到局部变量
    struct SeriesSnapshot {
        QString name;
        QColor color;
        bool visible;
        QVector<QPointF> points;
        StatisticsResult stats;
    };
    QVector<SeriesSnapshot> snapshots;
    snapshots.reserve(seriesData.size());

    for (auto it = seriesData.constBegin(); it != seriesData.constEnd(); ++it) {
        SeriesSnapshot snap;
        snap.name = it->name;
        snap.color = it->color;
        snap.visible = it->visible;
        snap.points = it->points;  // QVector隐式共享，拷贝很快
        if (it->stats) {
            snap.stats = it->stats->result();
        }
        snapshots.append(snap);
    }
    dataMutex.unlock();

    // 自动缩放计算（使用快照数据）
    if (localAutoScale) {
        bool hasData = false;
        double dataMin = 1e30, dataMax = -1e30;
        for (const auto& snap : snapshots) {
            if (!snap.visible || snap.points.isEmpty()) continue;
            for (const QPointF& p : snap.points) {
                if (p.y() < dataMin) dataMin = p.y();
                if (p.y() > dataMax) dataMax = p.y();
                hasData = true;
            }
        }
        if (hasData) {
            double m = (dataMax - dataMin) * 0.1;
            if (m < 0.01) m = 1.0;
            localYMin = dataMin - m;
            localYMax = dataMax + m;
        }
    }

    // 防止除以零：确保Y轴范围有效
    if (localYMax - localYMin < 1e-9) {
        localYMax = localYMin + 1.0;
    }

    // 防止除以零：确保时间范围有效
    if (localTimeRange <= 0) {
        localTimeRange = 60;
    }

    // Y轴标签
    painter.setFont(QFont("Consolas", 8));
    for (int i = 0; i <= numHLines; ++i) {
        double y = plotRect.bottom() - i * plotRect.height() / numHLines;
        double value = localYMin + i * (localYMax - localYMin) / numHLines;
        painter.setPen(Qt::black);
        painter.drawLine(QPointF(plotRect.left() - 5, y), QPointF(plotRect.left(), y));
        painter.drawText(QPointF(plotRect.left() - 55, y + 4), QString::number(value, 'f', 2));
    }

    // X轴标签
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    for (int i = 0; i <= numVLines; ++i) {
        double x = plotRect.left() + i * plotRect.width() / numVLines;
        qint64 timeMs = currentTime - (numVLines - i) * localTimeRange * 1000 / numVLines;
        painter.setPen(Qt::black);
        painter.drawLine(QPointF(x, plotRect.bottom()), QPointF(x, plotRect.bottom() + 5));
        QDateTime time = QDateTime::fromMSecsSinceEpoch(timeMs);
        painter.drawText(QPointF(x - 25, plotRect.bottom() + 20), time.toString("hh:mm:ss"));
    }

    // 绘制数据曲线（使用快照）
    qint64 startTime = currentTime - localTimeRange * 1000;
    int legendY = 50;

    for (const auto& snap : snapshots) {
        if (!snap.visible) continue;

        painter.setPen(QPen(snap.color, 2));
        QPainterPath path;
        bool firstPoint = true;

        for (const QPointF& point : snap.points) {
            if (point.x() >= startTime) {
                double x = (point.x() - startTime) / (localTimeRange * 1000.0);
                double y = 1.0 - (point.y() - localYMin) / (localYMax - localYMin);
                x = qBound(0.0, x, 1.0);
                y = qBound(0.0, y, 1.0);

                QPointF screenPoint(
                    plotRect.left() + x * plotRect.width(),
                    plotRect.top() + y * plotRect.height()
                );

                if (firstPoint) {
                    path.moveTo(screenPoint);
                    firstPoint = false;
                } else {
                    path.lineTo(screenPoint);
                }
            }
        }

        if (!path.isEmpty()) {
            painter.drawPath(path);
        }

        // 图例
        painter.setPen(QPen(snap.color, 3));
        painter.drawLine(margin, legendY, margin + 25, legendY);
        painter.setPen(Qt::black);
        painter.setFont(QFont("Microsoft YaHei", 8));
        painter.drawText(margin + 30, legendY + 4, snap.name);
        legendY += 18;
    }

    // 统计信息面板（使用快照中的统计结果）
    if (showStatistics) {
        int statsX = plotRect.right() + 10;
        int statsY = plotRect.top();
        int visibleCount = 0;
        for (const auto& snap : snapshots) {
            if (snap.visible) visibleCount++;
        }
        int statsBoxHeight = visibleCount * 100 + 10;
        painter.setPen(QPen(QColor(200, 200, 200), 1));
        painter.setBrush(QColor(245, 245, 245));
        painter.drawRect(statsX, statsY, 190, statsBoxHeight);

        int offsetY = statsY + 8;
        for (const auto& snap : snapshots) {
            if (!snap.visible) continue;

            const StatisticsResult& sr = snap.stats;
            painter.setPen(snap.color);
            painter.setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
            painter.drawText(statsX + 8, offsetY + 14, snap.name);
            painter.setFont(QFont("Consolas", 9));
            painter.setPen(Qt::darkGray);
            painter.drawText(statsX + 8, offsetY + 30, QString("均值: %1").arg(sr.mean, 0, 'f', 3));
            painter.drawText(statsX + 8, offsetY + 44, QString("标准差: %1").arg(sr.stdDev, 0, 'f', 3));
            painter.drawText(statsX + 8, offsetY + 58, QString("最小: %1").arg(sr.min, 0, 'f', 3));
            painter.drawText(statsX + 8, offsetY + 72, QString("最大: %1").arg(sr.max, 0, 'f', 3));
            painter.drawText(statsX + 8, offsetY + 86, QString("样本: %1").arg(sr.count));
            offsetY += 100;
        }
    }

    return image;
}

// ===== EnhancedChartWidget实现 =====

EnhancedChartWidget::EnhancedChartWidget(QWidget* parent)
    : QWidget(parent)
    , m_isPanning(false)
{
    setMinimumSize(400, 300);
    setFocusPolicy(Qt::StrongFocus);
    startRenderThread();
}

EnhancedChartWidget::~EnhancedChartWidget()
{
    if (m_renderThread) {
        m_renderThread->stop();
        m_renderThread->wait();

        // 清理StatisticsCalculator对象（渲染线程已停止，安全删除）
        {
            QMutexLocker locker(&m_renderThread->dataMutex);
            for (auto& series : m_renderThread->seriesData) {
                delete series.stats;
                series.stats = nullptr;
            }
        }

        delete m_renderThread;
    }
}

void EnhancedChartWidget::startRenderThread()
{
    m_renderThread = new RenderThread(this);
    m_renderThread->storeRenderSize(size());
    m_renderThread->start();
}

void EnhancedChartWidget::addSeries(const QString& name, const QColor& color, const QString& unit, bool visible)
{
    QMutexLocker locker(&m_renderThread->dataMutex);
    // 检查是否已存在同名系列，避免覆盖导致内存泄漏
    if (m_renderThread->seriesData.contains(name)) {
        delete m_renderThread->seriesData[name].stats;
    }
    DataSeries series;
    series.name = name;
    series.color = color;
    series.unit = unit;
    series.visible = visible;
    series.maxPoints = 10000;
    // StatisticsCalculator无父对象，避免跨线程父子关系问题
    series.stats = new StatisticsCalculator(1000, nullptr);
    m_renderThread->seriesData[name] = series;
}

void EnhancedChartWidget::removeSeries(const QString& name)
{
    QMutexLocker locker(&m_renderThread->dataMutex);
    auto it = m_renderThread->seriesData.find(name);
    if (it != m_renderThread->seriesData.end()) {
        delete it->stats;
        it->stats = nullptr;
        m_renderThread->seriesData.erase(it);
    }
}

void EnhancedChartWidget::setTimeRange(int seconds)
{
    QMutexLocker locker(&m_renderThread->dataMutex);
    m_renderThread->timeRange = seconds;
}

void EnhancedChartWidget::setYRange(double min, double max)
{
    QMutexLocker locker(&m_renderThread->dataMutex);
    m_renderThread->yMin = min;
    m_renderThread->yMax = max;
    m_renderThread->autoScaleY = false;
}

void EnhancedChartWidget::setAutoScaleY(bool enabled)
{
    QMutexLocker locker(&m_renderThread->dataMutex);
    m_renderThread->autoScaleY = enabled;
}

void EnhancedChartWidget::setTitle(const QString& title)
{
    QMutexLocker locker(&m_renderThread->dataMutex);
    m_renderThread->title = title;
}

void EnhancedChartWidget::setShowStatistics(bool enabled)
{
    QMutexLocker locker(&m_renderThread->dataMutex);
    m_renderThread->showStatistics = enabled;
}

StatisticsResult EnhancedChartWidget::statistics(const QString& seriesName)
{
    QMutexLocker locker(&m_renderThread->dataMutex);
    auto it = m_renderThread->seriesData.find(seriesName);
    if (it != m_renderThread->seriesData.end() && it->stats) {
        return it->stats->result();
    }
    return StatisticsResult();
}

void EnhancedChartWidget::setSeriesVisible(const QString& name, bool visible)
{
    QMutexLocker locker(&m_renderThread->dataMutex);
    if (m_renderThread->seriesData.contains(name)) {
        m_renderThread->seriesData[name].visible = visible;
    }
}

void EnhancedChartWidget::updateData(const QString& series, double value, const QDateTime& timestamp)
{
    QMutexLocker locker(&m_renderThread->dataMutex);
    auto it = m_renderThread->seriesData.find(series);
    if (it == m_renderThread->seriesData.end()) return;

    DataSeries& s = *it;
    qint64 timeMs = timestamp.isValid() ? timestamp.toMSecsSinceEpoch() : QDateTime::currentMSecsSinceEpoch();
    s.points.append(QPointF(timeMs, value));
    s.timestamps.append(timestamp.isValid() ? timestamp : QDateTime::currentDateTime());

    while (s.points.size() > s.maxPoints) {
        s.points.removeFirst();
        s.timestamps.removeFirst();
    }

    // 更新统计（在dataMutex保护下，StatisticsCalculator内部也有自己的锁）
    if (s.stats) {
        s.stats->addValue(value, timestamp.isValid() ? timestamp : QDateTime::currentDateTime());
    }

    locker.unlock();
    m_renderThread->requestRender();
}

void EnhancedChartWidget::clearData()
{
    QMutexLocker locker(&m_renderThread->dataMutex);
    for (auto& series : m_renderThread->seriesData) {
        series.points.clear();
        series.timestamps.clear();
        if (series.stats) series.stats->clear();
    }
    locker.unlock();
    m_renderThread->requestRender();
}

void EnhancedChartWidget::clearSeries(const QString& name)
{
    QMutexLocker locker(&m_renderThread->dataMutex);
    if (m_renderThread->seriesData.contains(name)) {
        m_renderThread->seriesData[name].points.clear();
        m_renderThread->seriesData[name].timestamps.clear();
        if (m_renderThread->seriesData[name].stats)
            m_renderThread->seriesData[name].stats->clear();
    }
    locker.unlock();
    m_renderThread->requestRender();
}

void EnhancedChartWidget::onRenderFrameReady(const QImage& frame)
{
    {
        QMutexLocker locker(&m_frameMutex);
        m_currentFrame = frame;
    }
    update();
}

void EnhancedChartWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);

    QMutexLocker locker(&m_frameMutex);
    if (!m_currentFrame.isNull()) {
        painter.drawImage(0, 0, m_currentFrame.scaled(size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    }
}

void EnhancedChartWidget::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event);
    m_renderThread->storeRenderSize(size());
    m_renderThread->requestRender();
}

void EnhancedChartWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_isPanning = true;
        m_lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void EnhancedChartWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isPanning) {
        m_lastMousePos = event->pos();
    }
}

void EnhancedChartWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_isPanning = false;
        unsetCursor();
    }
}

void EnhancedChartWidget::wheelEvent(QWheelEvent* event)
{
    double factor = event->angleDelta().y() > 0 ? 0.9 : 1.1;
    {
        QMutexLocker locker(&m_renderThread->dataMutex);
        if (!m_renderThread->autoScaleY) {
            double center = (m_renderThread->yMin + m_renderThread->yMax) / 2;
            double range = (m_renderThread->yMax - m_renderThread->yMin) * factor;
            m_renderThread->yMin = center - range / 2;
            m_renderThread->yMax = center + range / 2;
        }
    }
    m_renderThread->requestRender();
}
