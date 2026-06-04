#include "realtimeplotwidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QDateTime>
#include <QDebug>

RealtimePlotWidget::RealtimePlotWidget(QWidget* parent)
    : QWidget(parent)
    , m_timeRange(60)
    , m_yMin(0)
    , m_yMax(100)
    , m_bufferDirty(true)
    , m_isPanning(false)
{
    setMinimumSize(400, 300);
    setFocusPolicy(Qt::StrongFocus);
}

RealtimePlotWidget::~RealtimePlotWidget()
{
}

void RealtimePlotWidget::addDataSeries(const QString& name, const QColor& color)
{
    QMutexLocker locker(&m_dataMutex);
    DataSeries series;
    series.name = name;
    series.color = color;
    series.maxPoints = 10000;
    m_series[name] = series;
    m_bufferDirty = true;
    update();
}

void RealtimePlotWidget::removeDataSeries(const QString& name)
{
    QMutexLocker locker(&m_dataMutex);
    m_series.remove(name);
    m_bufferDirty = true;
    update();
}

void RealtimePlotWidget::setTimeRange(int seconds)
{
    m_timeRange = seconds;
    m_bufferDirty = true;
    update();
}

void RealtimePlotWidget::setYRange(double min, double max)
{
    m_yMin = min;
    m_yMax = max;
    m_bufferDirty = true;
    update();
}

void RealtimePlotWidget::setTitle(const QString& title)
{
    m_title = title;
    m_bufferDirty = true;
    update();
}

void RealtimePlotWidget::updateData(const QString& series, double value, const QDateTime& timestamp)
{
    QMutexLocker locker(&m_dataMutex);
    if (!m_series.contains(series)) {
        return;
    }
    
    DataSeries& s = m_series[series];
    qint64 timeMs = timestamp.isValid() ? timestamp.toMSecsSinceEpoch() : QDateTime::currentMSecsSinceEpoch();
    s.points.append(QPointF(timeMs, value));
    s.timestamps.append(timestamp.isValid() ? timestamp : QDateTime::currentDateTime());
    
    while (s.points.size() > s.maxPoints) {
        s.points.removeFirst();
        s.timestamps.removeFirst();
    }
    
    m_bufferDirty = true;
    update();
}

void RealtimePlotWidget::clearData()
{
    QMutexLocker locker(&m_dataMutex);
    for (auto& series : m_series) {
        series.points.clear();
        series.timestamps.clear();
    }
    m_bufferDirty = true;
    update();
}

void RealtimePlotWidget::clearSeries(const QString& name)
{
    QMutexLocker locker(&m_dataMutex);
    if (m_series.contains(name)) {
        m_series[name].points.clear();
        m_series[name].timestamps.clear();
        m_bufferDirty = true;
        update();
    }
}

void RealtimePlotWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    
    if (m_bufferDirty || m_buffer.size() != size()) {
        updateBuffer();
    }
    
    QPainter painter(this);
    painter.drawPixmap(0, 0, m_buffer);
}

void RealtimePlotWidget::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event);
    m_bufferDirty = true;
    update();
}

void RealtimePlotWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_isPanning = true;
        m_lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void RealtimePlotWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isPanning) {
        QPoint delta = event->pos() - m_lastMousePos;
        m_lastMousePos = event->pos();
        m_bufferDirty = true;
        update();
    }
}

void RealtimePlotWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_isPanning = false;
        unsetCursor();
    }
}

void RealtimePlotWidget::wheelEvent(QWheelEvent* event)
{
    double zoomFactor = event->angleDelta().y() > 0 ? 0.9 : 1.1;
    m_yMin *= zoomFactor;
    m_yMax *= zoomFactor;
    m_bufferDirty = true;
    update();
}

void RealtimePlotWidget::updateBuffer()
{
    m_buffer = QPixmap(size());
    m_buffer.fill(Qt::white);
    
    QPainter painter(&m_buffer);
    painter.setRenderHint(QPainter::Antialiasing);
    
    drawPlot(painter);
    m_bufferDirty = false;
}

void RealtimePlotWidget::drawPlot(QPainter& painter)
{
    int margin = 50;
    QRectF plotRect(margin, margin, width() - 2 * margin, height() - 2 * margin);
    
    painter.fillRect(rect(), Qt::white);
    
    drawGrid(painter, plotRect);
    drawAxes(painter, plotRect);
    drawDataSeries(painter, plotRect);
    
    if (!m_title.isEmpty()) {
        painter.setPen(Qt::black);
        painter.setFont(QFont("Arial", 12, QFont::Bold));
        painter.drawText(rect(), Qt::AlignTop | Qt::AlignHCenter, m_title);
    }
}

