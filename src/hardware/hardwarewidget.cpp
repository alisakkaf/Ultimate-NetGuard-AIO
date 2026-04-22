/**
 * @file    hardwarewidget.cpp
 * @brief   Hardware monitoring UI implementation with dynamic theme-aware drawing.
 */
#include "hardwarewidget.h"
#include "ui_hardwarewidget.h"
#include <QStyle>
#include <QApplication>
#include <QPainter>
#include <QPalette>

// ────────────────────────────────────────────────────────────────────────────
// CircularGauge
// ────────────────────────────────────────────────────────────────────────────
CircularGauge::CircularGauge(const QString &label, QWidget *parent)
    : QWidget(parent), m_label(label), m_value(0.0), m_temp(0.0)
{
    // Fix size to maintain a perfect circle aspect ratio
    setMinimumSize(180, 180);
    setMaximumSize(220, 220);
}

void CircularGauge::setValue(double pct, double temp)
{
    m_value = qBound(0.0, pct, 100.0);
    m_temp  = temp;
    update();
}

void CircularGauge::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Extract dynamic colors from the active palette (Dark/Light aware)
    QColor textColor = palette().color(QPalette::WindowText);
    QColor trackColor = textColor;
    trackColor.setAlpha(35);
    QColor subTextColor = textColor;
    subTextColor.setAlpha(160);

    const QRectF r = QRectF(14, 14, width() - 28, height() - 28);
    const double angle = 270.0 * (m_value / 100.0);

    // 1. Draw Background Track
    p.setPen(QPen(trackColor, 10.0, Qt::SolidLine, Qt::RoundCap));
    p.setBrush(Qt::NoBrush);
    p.drawArc(r, 225 * 16, -270 * 16);

    // 2. Draw Value Arc based on load level
    QColor arcColor;
    if (m_value < 60.0)       arcColor = QColor("#3B82F6"); // Modern Blue
    else if (m_value < 85.0)  arcColor = QColor("#F59E0B"); // Warning Amber
    else                      arcColor = QColor("#EF4444"); // Danger Red

    if (m_value > 0.0) {
        p.setPen(QPen(arcColor, 10.0, Qt::SolidLine, Qt::RoundCap));
        p.drawArc(r, 225 * 16, static_cast<int>(-angle * 16));
    }

    // 3. Center Text (Percentage)
    p.setPen(textColor);
    QFont fv("Segoe UI", 18, QFont::Bold);
    p.setFont(fv);
    p.drawText(r.adjusted(0, 4, 0, 0),
               Qt::AlignHCenter | Qt::AlignVCenter,
               QString("%1%").arg(static_cast<int>(m_value)));

    // 4. Sub Text (Temperature) - Will show N/A safely if 0
    if (m_temp > 0.0) {
        QFont ft("Segoe UI", 8, QFont::Bold);
        p.setFont(ft);
        p.setPen(subTextColor);
        p.drawText(r.adjusted(0, r.height() * 0.32, 0, 0),
                   Qt::AlignHCenter | Qt::AlignVCenter,
                   QString("%1 °C").arg(m_temp, 0, 'f', 1));
    }

    // 5. Outer Label (Hardware Name)
    QFont fl("Segoe UI", 9, QFont::Bold);
    p.setFont(fl);
    p.setPen(subTextColor);
    p.drawText(QRectF(0, height() - 22, width(), 20),
               Qt::AlignCenter, m_label);
}

// ────────────────────────────────────────────────────────────────────────────
// TempBar
// ────────────────────────────────────────────────────────────────────────────
TempBar::TempBar(const QString &label, QWidget *parent)
    : QWidget(parent), m_label(label), m_temp(0.0)
{
    setFixedHeight(32);
}

void TempBar::setTemp(double c)
{
    m_temp = c;
    update();
}

