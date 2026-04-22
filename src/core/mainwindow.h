/**
 * @file    mainwindow.h
 * @author  Ali Sakkaf
 * @brief   Main Window definition with intelligent History Management and JSON persistence.
 */
#pragma once

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QLabel>
#include <QSettings>
#include <QMap>
#include <QHash>
#include <QIcon>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include "apptheme.h"
#include "hardware/hardwaremonitor.h"

namespace Ui { class MainWindow; }

class NetworkWidget;
class FirewallWidget;
class HardwareWidget;
class TaskbarOverlay;

// Data structure to hold daily traffic per application
struct DailyAppStat {
    quint64 rxBytes = 0;
    quint64 txBytes = 0;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent  (QCloseEvent *event) override;
    void changeEvent (QEvent *event)      override;

private slots:
    void onToggleTheme();
    void onMinimizeToTray();
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void onNetworkSpeed(quint64 rxBps, quint64 txBps);
    void onHardwareSnapshot(const HardwareSnapshot &snap);

    void saveSettings();
    void applySettingsToOverlay();
    void applyRunAtStartup(bool enable);

    void onExportHistory();
    void onClearHistory();
    void onHistoryFilterChanged(int index);

    void recordLiveTraffic();
    void refreshHistoryUI();
    void saveHistoryToJson();

private:
    void setupWidgets();
    void setupTray();
    void animateLiveBadge();
    void loadSettings();
    void initHistoryManager();
    void loadHistoryFromJson();

    static QString fmtSpeed(quint64 bps);
    static QString fmtBytes(quint64 bytes);

    Ui::MainWindow    *ui          = nullptr;

    NetworkWidget     *m_netWidget = nullptr;
    FirewallWidget    *m_fwWidget  = nullptr;
    HardwareWidget    *m_hwWidget  = nullptr;
    TaskbarOverlay    *m_overlay   = nullptr;

    QSystemTrayIcon   *m_tray      = nullptr;
    QMenu             *m_trayMenu  = nullptr;

    AppTheme::Mode     m_theme     = AppTheme::Dark;

    QTimer            *m_badgeTimer= nullptr;
    bool               m_badgeLit  = true;

    QTimer            *m_trafficMonitorTimer = nullptr;
    QTimer            *m_historyUiTimer      = nullptr;
    QString            m_historyFilePath;
    int                m_saveCounter         = 0;

    QMap<QString, QMap<QString, DailyAppStat>> m_historyData;
    QHash<QString, QIcon> m_iconCache;
};