void RealtimePlotWidget::drawGrid(QPainter& painter, const QRectF& plotRect)
{
    painter.setPen(QColor(220, 220, 220));
    
    int numVerticalLines = 10;
    for (int i = 0; i <= numVerticalLines; ++i) {
        double x = plotRect.left() + i * plotRect.width() / numVerticalLines;
        painter.drawLine(QPointF(x, plotRect.top()), QPointF(x, plotRect.bottom()));
    }
    
    int numHorizontalLines = 8;
    for (int i = 0; i <= numHorizontalLines; ++i) {
        double y = plotRect.top() + i * plotRect.height() / numHorizontalLines;
        painter.drawLine(QPointF(plotRect.left(), y), QPointF(plotRect.right(), y));
    }
}

void RealtimePlotWidget::drawAxes(QPainter& painter, const QRectF& plotRect)
{
    painter.setPen(QColor(0, 0, 0));
    painter.setFont(QFont("Arial", 9));
    
    painter.drawLine(plotRect.bottomLeft(), plotRect.topLeft());
    painter.drawLine(plotRect.bottomLeft(), plotRect.bottomRight());
    
    int numYTicks = 6;
    for (int i = 0; i <= numYTicks; ++i) {
        double y = plotRect.bottom() - i * plotRect.height() / numYTicks;
        double value = m_yMin + i * (m_yMax - m_yMin) / numYTicks;
        
        painter.drawLine(QPointF(plotRect.left() - 5, y), QPointF(plotRect.left(), y));
        QString label = QString::number(value, 'f', 1);
        painter.drawText(QPointF(plotRect.left() - 45, y + 4), label);
    }
    
    int numXTicks = 6;
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    for (int i = 0; i <= numXTicks; ++i) {
        double x = plotRect.left() + i * plotRect.width() / numXTicks;
        qint64 timeMs = currentTime - (numXTicks - i) * m_timeRange * 1000 / numXTicks;
        
        painter.drawLine(QPointF(x, plotRect.bottom()), QPointF(x, plotRect.bottom() + 5));
        QDateTime time = QDateTime::fromMSecsSinceEpoch(timeMs);
        QString label = time.toString("hh:mm:ss");
        painter.drawText(QPointF(x - 20, plotRect.bottom() + 20), label);
    }
}

void RealtimePlotWidget::drawDataSeries(QPainter& painter, const QRectF& plotRect)
{
    QMutexLocker locker(&m_dataMutex);
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 startTime = currentTime - m_timeRange * 1000;
    
    for (auto it = m_series.begin(); it != m_series.end(); ++it) {
        const DataSeries& series = it.value();
        painter.setPen(QPen(series.color, 2));
        
        QPainterPath path;
        bool firstPoint = true;
        
        for (const QPointF& point : series.points) {
            if (point.x() >= startTime) {
                QPointF screenPoint = dataToScreen(point, plotRect);
                if (firstPoint) {
                    path.moveTo(screenPoint);
                    firstPoint = false;
                } else {
                    path.lineTo(screenPoint);
                }
            }
        }
        
        painter.drawPath(path);
    }
}

QPointF RealtimePlotWidget::dataToScreen(const QPointF& dataPoint, const QRectF& plotRect)
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 startTime = currentTime - m_timeRange * 1000;
    
    double x = (dataPoint.x() - startTime) / (m_timeRange * 1000.0);
    double y = 1.0 - (dataPoint.y() - m_yMin) / (m_yMax - m_yMin);
    
    x = qBound(0.0, x, 1.0);
    y = qBound(0.0, y, 1.0);
    
    return QPointF(
        plotRect.left() + x * plotRect.width(),
        plotRect.top() + y * plotRect.height()
    );
}

QPointF RealtimePlotWidget::screenToData(const QPointF& screenPoint, const QRectF& plotRect)
{
    double x = (screenPoint.x() - plotRect.left()) / plotRect.width();
    double y = 1.0 - (screenPoint.y() - plotRect.top()) / plotRect.height();
    
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 startTime = currentTime - m_timeRange * 1000;
    
    return QPointF(
        startTime + x * m_timeRange * 1000.0,
        m_yMin + y * (m_yMax - m_yMin)
    );
}
