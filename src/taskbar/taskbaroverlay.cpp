/**
 * @file    taskbaroverlay.cpp
 * @brief   Transparent taskbar overlay with dynamic auto-resizing, tight spacing, and Drag & Drop.
 * @author  Ali Sakkaf
 */
#include "taskbaroverlay.h"

#ifndef _WIN32_WINNT
#  define _WIN32_WINNT 0x0601
#endif
#include <windows.h>

#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QApplication>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QMenu>
#include <QGraphicsDropShadowEffect>

// ============================================================================
// --- Professional Stats Popup Widget ----------------------------------------
// ============================================================================
class StatsPopup : public QWidget {
public:
    QLabel *valRx, *valTx, *valTotRx, *valTotTx, *valCpu, *valRam;
    QLabel *valGpu, *valCpuTemp, *valGpuTemp, *valRamDetail;

    StatsPopup() : QWidget(nullptr, Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint) {
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_ShowWithoutActivating);

        HWND hwPopup = reinterpret_cast<HWND>(winId());
        HWND hwTray = FindWindowW(L"Shell_TrayWnd", nullptr);
        if (hwTray) {
            SetWindowLongPtr(hwPopup, GWLP_HWNDPARENT, reinterpret_cast<LONG_PTR>(hwTray));
        }

        QFrame *bgFrame = new QFrame(this);
        bgFrame->setObjectName("bgFrame");

        bgFrame->setStyleSheet(
            "QFrame#bgFrame { background-color: #18181B; border: 1px solid #3F3F46; border-radius: 8px; }"
            "QLabel { color: #A1A1AA; font-family: 'Segoe UI'; font-size: 9pt; font-weight: 600; background: transparent; }"
            "QLabel#title { color: #E4E4E7; font-size: 10pt; font-weight: bold; }"
            "QLabel#valBlue { color: #60A5FA; font-weight: bold; }"
            "QLabel#valGreen { color: #34D399; font-weight: bold; }"
            "QLabel#valYellow { color: #FBBF24; font-weight: bold; }"
            "QLabel#valOrange { color: #FB923C; font-weight: bold; }"
            "QLabel#valRed { color: #F87171; font-weight: bold; }"
            );

        QVBoxLayout *windowLay = new QVBoxLayout(this);
        windowLay->setContentsMargins(0, 0, 0, 0);
        windowLay->addWidget(bgFrame);

        QVBoxLayout *mainLay = new QVBoxLayout(bgFrame);
        mainLay->setContentsMargins(16, 12, 16, 16);
        mainLay->setSpacing(10);

        QLabel *title = new QLabel("⚡ Live System Status");
        title->setObjectName("title");
        title->setAlignment(Qt::AlignCenter);
        mainLay->addWidget(title);

        QFrame *line = new QFrame();
        line->setFrameShape(QFrame::HLine);
        line->setStyleSheet("background-color: #3F3F46; max-height: 1px; border: none;");
        mainLay->addWidget(line);

        QGridLayout *grid = new QGridLayout();
        grid->setSpacing(8);
        grid->setHorizontalSpacing(25);

        auto createRow = [&](int row, const QString &icon1, const QString &lbl1, QLabel *&v1, const QString &color1,
                             const QString &icon2, const QString &lbl2, QLabel *&v2, const QString &color2) {

            grid->addWidget(new QLabel(icon1 + " " + lbl1), row, 0, Qt::AlignLeft | Qt::AlignVCenter);
            v1 = new QLabel("0"); v1->setObjectName(color1); v1->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            grid->addWidget(v1, row, 1);

            grid->addWidget(new QLabel(icon2 + " " + lbl2), row, 2, Qt::AlignLeft | Qt::AlignVCenter);
            v2 = new QLabel("0"); v2->setObjectName(color2); v2->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            grid->addWidget(v2, row, 3);
        };

