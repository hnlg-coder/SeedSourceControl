#ifndef REALTIMEPLOTWIDGET_H
#define REALTIMEPLOTWIDGET_H

#include <QWidget>
#include <QMap>
#include <QColor>
#include <QVector>
#include <QMutex>
#include <QPixmap>
#include <QTransform>
#include <QDateTime>

class RealtimePlotWidget : public QWidget {
    Q_OBJECT
public:
    explicit RealtimePlotWidget(QWidget* parent = nullptr);
    ~RealtimePlotWidget();
    
    void addDataSeries(const QString& name, const QColor& color);
    void removeDataSeries(const QString& name);
    void setTimeRange(int seconds);
    void setYRange(double min, double max);
    void setTitle(const QString& title);
    
public slots:
    void updateData(const QString& series, double value, const QDateTime& timestamp = QDateTime());
    void clearData();
    void clearSeries(const QString& name);
    
protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    
private:
    struct DataSeries {
        QString name;
        QColor color;
        QVector<QPointF> points;
        QVector<QDateTime> timestamps;
        int maxPoints;
    };
    
    void drawPlot(QPainter& painter);
    void drawGrid(QPainter& painter, const QRectF& plotRect);
    void drawAxes(QPainter& painter, const QRectF& plotRect);
    void drawDataSeries(QPainter& painter, const QRectF& plotRect);
    void updateBuffer();
    QPointF dataToScreen(const QPointF& dataPoint, const QRectF& plotRect);
    QPointF screenToData(const QPointF& screenPoint, const QRectF& plotRect);
    
    QMap<QString, DataSeries> m_series;
    QMutex m_dataMutex;
    
    int m_timeRange;
    double m_yMin;
    double m_yMax;
    QString m_title;
    
    QPixmap m_buffer;
    bool m_bufferDirty;
    bool m_isPanning;
    QPoint m_lastMousePos;
    QRectF m_dataBounds;
    QTransform m_transform;
};

#endif // REALTIMEPLOTWIDGET_H
