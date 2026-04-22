/**
 * @file    taskbaroverlay.cpp
 * @brief   Transparent taskbar overlay with absolute Z-order immunity and Smart Menus.
 * @author  Ali Sakkaf
 */
#include "taskbaroverlay.h"

#ifndef _WIN32_WINNT
#  define _WIN32_WINNT 0x0601
#endif
#include <windows.h>

#include <QPainter>
#include <QFont>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QApplication>
#include <QPaintEvent>
#include <QFrame>
#include <QMenu>

// ============================================================================
// ─── Professional Stats Popup Widget ────────────────────────────────────────
// ============================================================================
class StatsPopup : public QWidget {
public:
    QLabel *valRx, *valTx, *valTotRx, *valTotTx, *valCpu, *valRam;

    StatsPopup() : QWidget(nullptr, Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint) {
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_ShowWithoutActivating); // ── FIX: Prevent focus stealing completely

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
// ─── TaskbarOverlay Implementation ──────────────────────────────────────────
// ============================================================================

TaskbarOverlay::TaskbarOverlay(QWidget *parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus)
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_NoSystemBackground,    true);
    setAttribute(Qt::WA_ShowWithoutActivating, true); // ── FIX: Prevent the window from ever gaining focus

    HWND hwnd = reinterpret_cast<HWND>(winId());
    LONG ex   = GetWindowLong(hwnd, GWL_EXSTYLE);
    ex |= WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
    ex &= ~WS_EX_APPWINDOW;

    SetWindowLong(hwnd, GWL_EXSTYLE, ex);

    m_popup = new StatsPopup();
    m_popup->hide();

    setFixedWidth(80);

    QGridLayout *lay = new QGridLayout(this);
    lay->setContentsMargins(4, 0, 4, 0);
    lay->setSpacing(4);
    lay->setVerticalSpacing(1);

    m_iconTx = new QLabel("↑:");
    m_iconTx->setStyleSheet("color: #F59E0B; background: transparent;");
    m_lblTx  = new QLabel("0 B/s");

    m_iconRx = new QLabel("↓:");
    m_iconRx->setStyleSheet("color: #3B82F6; background: transparent;");
    m_lblRx  = new QLabel("0 B/s");

    // ── Magic setup for alignment: Arrow to the right, number to the left ──
    m_iconTx->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_lblTx->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_iconRx->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_lblRx->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    lay->addWidget(m_iconTx, 0, 0);
    lay->addWidget(m_lblTx,  0, 1);
    lay->addWidget(m_iconRx, 1, 0);
    lay->addWidget(m_lblRx,  1, 1);

    setLayout(lay);

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

void TaskbarOverlay::setCustomStyle(int fontSize, const QString &textColor, int bgOpacity, const QColor &bgColor)
{
    m_fontSize = fontSize;
    m_textColor = textColor;
    m_bgOpacity = bgOpacity;
    m_bgColor = bgColor;

    QFont f("Segoe UI", m_fontSize);
    m_iconTx->setFont(f);
    m_lblTx->setFont(f);
    m_iconRx->setFont(f);
    m_lblRx->setFont(f);

    QString css = QString("color: %1; background-color: transparent; padding: 0; margin: 0;").arg(m_textColor);
    m_lblTx->setStyleSheet(css);
    m_lblRx->setStyleSheet(css);

    int newWidth = 75 + (fontSize > 8 ? (fontSize - 8) * 8 : 0);
    setFixedWidth(newWidth);

    m_lastX = -1; // Force reposition
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
    if (!isVisible()) return;

    HWND hwTray = FindWindowW(L"Shell_TrayWnd", nullptr);
    if (!hwTray) {
        // ── FIX: If Taskbar is not found (Explorer Crash), hide widget temporarily to prevent random appearance
        setGeometry(-1000, -1000, width(), height());
        return;
    }

    HWND hwSelf = reinterpret_cast<HWND>(winId());

    // ── FIX: Recover from Windows Explorer restart (Explorer Restart Survival) ──
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

    // ── FIX: Handle Auto-hide Taskbar ──
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
        ovX = rcNotify.left  - ovW - 8;
        ovY = rcNotify.top + 2;
    } else {
        ovX = rcTray.left + (barW - ovW) / 2;
        ovY = rcNotify.top  - ovH - 8;
    }

    UINT flags = SWP_NOACTIVATE | SWP_SHOWWINDOW;

    // ── FIX: Update position and Z-Order professionally without high resource usage
    if (ovX == m_lastX && ovY == m_lastY) {
        flags |= SWP_NOMOVE | SWP_NOSIZE;
    } else {
        m_lastX = ovX; m_lastY = ovY;
        setGeometry(ovX, ovY, ovW, ovH);
    }

    // ── FIX: Force strictly TopMost permanently (even if another app tries to cover it)
    SetWindowPos(hwSelf, HWND_TOPMOST, ovX, ovY, ovW, ovH, flags);
}

