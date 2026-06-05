#include "statisticscalculator.h"

StatisticsCalculator::StatisticsCalculator(int windowSize, QObject* parent)
    : QObject(parent)
    , m_windowSize(windowSize)
    , m_mean(0.0)
    , m_m2(0.0)
    , m_count(0)
    , m_min(0.0)
    , m_max(0.0)
{
}

StatisticsCalculator::~StatisticsCalculator()
{
}

void StatisticsCalculator::addValue(double value, const QDateTime& timestamp)
{
    QWriteLocker locker(&m_lock);

    // 添加新值
    m_values.append(value);
    m_timestamps.append(timestamp.isValid() ? timestamp : QDateTime::currentDateTime());

    // 维护滑动窗口
    while (m_values.size() > m_windowSize) {
        m_values.removeFirst();
        m_timestamps.removeFirst();
    }

    // 更新极值
    if (m_values.size() == 1) {
        m_min = value;
        m_max = value;
    } else {
        if (value < m_min) m_min = value;
        if (value > m_max) m_max = value;
    }

    // 不在锁内emit信号，避免跨线程信号问题和死锁
    // 信号由调用方在合适的线程上下文中处理
}

StatisticsResult StatisticsCalculator::result() const
{
    QReadLocker locker(&m_lock);

    StatisticsResult r;
    r.count = m_values.size();
    if (r.count == 0) return r;

    // 直接精确计算（滑动窗口模式下最可靠）
    double sum = 0.0, sumSq = 0.0;
    double localMin = m_values[0], localMax = m_values[0];
    for (double v : m_values) {
        sum += v;
        if (v < localMin) localMin = v;
        if (v > localMax) localMax = v;
    }
    double mean = sum / r.count;
    for (double v : m_values) {
        double d = v - mean;
        sumSq += d * d;
    }

    r.mean = mean;
    r.last = m_values.last();
    r.min = localMin;
    r.max = localMax;
    r.variance = (r.count > 1) ? (sumSq / r.count) : 0.0;
    if (r.variance < 0) r.variance = 0;
    r.stdDev = std::sqrt(r.variance);

    if (!m_timestamps.isEmpty()) {
        r.startTime = m_timestamps.first();
        r.endTime = m_timestamps.last();
    }

    return r;
}

void StatisticsCalculator::clear()
{
    QWriteLocker locker(&m_lock);
    m_values.clear();
    m_timestamps.clear();
    m_mean = 0.0;
    m_m2 = 0.0;
    m_count = 0;
    m_min = 0.0;
    m_max = 0.0;
}

void StatisticsCalculator::setWindowSize(int size)
{
    QWriteLocker locker(&m_lock);
    m_windowSize = size;
    while (m_values.size() > m_windowSize) {
        m_values.removeFirst();
        m_timestamps.removeFirst();
    }
}

void StatisticsCalculator::recalculate()
{
    // 内部调用，调用方已持有写锁
    m_count = m_values.size();
    if (m_count == 0) {
        m_mean = 0; m_m2 = 0; m_min = 0; m_max = 0;
        return;
    }

    double sum = 0.0;
    m_min = m_values[0];
    m_max = m_values[0];

    for (double v : m_values) {
        sum += v;
        if (v < m_min) m_min = v;
        if (v > m_max) m_max = v;
    }

    m_mean = sum / m_count;

    double sumSq = 0.0;
    for (double v : m_values) {
        double d = v - m_mean;
        sumSq += d * d;
    }
    m_m2 = sumSq;
}