        createRow(0, "↓", "Download:", valRx, "valBlue",       "↑", "Upload:", valTx, "valGreen");
        createRow(1, "📥", "Total DL:", valTotRx, "valBlue",    "📤", "Total UL:", valTotTx, "valGreen");
        createRow(2, "💻", "CPU Load:", valCpu, "valYellow",    "🧠", "RAM Load:", valRam, "valYellow");
        createRow(3, "🎮", "GPU Load:", valGpu, "valOrange",    "💾", "RAM Used:", valRamDetail, "valOrange");
        createRow(4, "🌡", "CPU Temp:", valCpuTemp, "valRed",   "🌡", "GPU Temp:", valGpuTemp, "valRed");

        mainLay->addLayout(grid);
    }

    static QString fmtBytes(quint64 b) {
        if (b < 1024) return QString("%1 B").arg(b);
        if (b < 1024*1024) return QString("%1 KB").arg(b/1024.0, 0, 'f', 1);
        if (b < 1024*1024*1024ULL) return QString("%1 MB").arg(b/(1024.0*1024), 0, 'f', 2);
        return QString("%1 GB").arg(b/(1024.0*1024*1024), 0, 'f', 2);
    }
};

// ============================================================================
// --- TaskbarOverlay Implementation ------------------------------------------
// ============================================================================

// Helper function to apply drop shadows for text visibility
void applyTextShadow(QWidget* widget) {
    auto *effect = new QGraphicsDropShadowEffect(widget);
    effect->setBlurRadius(4);
    effect->setOffset(1, 1);
    effect->setColor(QColor(0, 0, 0, 240));
    widget->setGraphicsEffect(effect);
}

TaskbarOverlay::TaskbarOverlay(QWidget *parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus)
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_NoSystemBackground,    true);
    setAttribute(Qt::WA_ShowWithoutActivating, true);

    HWND hwnd = reinterpret_cast<HWND>(winId());
    LONG ex   = GetWindowLong(hwnd, GWL_EXSTYLE);
    ex |= WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
    ex &= ~WS_EX_APPWINDOW;
    SetWindowLong(hwnd, GWL_EXSTYLE, ex);

    m_popup = new StatsPopup();
    m_popup->hide();

    // --- Initialize Labels ---
    m_iconTx = new QLabel("↑"); m_iconTx->setStyleSheet("color: #F59E0B;"); m_lblTx  = new QLabel("0 B/s");
    m_iconRx = new QLabel("↓"); m_iconRx->setStyleSheet("color: #3B82F6;"); m_lblRx  = new QLabel("0 B/s");
    m_iconCpu = new QLabel("C"); m_iconCpu->setStyleSheet("color: #FBBF24;"); m_lblCpu  = new QLabel("0%");
    m_iconRam = new QLabel("R"); m_iconRam->setStyleSheet("color: #34D399;"); m_lblRam  = new QLabel("0 GB");
    m_iconGpu = new QLabel("G"); m_iconGpu->setStyleSheet("color: #FB923C;"); m_lblGpu  = new QLabel("0%");
    m_iconTemp= new QLabel("T"); m_iconTemp->setStyleSheet("color: #F87171;"); m_lblTemp = new QLabel("--");

    // --- Apply Text Shadows ---
    applyTextShadow(m_iconTx); applyTextShadow(m_lblTx);
    applyTextShadow(m_iconRx); applyTextShadow(m_lblRx);
    applyTextShadow(m_iconCpu); applyTextShadow(m_lblCpu);
    applyTextShadow(m_iconRam); applyTextShadow(m_lblRam);
    applyTextShadow(m_iconGpu); applyTextShadow(m_lblGpu);
    applyTextShadow(m_iconTemp); applyTextShadow(m_lblTemp);

    // --- Smart Tight Alignment ---
    // Icons align right, Texts align left. This ensures they stick together exactly.
    m_iconTx->setAlignment(Qt::AlignRight | Qt::AlignVCenter); m_lblTx->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_iconRx->setAlignment(Qt::AlignRight | Qt::AlignVCenter); m_lblRx->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_iconCpu->setAlignment(Qt::AlignRight | Qt::AlignVCenter);m_lblCpu->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_iconRam->setAlignment(Qt::AlignRight | Qt::AlignVCenter);m_lblRam->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_iconGpu->setAlignment(Qt::AlignRight | Qt::AlignVCenter);m_lblGpu->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_iconTemp->setAlignment(Qt::AlignRight | Qt::AlignVCenter);m_lblTemp->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    m_gridLayout = new QGridLayout(this);
    setLayout(m_gridLayout);

    m_timer = new QTimer(this);
    m_timer->setInterval(500);
    connect(m_timer, &QTimer::timeout, this, &TaskbarOverlay::enforceTopMostAndPosition);
    m_timer->start();

    setCustomStyle(m_fontSize, m_textColor, m_bgOpacity, m_bgColor);
    enforceTopMostAndPosition();
}