void TempBar::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QColor textColor = palette().color(QPalette::WindowText);
    QColor trackColor = textColor;
    trackColor.setAlpha(25);
    QColor subTextColor = textColor;
    subTextColor.setAlpha(160);

    const int W = width(), H = height();
    const int barX = 85, barW = W - barX - 65;

    // 1. Hardware Label
    p.setPen(subTextColor);
    p.setFont(QFont("Segoe UI", 9, QFont::Bold));
    p.drawText(QRect(0, 0, barX - 10, H), Qt::AlignVCenter | Qt::AlignRight, m_label + ":");

    // 2. Track Background
    p.setBrush(trackColor);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(barX, (H - 8) / 2, barW, 8, 4, 4);

    // 3. Colored Fill Bar
    double ratio = qBound(0.0, m_temp / 120.0, 1.0);
    QColor fc = (m_temp < 60.0) ? QColor("#3B82F6")
                : (m_temp < 85.0) ? QColor("#F59E0B")
                                  : QColor("#EF4444");

    if (ratio > 0.0) {
        p.setBrush(fc);
        p.drawRoundedRect(barX, (H - 8) / 2, static_cast<int>(barW * ratio), 8, 4, 4);
    }

    // 4. Value Text
    p.setPen(textColor);
    p.setFont(QFont("Segoe UI", 9, QFont::Bold));
    p.drawText(QRect(barX + barW + 8, 0, 55, H), Qt::AlignVCenter,
               m_temp > 0.0
                   ? QString("%1 °C").arg(m_temp, 0, 'f', 1)
                   : QString("N/A"));
}

// ────────────────────────────────────────────────────────────────────────────
// HardwareWidget
// ────────────────────────────────────────────────────────────────────────────
HardwareWidget::HardwareWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::HardwareWidget)
{
    ui->setupUi(this);

    auto inject = [](QWidget *container, QWidget *widget) {
        QVBoxLayout *l = new QVBoxLayout(container);
        l->setContentsMargins(0, 0, 0, 0);
        l->addWidget(widget);
    };

    m_gaugeCpu = new CircularGauge("CPU",  this);
    m_gaugeRam = new CircularGauge("RAM",  this);
    m_gaugeGpu = new CircularGauge("GPU",  this);
    inject(ui->gaugeContainerCpu, m_gaugeCpu);
    inject(ui->gaugeContainerRam, m_gaugeRam);
    inject(ui->gaugeContainerGpu, m_gaugeGpu);

    m_tbCpu  = new TempBar("CPU",  this);
    m_tbGpu  = new TempBar("GPU",  this);
    m_tbDisk = new TempBar("Disk", this);
    m_tbMb   = new TempBar("MB",   this);
    inject(ui->tempContainerCpu,  m_tbCpu);
    inject(ui->tempContainerGpu,  m_tbGpu);
    inject(ui->tempContainerDisk, m_tbDisk);
    inject(ui->tempContainerMb,   m_tbMb);

    m_monitor = new HardwareMonitor(this);
    connect(m_monitor, &HardwareMonitor::snapshotReady,
            this, &HardwareWidget::onSnapshot, Qt::QueuedConnection);
    m_monitor->startMonitor();
}

HardwareWidget::~HardwareWidget()
{
    m_monitor->stopMonitor();
    m_monitor->wait(3000);
    delete ui;
}

void HardwareWidget::onSnapshot(const HardwareSnapshot &snap)
{
    // Dispatch values to UI elements
    m_gaugeCpu->setValue(snap.cpuLoadPct, snap.cpuTempC);
    m_gaugeRam->setValue(snap.ramLoadPct, 0.0);
    m_gaugeGpu->setValue(snap.gpuLoadPct, snap.gpuTempC);

    m_tbCpu->setTemp(snap.cpuTempC);
    m_tbGpu->setTemp(snap.gpuTempC);
    m_tbDisk->setTemp(snap.diskTempC);
    m_tbMb->setTemp(snap.mbTempC);

    // Updating formatting without the unnecessary unsigned comparison warning
    auto fmt = [](quint64 bps) -> QString {
        if (bps < 1024)      return QString("%1 B/s").arg(bps);
        if (bps < 1024 * 1024) return QString("%1 KB/s").arg(bps / 1024);
        return QString("%1 MB/s").arg(bps / (1024 * 1024));
    };
    ui->lblNetRx->setText("↓  " + fmt(snap.netRxBps));
    ui->lblNetTx->setText("↑  " + fmt(snap.netTxBps));

    ui->lblRamDetail->setText(
        QString("%1 / %2 MB  (%3%)")
            .arg(static_cast<int>(snap.ramUsedMB))
            .arg(static_cast<int>(snap.ramTotalMB))
            .arg(static_cast<int>(snap.ramLoadPct)));
}
