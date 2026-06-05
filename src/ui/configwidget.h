#ifndef CONFIGWIDGET_H
#define CONFIGWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QGridLayout>
#include "../model/devicedatamodel.h"

class ConfigWidget : public QWidget {
    Q_OBJECT
public:
    explicit ConfigWidget(QWidget* parent = nullptr);

public slots:
    void updateData(const DeviceDataModel::RealTimeData& data);

    /// 获取控件中当前的配置值
    DeviceDataModel::ConfigBits getConfigData() const;

signals:
    void configChanged();

private:
    void setupUI();

    // CONFIG [03h]
    QCheckBox* m_tcEnCheck;
    QCheckBox* m_ccEnCheck;
    QCheckBox* m_aeEnCheck;
    QCheckBox* m_powerSvCheck;
    QCheckBox* m_curSvCheck;
    QCheckBox* m_tempSvCheck;
    QDoubleSpinBox* m_curThSpinBox;
    QDoubleSpinBox* m_curSlopeSpinBox;
    QComboBox* m_baudRateCombo;
    QPushButton* m_applyConfigButton;

    // DEVINFO [01h]
    QLabel* m_devNameLabel;
    QLabel* m_verSLabel;
    QLabel* m_verHLabel;
    QLabel* m_serialLabel;
    QLabel* m_curMaxLabel;
    QLabel* m_tempMinLabel;
    QLabel* m_tempMaxLabel;
    QPushButton* m_readDevInfoButton;

    // SYSTEM [00h]
    QPushButton* m_sleepButton;
    QPushButton* m_resetButton;
    QPushButton* m_updateButton;
};

#endif // CONFIGWIDGET_H
