/**
 * @file    systemtrayicon.cpp
 * @brief   System tray icon using the real app icon with dynamic activity indicator.
 * @author  Ali Sakkaf
 */
#include "systemtrayicon.h"
#include <QStyle>
#include <QApplication>
#include <QPainter>
#include <QPixmap>
#include <QMenu>
#include <QAction>

SystemTrayIcon::SystemTrayIcon(QObject *parent)
    : QObject(parent)
{
    m_menu = new QMenu();
    m_menu->setObjectName("trayMenu");

    // English menu actions
    QAction *actShow  = m_menu->addAction("Show NetGuard");
    m_menu->addSeparator();
    QAction *actQuit  = m_menu->addAction("Quit NetGuard");

    connect(actShow, &QAction::triggered, this, &SystemTrayIcon::showRequested);
    connect(actQuit, &QAction::triggered, this, &SystemTrayIcon::quitRequested);

    // Build the initial icon (0 traffic) and create the tray icon
    m_tray = new QSystemTrayIcon(buildIcon(0, 0), this);
    m_tray->setContextMenu(m_menu);

    // Initial English tooltip
    m_tray->setToolTip("Ultimate NetGuard AIO [Idle]");
}

void SystemTrayIcon::setSpeed(quint64 rxBps, quint64 txBps)
{
    // Update the tray icon with the dynamic activity dot.
    m_tray->setIcon(buildIcon(rxBps, txBps));

    // Simple English formatting function for bytes.
    auto fmt = [](quint64 b) -> QString {
        if (b < 1024)      return QString("%1 B/s").arg(b);
        if (b < 1024*1024) return QString("%1 KB/s").arg(b/1024);
        return QString("%1 MB/s").arg(b/(1024*1024));
    };

    // Update the English tooltip with formatted speeds.
    // Using a clean, professional format.
    m_tray->setToolTip(QString("NetGuard Status:\n"
                               "Download: %1\n"
                               "Upload:   %2")
                           .arg(fmt(rxBps), fmt(txBps)));
}

QIcon SystemTrayIcon::buildIcon(quint64 rxBps, quint64 txBps)
{
    // --- LOAD APP ICON DIRECTLY FROM RESOURCES ---
    // The previous code used QApplication::windowIcon(), which can be null.
    // We now load it directly from the resources for guaranteed results.
    // We use the PNG version for better inline handling within Qt.
    QIcon appIcon(":/icons/Ultimate_NetGuard_AIO.png");

    // Fallback: If PNG fails, try the ICO version
    if (appIcon.isNull()) {
        appIcon = QIcon(":/icons/app.ico");
    }

    // --- CONVERT TO PIXMAP AND PREPARE FOR PAINTING ---
    // Convert to a 16x16 pixmap, which is a standard size for tray icons.
    QPixmap pm = appIcon.pixmap(16, 16);

    // Last-resort fallback: If both fail, create a transparent pixmap to prevent crash.
    if (pm.isNull()) {
        pm = QPixmap(16, 16);
        pm.fill(Qt::transparent);
    }

    // --- PAINT THE DYNAMIC ACTIVITY INDICATOR DOT ---
    // Re-enabled the indicator dot drawing.
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    // Color: Green if there is any network traffic (rx or tx), Gray if not.
    QColor dotColor = (rxBps > 0 || txBps > 0) ? QColor("#34D399") : QColor("#71717A");

    p.setBrush(dotColor);
    // We add a darker pen for outline to make it pop against the icon colors.
    p.setPen(QPen(QColor("#18181B"), 1.0));

    // Draw the indicator dot in the bottom-right corner.
    p.drawEllipse(10, 10, 5, 5);
    p.end();

    // Re-create the QIcon from the newly painted pixmap.
    return QIcon(pm);
}