TaskbarOverlay::~TaskbarOverlay() {
    delete m_popup;
}

// ============================================================================
// --- Fully Dynamic Auto-Resizing Layout -------------------------------------
// ============================================================================
void TaskbarOverlay::rebuildLayout()
{
    while (QLayoutItem *item = m_gridLayout->takeAt(0)) {
        delete item;
    }

    m_iconCpu->hide(); m_lblCpu->hide();
    m_iconRam->hide(); m_lblRam->hide();
    m_iconGpu->hide(); m_lblGpu->hide();
    m_iconTemp->hide(); m_lblTemp->hide();

    for (int c = 0; c < 12; c++) {
        m_gridLayout->setColumnStretch(c, 0);
        m_gridLayout->setColumnMinimumWidth(c, 0);
    }

    // 1. Margins and Spacing: Perfectly tight
    m_gridLayout->setContentsMargins(4, 0, 4, 0);
    m_gridLayout->setVerticalSpacing(0);
    // EXACTLY 1 Space gap (4 pixels) between the Icon and the Value
    m_gridLayout->setHorizontalSpacing(4);

    int col = 0;

    // Group 1: Network
    m_gridLayout->addWidget(m_iconTx, 0, col); m_gridLayout->addWidget(m_lblTx,  0, col + 1);
    m_gridLayout->addWidget(m_iconRx, 1, col); m_gridLayout->addWidget(m_lblRx,  1, col + 1);
    m_iconTx->show(); m_lblTx->show();
    m_iconRx->show(); m_lblRx->show();

    struct HwItem { QLabel *icon; QLabel *lbl; };
    QVector<HwItem> tops, bots;

    if (m_showCpu)  { HwItem h; h.icon = m_iconCpu;  h.lbl = m_lblCpu;  tops.append(h); }
    if (m_showRam)  { HwItem h; h.icon = m_iconRam;  h.lbl = m_lblRam;  bots.append(h); }
    if (m_showGpu)  { HwItem h; h.icon = m_iconGpu;  h.lbl = m_lblGpu;  tops.append(h); }
    if (m_showTemps){ HwItem h; h.icon = m_iconTemp; h.lbl = m_lblTemp; bots.append(h); }

    int maxPairs = qMax(tops.size(), bots.size());
    for (int i = 0; i < maxPairs; i++) {
        col += 2;

        // Gap between Groups (e.g., Network and Hardware)
        m_gridLayout->setColumnMinimumWidth(col, 8);
        col += 1;

        if (i < tops.size()) {
            m_gridLayout->addWidget(tops[i].icon, 0, col);
            m_gridLayout->addWidget(tops[i].lbl,  0, col + 1);
            tops[i].icon->show(); tops[i].lbl->show();
        }
        if (i < bots.size()) {
            m_gridLayout->addWidget(bots[i].icon, 1, col);
            m_gridLayout->addWidget(bots[i].lbl,  1, col + 1);
            bots[i].icon->show(); bots[i].lbl->show();
        }
    }

    // Allow the widget to shrink perfectly to fit its content
    m_gridLayout->invalidate();
    m_gridLayout->activate();
    adjustSize();

    m_lastX = -1;
    update();
}

