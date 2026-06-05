#ifndef STATISTICSCALCULATOR_H
#define STATISTICSCALCULATOR_H

#include <QObject>
#include <QVector>
#include <QReadWriteLock>
#include <QDateTime>
#include <cmath>

/**
 * @brief 统计结果结构体
 */
struct StatisticsResult {
    double mean;        // 平均值
    double variance;    // 方差
    double stdDev;      // 标准差
    double min;         // 最小值
    double max;         // 最大值
    double last;        // 最新值
    int count;          // 样本数
    QDateTime startTime; // 起始时间
    QDateTime endTime;   // 结束时间

    StatisticsResult()
        : mean(0), variance(0), stdDev(0), min(0), max(0), last(0), count(0) {}
};

/**
 * @brief 统计计算器类
 *
 * 线程安全的滑动窗口统计计算器，支持实时更新
 */
class StatisticsCalculator : public QObject {
    Q_OBJECT
public:
    explicit StatisticsCalculator(int windowSize = 1000, QObject* parent = nullptr);
    ~StatisticsCalculator();

    /**
     * @brief 添加一个数据点
     * @param value 数据值
     * @param timestamp 时间戳
     */
    void addValue(double value, const QDateTime& timestamp = QDateTime());

    /**
     * @brief 获取当前统计结果
     */
    StatisticsResult result() const;

    /**
     * @brief 清空数据
     */
    void clear();

    /**
     * @brief 设置滑动窗口大小
     */
    void setWindowSize(int size);

    /**
     * @brief 获取窗口大小
     */
    int windowSize() const { return m_windowSize; }

signals:
    /**
     * @brief 统计结果更新信号
     */
    void statisticsUpdated(const StatisticsResult& result);

private:
    QVector<double> m_values;
    QVector<QDateTime> m_timestamps;
    int m_windowSize;
    mutable QReadWriteLock m_lock;

    double m_min;
    double m_max;
};

#endif // STATISTICSCALCULATOR_H