void TaskbarOverlay::updateSpeed(quint64 rxBps, quint64 txBps)
{
    m_currentRx = rxBps;
    m_currentTx = txBps;

    m_sessionRx += rxBps;
    m_sessionTx += txBps;

    m_lblRx->setText(fmtSpeed(rxBps));
    m_lblTx->setText(fmtSpeed(txBps));

    if (m_popup->isVisible()) {
        m_popup->valRx->setText(fmtSpeed(rxBps));
        m_popup->valTx->setText(fmtSpeed(txBps));
        m_popup->valTotRx->setText(StatsPopup::fmtBytes(m_sessionRx));
        m_popup->valTotTx->setText(StatsPopup::fmtBytes(m_sessionTx));
    }
}

void TaskbarOverlay::updateHardware(int cpu, int ram)
{
    m_lastCpu = cpu;
    m_lastRam = ram;
    if (m_popup->isVisible()) {
        m_popup->valCpu->setText(QString::number(cpu) + "%");
        m_popup->valRam->setText(QString::number(ram) + "%");
    }
}

// ─── Smart Context Menu for Left & Right Clicks ──────────────────────────────
void TaskbarOverlay::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::RightButton /*|| e->button() == Qt::LeftButton*/) {
        QMenu menu(nullptr);
        menu.setWindowFlags(menu.windowFlags() | Qt::WindowStaysOnTopHint);

        // menu.setStyleSheet(
        //     "QMenu { background-color: #18181B; color: #E4E4E7; border: 1px solid #3F3F46; border-radius: 6px; padding: 4px; }"
        //     "QMenu::item { padding: 8px 26px; border-radius: 4px; font-weight: bold; font-family: 'Segoe UI'; font-size: 9pt; }"
        //     "QMenu::item:selected { background-color: #27272A; color: #60A5FA; }"
        //     "QMenu::separator { height: 1px; background-color: #3F3F46; margin: 4px 8px; }"
        //     );

        // ── 1. Smart Resizing Controls ──
        QAction *actZoomIn  = menu.addAction("➕ Increase Size");
        QAction *actZoomOut = menu.addAction("➖ Decrease Size");
        menu.addSeparator();

        // ── 2. Window Visibility Controls ──
        QAction *actShow = menu.addAction("🖥️ Show App");
        QAction *actMinimize = menu.addAction("🔽 Minimize App");
        menu.addSeparator();

        // ── 3. Application Exit ──
        QAction *actExit = menu.addAction("❌ Exit NetGuard");

        // ── 4. Connect Logic ──
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

        connect(actShow, &QAction::triggered, this, &TaskbarOverlay::showMainWindowRequested);
        connect(actMinimize, &QAction::triggered, this, &TaskbarOverlay::minimizeRequested);
        connect(actExit, &QAction::triggered, this, &TaskbarOverlay::exitRequested);

        menu.adjustSize();
        int menuHeight = menu.sizeHint().height();
        QPoint pos = e->globalPos();

        if (pos.y() > menuHeight) {
            pos.setY(pos.y() - menuHeight - 5);
        } else {
            pos.setY(pos.y() + 5);
        }

        if (m_timer && m_timer->isActive()) {
            m_timer->stop();
        }

        menu.exec(pos);

        if (m_timer) {
            m_timer->start();
        }

        enforceTopMostAndPosition();
    }
}
void TaskbarOverlay::enterEvent(QEvent *)
{
    m_popup->valRx->setText(fmtSpeed(m_currentRx));
    m_popup->valTx->setText(fmtSpeed(m_currentTx));
    m_popup->valTotRx->setText(StatsPopup::fmtBytes(m_sessionRx));
    m_popup->valTotTx->setText(StatsPopup::fmtBytes(m_sessionTx));
    m_popup->valCpu->setText(QString::number(m_lastCpu) + "%");
    m_popup->valRam->setText(QString::number(m_lastRam) + "%");

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

QString TaskbarOverlay::fmtSpeed(quint64 bps) {
    if (bps == 0)            return "0 B/s";
    if (bps < 1024)          return QString("%1 B/s").arg(bps);
    if (bps < 1024*1024)     return QString("%1 KB/s").arg(bps / 1024.0, 0, 'f', 1);
    return                   QString("%1 MB/s").arg(bps / (1024.0*1024), 0, 'f', 2);
}