void TaskbarOverlay::setShowCpu(bool show)  { if (m_showCpu != show)  { m_showCpu = show;  rebuildLayout(); enforceTopMostAndPosition(); } }
void TaskbarOverlay::setShowRam(bool show)  { if (m_showRam != show)  { m_showRam = show;  rebuildLayout(); enforceTopMostAndPosition(); } }
void TaskbarOverlay::setShowGpu(bool show)  { if (m_showGpu != show)  { m_showGpu = show;  rebuildLayout(); enforceTopMostAndPosition(); } }
void TaskbarOverlay::setShowTemps(bool show){ if (m_showTemps!= show) { m_showTemps= show; rebuildLayout(); enforceTopMostAndPosition(); } }

void TaskbarOverlay::setRamDisplayMode(RamDisplayMode mode)
{
    if (m_ramDisplayMode != mode) {
        m_ramDisplayMode = mode;
        // Re-render the RAM label with the new format
        if (m_showRam && m_ramTotalMB > 0) {
            switch (m_ramDisplayMode) {
            case RamDisplayMode::Percentage:
                m_lblRam->setText(QString("%1%").arg((int)m_ramPct));
                break;
            case RamDisplayMode::TotalMB:
                m_lblRam->setText(QString("%1/%2 MB").arg((int)m_ramUsedMB).arg((int)m_ramTotalMB));
                break;
            case RamDisplayMode::TotalGB:
            default:
                m_lblRam->setText(QString("%1/%2 GB").arg((int)(m_ramUsedMB / 1024.0 + 0.5)).arg((int)(m_ramTotalMB / 1024.0 + 0.5)));
                break;
            }
            adjustSize();
        }
    }
}

void TaskbarOverlay::setGpuIndex(int index)
{
    m_gpuIndex = index;
}

void TaskbarOverlay::setCustomStyle(int fontSize, const QString &textColor, int bgOpacity, const QColor &bgColor)
{
    m_fontSize = fontSize;
    m_textColor = textColor;
    m_bgOpacity = bgOpacity;
    m_bgColor = bgColor;

    QFont f("Segoe UI", m_fontSize);
    m_iconTx->setFont(f);  m_lblTx->setFont(f);
    m_iconRx->setFont(f);  m_lblRx->setFont(f);
    m_iconCpu->setFont(f); m_lblCpu->setFont(f);
    m_iconRam->setFont(f); m_lblRam->setFont(f);
    m_iconGpu->setFont(f); m_lblGpu->setFont(f);
    m_iconTemp->setFont(f);m_lblTemp->setFont(f);

    QString css = QString("color: %1; background-color: transparent; padding: 0; margin: 0;").arg(m_textColor);
    m_lblTx->setStyleSheet(css);
    m_lblRx->setStyleSheet(css);
    m_lblCpu->setStyleSheet(css);
    m_lblRam->setStyleSheet(css);
    m_lblGpu->setStyleSheet(css);
    m_lblTemp->setStyleSheet(css);

    rebuildLayout();
    update();
    enforceTopMostAndPosition();
}

void TaskbarOverlay::setOverlayVisible(bool visible) {
    setVisible(visible);
    if (visible) { m_lastX = -1; enforceTopMostAndPosition(); }
    else { m_popup->hide(); }
}

