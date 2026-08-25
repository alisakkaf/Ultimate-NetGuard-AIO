/**
 * @file    taskbaroverlay.h
 * @brief   Frameless overlay widget that attaches precisely to the Taskbar.
 * Includes a professional Hover Stats Popup, Smart Resizing, & Right-Click Menu.
 * Supports dynamic CPU/RAM/GPU/Temperature display alongside network speed.
 * @author  Ali Sakkaf
 */
#pragma once

#include <QWidget>
#include <QTimer>
#include <QLabel>
#include <QColor>
#include <QMouseEvent>
#include <QGridLayout>
#include "network/networkmonitor.h"

// ── RAM display format for overlay ──
enum class RamDisplayMode {
    Percentage = 0,   // e.g. "65%"
    TotalMB    = 1,   // e.g. "12045/16384 MB"
    TotalGB    = 2    // e.g. "12/16 GB"
};

class StatsPopup;

class TaskbarOverlay : public QWidget
{
    Q_OBJECT
public:
    explicit TaskbarOverlay(QWidget *parent = nullptr);
    ~TaskbarOverlay() override;

    void setCustomStyle(int fontSize, const QString &textColor, int bgOpacity, const QColor &bgColor);
    void setOverlayVisible(bool visible);
    void resetPosition();

    // ── Hardware display mode flags ──
    void setShowCpu(bool show);
    void setShowRam(bool show);
    void setShowGpu(bool show);
    void setShowTemps(bool show);

    // ── New configuration setters ──
    void setRamDisplayMode(RamDisplayMode mode);
    void setGpuIndex(int index);

protected:
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
    int m_offsetX = 0;
    int m_offsetY = 0;
    bool m_isDragging = false;
    QPoint m_dragPos;

signals:
    // ─── Signals for Menu Actions (Connect these in your MainWindow) ───
    void showMainWindowRequested();
    void minimizeRequested();
    void exitRequested();
    void adapterChangeRequested(const QString &ipOrAuto);

    // ── Signal to save the new font size in QSettings ──
    void fontSizeChanged(int newSize);

public:
    void setAdapterList(const QList<AdapterInfo> &list) { m_adapterList = list; }

public slots:
    void updateSpeed(quint64 rxBps, quint64 txBps);
    void updateHardware(int cpu, int ram);
    void updateFullHardware(double cpuPct, double ramPct, double ramUsedMB, double ramTotalMB,
                            double gpuPct, double cpuTempC, double gpuTempC, double ramTempC);

protected:
    void paintEvent(QPaintEvent *e) override;
    void enterEvent(QEvent *e) override;
    void leaveEvent(QEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;

private slots:
    void enforceTopMostAndPosition();

private:
    static QString fmtSpeed(quint64 bps);
    void rebuildLayout();

    // ─── Separated Icons and Values for perfect Grid alignment ───
    QLabel *m_iconTx = nullptr;
    QLabel *m_lblTx  = nullptr;
    QLabel *m_iconRx = nullptr;
    QLabel *m_lblRx  = nullptr;

    // ── Hardware display labels ──
    QLabel *m_iconCpu   = nullptr;
    QLabel *m_lblCpu    = nullptr;
    QLabel *m_iconRam   = nullptr;
    QLabel *m_lblRam    = nullptr;
    QLabel *m_iconGpu   = nullptr;
    QLabel *m_lblGpu    = nullptr;
    QLabel *m_iconTemp  = nullptr;
    QLabel *m_lblTemp   = nullptr;

    QGridLayout *m_gridLayout = nullptr;
    QTimer *m_timer  = nullptr;

    StatsPopup *m_popup = nullptr;

    quint64 m_sessionRx = 0;
    quint64 m_sessionTx = 0;
    quint64 m_currentRx = 0;
    quint64 m_currentTx = 0;
    int     m_lastCpu   = 0;
    int     m_lastRam   = 0;

    // ── Full hardware snapshot cache ──
    double  m_cpuPct     = 0.0;
    double  m_ramPct     = 0.0;
    double  m_ramUsedMB  = 0.0;
    double  m_ramTotalMB = 0.0;
    double  m_gpuPct     = 0.0;
    double  m_cpuTempC   = 0.0;
    double  m_gpuTempC   = 0.0;
    double  m_ramTempC   = 0.0;

    // ── Display flags (all OFF by default) ──
    bool m_showCpu   = false;
    bool m_showRam   = false;
    bool m_showGpu   = false;
    bool m_showTemps = false;

    RamDisplayMode m_ramDisplayMode = RamDisplayMode::TotalGB;
    int m_gpuIndex = -1;  // -1 = auto/all

    int     m_fontSize  = 12;
    QString m_textColor = "white";
    int     m_bgOpacity = 0;
    QColor  m_bgColor   = QColor(28, 33, 40);

    int m_lastX = -1;
    int m_lastY = -1;
    int m_numGroups = 1; // Number of active column groups (1 = net only)
    QList<AdapterInfo> m_adapterList;
};
