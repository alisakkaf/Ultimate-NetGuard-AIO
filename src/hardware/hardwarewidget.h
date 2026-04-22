/**
 * @file    hardwarewidget.h
 * @brief   Header for the modern, theme-aware hardware monitoring UI.
 */
#pragma once
#include <QWidget>
#include <QTimer>
#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>

#include "hardwaremonitor.h"

// ─── CircularGauge ───────────────────────────────────────────────────────────
class CircularGauge : public QWidget
{
    Q_OBJECT
public:
    explicit CircularGauge(const QString &label, QWidget *parent = nullptr);
    void setValue(double pct, double temp = 0.0);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QString m_label;
    double  m_value = 0.0;
    double  m_temp  = 0.0;
};

// ─── TempBar ─────────────────────────────────────────────────────────────────
class TempBar : public QWidget
{
    Q_OBJECT
public:
    explicit TempBar(const QString &label, QWidget *parent = nullptr);
    void setTemp(double c);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QString m_label;
    double  m_temp = 0.0;
};

// ─── HardwareWidget ──────────────────────────────────────────────────────────
namespace Ui { class HardwareWidget; }

class HardwareWidget : public QWidget
{
    Q_OBJECT
public:
    explicit HardwareWidget(QWidget *parent = nullptr);
    ~HardwareWidget() override;

    HardwareMonitor *monitor() const { return m_monitor; }

private slots:
    void onSnapshot(const HardwareSnapshot &snap);

private:
    Ui::HardwareWidget *ui      = nullptr;
    HardwareMonitor    *m_monitor = nullptr;

    CircularGauge      *m_gaugeCpu = nullptr;
    CircularGauge      *m_gaugeRam = nullptr;
    CircularGauge      *m_gaugeGpu = nullptr;

    TempBar            *m_tbCpu  = nullptr;
    TempBar            *m_tbGpu  = nullptr;
    TempBar            *m_tbDisk = nullptr;
    TempBar            *m_tbMb   = nullptr;
};