void TaskbarOverlay::enforceTopMostAndPosition()
{
    if (!isVisible() || m_isDragging) return;

    HWND hwTray = FindWindowW(L"Shell_TrayWnd", nullptr);
    if (!hwTray || !IsWindow(hwTray)) {
        setGeometry(-1000, -1000, width(), height());
        return;
    }

    HWND hwSelf = reinterpret_cast<HWND>(winId());
    HWND currentParent = reinterpret_cast<HWND>(GetWindowLongPtr(hwSelf, GWLP_HWNDPARENT));
    if (currentParent != hwTray) {
        SetWindowLongPtr(hwSelf, GWLP_HWNDPARENT, reinterpret_cast<LONG_PTR>(hwTray));
        if (m_popup) {
            SetWindowLongPtr(reinterpret_cast<HWND>(m_popup->winId()), GWLP_HWNDPARENT, reinterpret_cast<LONG_PTR>(hwTray));
        }
    }

    HWND hwNotify = FindWindowExW(hwTray, nullptr, L"TrayNotifyWnd", nullptr);
    if (!hwNotify) hwNotify = hwTray;

    RECT rcNotify = {}; GetWindowRect(hwNotify, &rcNotify);
    RECT rcTray   = {}; GetWindowRect(hwTray, &rcTray);

    const int barW = rcTray.right  - rcTray.left;
    const int barH = rcTray.bottom - rcTray.top;
    const bool isHorizontal = (barW > barH);

    if (isHorizontal && barH < 10) {
        setGeometry(-1000, -1000, width(), height());
        if (m_popup->isVisible()) m_popup->hide();
        return;
    }

    int targetH = isHorizontal ? (rcNotify.bottom - rcNotify.top) : barH;
    if (targetH > 4) {
        setFixedHeight(targetH - 4);
    }

    const int ovW = width();
    const int ovH = height();
    int ovX, ovY;

    if (isHorizontal) {
        ovX = rcNotify.left - ovW - 8 + m_offsetX;
        ovY = rcNotify.top + (barH - ovH) / 2 + m_offsetY;
    } else {
        ovX = rcTray.left + (barW - ovW) / 2 + m_offsetX;
        ovY = rcNotify.top  - ovH - 8 + m_offsetY;
    }

    UINT flags = SWP_NOACTIVATE | SWP_SHOWWINDOW;

    if (ovX == m_lastX && ovY == m_lastY) {
        flags |= SWP_NOMOVE | SWP_NOSIZE;
    } else {
        m_lastX = ovX; m_lastY = ovY;
        setGeometry(ovX, ovY, ovW, ovH);
    }

    SetWindowPos(hwSelf, HWND_TOPMOST, ovX, ovY, ovW, ovH, flags);
}

// ============================================================================
// --- Drag & Drop with Fixed Safe Menu ---------------------------------------
void TaskbarOverlay::resetPosition()
{
    m_offsetX = 0;
    m_offsetY = 0;
    m_lastX = -1;
    enforceTopMostAndPosition();
}

// ============================================================================
void TaskbarOverlay::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_dragPos = e->globalPos() - frameGeometry().topLeft();
        e->accept();
    }
    else if (e->button() == Qt::RightButton) {
        QMenu menu(nullptr);
        menu.setWindowFlags(menu.windowFlags() | Qt::WindowStaysOnTopHint);

        QMenu *netMenu = menu.addMenu("🌐 Select Network Adapter");
        QAction *actAuto = netMenu->addAction("⚡ Smart Auto-Detect (Auto)");
        connect(actAuto, &QAction::triggered, this, [this]() {
            emit adapterChangeRequested("AUTO");
        });
        QAction *actAll = netMenu->addAction("🌐 All Network Adapters (Select All)");
        connect(actAll, &QAction::triggered, this, [this]() {
            emit adapterChangeRequested("ALL");
        });
        netMenu->addSeparator();

        for (const auto &ai : m_adapterList) {
            QAction *actAdapter = netMenu->addAction(ai.description);
            connect(actAdapter, &QAction::triggered, this, [this, ip = ai.ip]() {
                emit adapterChangeRequested(ip);
            });
        }
        menu.addSeparator();

        QAction *actZoomIn  = menu.addAction("➕ Increase Size");
        QAction *actZoomOut = menu.addAction("➖ Decrease Size");
        menu.addSeparator();

        QAction *actResetPos = menu.addAction("🔄 Reset Position");
        menu.addSeparator();

        QAction *actShow = menu.addAction("🖥️ Show App");
        QAction *actMinimize = menu.addAction("🔽 Minimize App");
        menu.addSeparator();

        QAction *actExit = menu.addAction("❌ Exit NetGuard");

        connect(actZoomIn, &QAction::triggered, this, [this]() {
            if (m_fontSize < 24) {
                setCustomStyle(m_fontSize + 1, m_textColor, m_bgOpacity, m_bgColor);
                emit fontSizeChanged(m_fontSize);
            }
        });

        connect(actZoomOut, &QAction::triggered, this, [this]() {
            if (m_fontSize > 7) {
                setCustomStyle(m_fontSize - 1, m_textColor, m_bgOpacity, m_bgColor);
                emit fontSizeChanged(m_fontSize);
            }
        });

        connect(actResetPos, &QAction::triggered, this, [this]() {
            m_offsetX = 0; m_offsetY = 0; m_lastX = -1;
            enforceTopMostAndPosition();
        });

        connect(actShow, &QAction::triggered, this, &TaskbarOverlay::showMainWindowRequested);
        connect(actMinimize, &QAction::triggered, this, &TaskbarOverlay::minimizeRequested);
        connect(actExit, &QAction::triggered, this, &TaskbarOverlay::exitRequested);

        menu.adjustSize();
        int menuHeight = menu.sizeHint().height();

        QPoint safePos = this->mapToGlobal(QPoint(0, 0));
        safePos.setY(safePos.y() - menuHeight - 5);

        menu.exec(safePos);
        enforceTopMostAndPosition();
    }
}

