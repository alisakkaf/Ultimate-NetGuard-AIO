/**
 * @file    taskbaroverlay.h
 * @brief   Frameless overlay widget that attaches precisely to the Taskbar.
 * Includes a professional Hover Stats Popup, Smart Resizing, & Right-Click Menu.
 * @author  Ali Sakkaf
 */
#pragma once

#include <QWidget>
#include <QTimer>
#include <QLabel>
#include <QColor>
#include <QMouseEvent>

class StatsPopup;

class TaskbarOverlay : public QWidget
{
    Q_OBJECT
public:
    explicit TaskbarOverlay(QWidget *parent = nullptr);
    ~TaskbarOverlay() override;

    void setCustomStyle(int fontSize, const QString &textColor, int bgOpacity, const QColor &bgColor);
    void setOverlayVisible(bool visible);

signals:
    // ─── Signals for Menu Actions (Connect these in your MainWindow) ───
    void showMainWindowRequested();
    void minimizeRequested();
    void exitRequested();

    // ── Signal to save the new font size in QSettings ──
    void fontSizeChanged(int newSize);

public slots:
    void updateSpeed(quint64 rxBps, quint64 txBps);
    void updateHardware(int cpu, int ram);

protected:
    void paintEvent(QPaintEvent *e) override;
    void enterEvent(QEvent *e) override;
    void leaveEvent(QEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;

private slots:
    void enforceTopMostAndPosition();

private:
    static QString fmtSpeed(quint64 bps);

    // ─── Separated Icons and Values for perfect Grid alignment ───
    QLabel *m_iconTx = nullptr;
    QLabel *m_lblTx  = nullptr;
    QLabel *m_iconRx = nullptr;
    QLabel *m_lblRx  = nullptr;
    QTimer *m_timer  = nullptr;

    StatsPopup *m_popup = nullptr;

    quint64 m_sessionRx = 0;
    quint64 m_sessionTx = 0;
    quint64 m_currentRx = 0;
    quint64 m_currentTx = 0;
    int     m_lastCpu   = 0;
    int     m_lastRam   = 0;

    int     m_fontSize  = 12;
    QString m_textColor = "white";
    int     m_bgOpacity = 0;
    QColor  m_bgColor   = QColor(28, 33, 40);

    int m_lastX = -1;
    int m_lastY = -1;
};
