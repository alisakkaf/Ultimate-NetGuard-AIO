/**
 * @file    systemtrayicon.h
 * @author  Ali Sakkaf
 */
#pragma once
#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QTimer>

class SystemTrayIcon : public QObject
{
    Q_OBJECT
public:
    explicit SystemTrayIcon(QObject *parent=nullptr);
    void setSpeed(quint64 rxBps, quint64 txBps);
    void show()  { m_tray->show(); }
    QSystemTrayIcon *tray() const { return m_tray; }

signals:
    void showRequested();
    void quitRequested();

private:
    QSystemTrayIcon *m_tray;
    QMenu           *m_menu;
    QIcon buildIcon(quint64 rx, quint64 tx);
};