void TaskbarOverlay::mouseMoveEvent(QMouseEvent *e)
{
    if (m_isDragging && (e->buttons() & Qt::LeftButton)) {
        move(e->globalPos() - m_dragPos);
        e->accept();
    }
}

void TaskbarOverlay::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && m_isDragging) {
        m_isDragging = false;

        HWND hwTray = FindWindowW(L"Shell_TrayWnd", nullptr);
        HWND hwNotify = FindWindowExW(hwTray, nullptr, L"TrayNotifyWnd", nullptr);
        if (!hwNotify) hwNotify = hwTray;

        RECT rcNotify = {}; GetWindowRect(hwNotify, &rcNotify);
        RECT rcTray   = {}; GetWindowRect(hwTray, &rcTray);

        const int barW = rcTray.right  - rcTray.left;
        const int barH = rcTray.bottom - rcTray.top;
        const bool isHorizontal = (barW > barH);

        int baseOvX, baseOvY;
        if (isHorizontal) {
            baseOvX = rcNotify.left - width() - 8;
            baseOvY = rcNotify.top + (barH - height()) / 2;
        } else {
            baseOvX = rcTray.left + (barW - width()) / 2;
            baseOvY = rcNotify.top  - height() - 8;
        }

        m_offsetX = this->x() - baseOvX;
        m_offsetY = this->y() - baseOvY;

        m_lastX = -1;
        enforceTopMostAndPosition();
        e->accept();
    }
}

void TaskbarOverlay::enterEvent(QEvent *)
{
    if(m_isDragging) return;

    m_popup->valRx->setText(fmtSpeed(m_currentRx));
    m_popup->valTx->setText(fmtSpeed(m_currentTx));
    m_popup->valTotRx->setText(StatsPopup::fmtBytes(m_sessionRx));
    m_popup->valTotTx->setText(StatsPopup::fmtBytes(m_sessionTx));
    m_popup->valCpu->setText(QString::number((int)m_cpuPct) + "%");
    m_popup->valRam->setText(QString::number((int)m_ramPct) + "%");
    m_popup->valGpu->setText(QString::number((int)m_gpuPct) + "%");
    m_popup->valRamDetail->setText(QString("%1 / %2 GB").arg(m_ramUsedMB/1024.0, 0, 'f', 1).arg(m_ramTotalMB/1024.0, 0, 'f', 0));
    m_popup->valCpuTemp->setText(m_cpuTempC > 0 ? QString::number((int)m_cpuTempC) + "°C" : "N/A");
    m_popup->valGpuTemp->setText(m_gpuTempC > 0 ? QString::number((int)m_gpuTempC) + "°C" : "N/A");

    m_popup->adjustSize();

    QPoint globalPos = mapToGlobal(QPoint(0, 0));
    int popX = globalPos.x() - (m_popup->width() - width()) / 2;
    int popY;

    if (globalPos.y() > m_popup->height()) {
        popY = globalPos.y() - m_popup->height() - 10;
    } else {
        popY = globalPos.y() + height() + 10;
    }

    m_popup->move(popX, popY);
    m_popup->show();
}

void TaskbarOverlay::leaveEvent(QEvent *) {
    m_popup->hide();
}

void TaskbarOverlay::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QColor finalBgColor = m_bgColor;
    if (m_bgColor == Qt::transparent || m_bgOpacity == 0) {
        finalBgColor = QColor(0, 0, 0, 1);
    } else {
        finalBgColor.setAlpha(m_bgOpacity);
    }

    p.setBrush(finalBgColor);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(rect(), 4, 4);
}

// ============================================================================
// --- Update Methods (Triggers Smart Auto-Resize) ----------------------------
// ============================================================================
void TaskbarOverlay::updateSpeed(quint64 rxBps, quint64 txBps)
{
    m_currentRx = rxBps; m_currentTx = txBps;
    m_sessionRx += rxBps; m_sessionTx += txBps;

    m_lblRx->setText(fmtSpeed(rxBps));
    m_lblTx->setText(fmtSpeed(txBps));

    // Force the widget to perfectly hug the new text size
    adjustSize();
}

void TaskbarOverlay::updateHardware(int cpu, int ram)
{
    m_lastCpu = cpu; m_lastRam = ram;
}

void TaskbarOverlay::updateFullHardware(double cpuPct, double ramPct, double ramUsedMB, double ramTotalMB,
                                        double gpuPct, double cpuTempC, double gpuTempC, double ramTempC)
{
    m_cpuPct = cpuPct; m_ramPct = ramPct; m_ramUsedMB = ramUsedMB; m_ramTotalMB = ramTotalMB;
    m_gpuPct = gpuPct; m_cpuTempC = cpuTempC; m_gpuTempC = gpuTempC; m_ramTempC = ramTempC;

    if (m_showCpu)  m_lblCpu->setText(QString("%1%").arg((int)cpuPct));
    if (m_showRam) {
        switch (m_ramDisplayMode) {
        case RamDisplayMode::Percentage:
            m_lblRam->setText(QString("%1%").arg((int)ramPct));
            break;
        case RamDisplayMode::TotalMB:
            m_lblRam->setText(QString("%1/%2 MB").arg((int)ramUsedMB).arg((int)ramTotalMB));
            break;
        case RamDisplayMode::TotalGB:
        default: {
            int ramGB = (int)(ramUsedMB / 1024.0 + 0.5);
            int totalGB = (int)(ramTotalMB / 1024.0 + 0.5);
            m_lblRam->setText(QString("%1/%2 GB").arg(ramGB).arg(totalGB));
            break;
        }
        }
    }
    if (m_showGpu)  m_lblGpu->setText(QString("%1%").arg((int)gpuPct));
    if (m_showTemps) {
        QString tempStr;
        if (cpuTempC > 0 && gpuTempC > 0) tempStr = QString("%1/%2°").arg((int)cpuTempC).arg((int)gpuTempC);
        else if (cpuTempC > 0) tempStr = QString("%1°").arg((int)cpuTempC);
        else if (gpuTempC > 0) tempStr = QString("%1°").arg((int)gpuTempC);
        else tempStr = "--";
        m_lblTemp->setText(tempStr);
    }

    // Force the widget to perfectly hug the new text size
    adjustSize();
}

QString TaskbarOverlay::fmtSpeed(quint64 bps) {
    if (bps == 0)            return "0 B/s";
    if (bps < 1024)          return QString("%1 B/s").arg(bps);
    if (bps < 1024*1024)     return QString("%1 KB/s").arg(bps / 1024.0, 0, 'f', 1);
    return                   QString("%1 MB/s").arg(bps / (1024.0*1024), 0, 'f', 2);
}
