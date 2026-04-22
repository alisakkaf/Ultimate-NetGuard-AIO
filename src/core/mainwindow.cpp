/*
 =============================================================================
    Copyright (c) 2026  AliSakkaf  |  All Rights Reserved
 =============================================================================
*/
#include "mainwindow.h"
#include "version.h"
#include "qdebug.h"
#include "ui_mainwindow.h"
#include "stylemanager.h"
#include "apptheme.h"
#include "updatechecker.h"
#include "appinstaller.h"
#include "network/networkwidget.h"
#include "firewall/firewallwidget.h"
#include "hardware/hardwarewidget.h"
#include "taskbar/taskbaroverlay.h"

#include <QCloseEvent>
#include <QTimer>
#include <QStyle>
#include <QScreen>
#include <QGuiApplication>
#include <QApplication>
#include <QDir>
#include <QTreeView>
#include <QAbstractItemModel>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDateTime>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QTableWidgetItem>

// ============================================================================
// Initialization & Core Setup
// ============================================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QString Title = APP_NAME;
    Title.append(" Version ");
    Title.append(APP_VERSION_STR);
    setWindowTitle(Title);
    ui->versionLabel->setText(APP_VERSION_DISPLAY);

    if (QScreen *scr = QGuiApplication::primaryScreen())
        move(scr->availableGeometry().center() - rect().center());

    setupWidgets();
    setupTray();

    loadSettings();
    initHistoryManager();

    // ui->mainTabs->setCurrentIndex(0);


    m_badgeTimer = new QTimer(this);
    m_badgeTimer->setInterval(900);
    connect(m_badgeTimer, &QTimer::timeout, this, &MainWindow::animateLiveBadge);
    m_badgeTimer->start();

    connect(ui->btnThemeToggle, &QPushButton::clicked, this, &MainWindow::onToggleTheme);
    connect(ui->btnMinimizeToTray, &QPushButton::clicked, this, &MainWindow::onMinimizeToTray);

    connect(ui->chkRunAtStartup, &QCheckBox::toggled, this, [this](bool checked){
        applyRunAtStartup(checked);
        saveSettings();
    });
    connect(ui->chkStartMinimized, &QCheckBox::toggled, this, &MainWindow::saveSettings);

    connect(ui->chkEnableOverlay, &QCheckBox::toggled, this, &MainWindow::applySettingsToOverlay);
    connect(ui->spinOverlayFontSize, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::applySettingsToOverlay);
    connect(ui->spinOverlayOpacity, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::applySettingsToOverlay);
    connect(ui->cmbOverlayTextColor, &QComboBox::currentTextChanged, this, &MainWindow::applySettingsToOverlay);
    connect(ui->cmbOverlayBgColor, &QComboBox::currentTextChanged, this, &MainWindow::applySettingsToOverlay);

    m_netWidget->autoStart();

    // ── Silent background update check (2 seconds after launch) ──
    QTimer::singleShot(2000, this, [this](){
        auto *updater = new UpdateChecker(this, this);
        updater->checkNow();
    });

}

MainWindow::~MainWindow()
{
    saveHistoryToJson(); // Ensure data is saved before exiting
    delete m_overlay;
    delete ui;
}

// ============================================================================
// History Engine (Smart Data Extraction & Persistence)
// ============================================================================

// ============================================================================
// History Engine (Smart Data Extraction & Persistence with Tree Logic)
// ============================================================================

void MainWindow::initHistoryManager()
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appDataPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    m_historyFilePath = dir.absoluteFilePath("NetGuard_History.json");

    loadHistoryFromJson();

    // Setup History Tree UI
    ui->treeHistory->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->treeHistory->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->treeHistory->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->treeHistory->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    connect(ui->btnExportHistory, &QPushButton::clicked, this, &MainWindow::onExportHistory);
    connect(ui->btnClearHistory, &QPushButton::clicked, this, &MainWindow::onClearHistory);
    connect(ui->cmbHistoryRange, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onHistoryFilterChanged);

    // Set Default Filter to "All Time" (Index 3)
    ui->cmbHistoryRange->setCurrentIndex(3);

    m_trafficMonitorTimer = new QTimer(this);
    m_trafficMonitorTimer->setInterval(1000);
    connect(m_trafficMonitorTimer, &QTimer::timeout, this, &MainWindow::recordLiveTraffic);
    m_trafficMonitorTimer->start();

    m_historyUiTimer = new QTimer(this);
    m_historyUiTimer->setInterval(2000);
    connect(m_historyUiTimer, &QTimer::timeout, this, [this](){
        if (ui->mainTabs->currentWidget() == ui->tabHistory) {
            refreshHistoryUI();
        }
    });
    m_historyUiTimer->start();

    refreshHistoryUI();
}

void MainWindow::recordLiveTraffic()
{
    QTreeView *tree = m_netWidget->findChild<QTreeView*>("treeView");
    if (!tree) return;

    QAbstractItemModel *model = tree->model();
    if (!model) return;

    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    bool dataChanged = false;

    for (int r = 0; r < model->rowCount(); ++r) {
        QModelIndex idxName = model->index(r, 0);
        QModelIndex idxRx   = model->index(r, 4);
        QModelIndex idxTx   = model->index(r, 5);

        QString appName = model->data(idxName, Qt::UserRole + 1).toString();
        if (appName.isEmpty()) continue;

        quint64 rxSpeed = model->data(idxRx, Qt::UserRole + 5).toULongLong();
        quint64 txSpeed = model->data(idxTx, Qt::UserRole + 5).toULongLong();

        if (rxSpeed > 0 || txSpeed > 0) {
            m_historyData[today][appName].rxBytes += rxSpeed;
            m_historyData[today][appName].txBytes += txSpeed;
            dataChanged = true;
        }

        // FIX: Extract QPixmap from QVariant correctly, as NetworkTreeModel returns a Pixmap, not an Icon.
        if (!m_iconCache.contains(appName)) {
            QVariant iconVar = model->data(idxName, Qt::DecorationRole);
            if (!iconVar.isNull() && iconVar.canConvert<QPixmap>()) {
                m_iconCache.insert(appName, QIcon(iconVar.value<QPixmap>()));
            }
        }
    }

    if (dataChanged) {
        m_saveCounter++;
        if (m_saveCounter >= 60) {
            saveHistoryToJson();
            m_saveCounter = 0;
        }
    }
}

void MainWindow::refreshHistoryUI()
{
    int filterIdx = ui->cmbHistoryRange->currentIndex();
    QDate today = QDate::currentDate();

    quint64 globalRx = 0, globalTx = 0;
    QString topApp = "No Data";
    quint64 topAppTotal = 0;

    // FIX: Save the expanded state of the tree before clearing it
    QSet<QString> expandedDates;
    for (int i = 0; i < ui->treeHistory->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = ui->treeHistory->topLevelItem(i);
        if (item->isExpanded()) {
            expandedDates.insert(item->text(0));
        }
    }

    ui->treeHistory->clear();

    QIcon defaultAppIcon = qApp->style()->standardIcon(QStyle::SP_ComputerIcon);

    if (filterIdx == 0) {
        // Mode: TODAY -> Flat list of applications
        QMap<QString, DailyAppStat> todayStats;
        QString todayStr = today.toString("yyyy-MM-dd");

        if (m_historyData.contains(todayStr)) {
            todayStats = m_historyData[todayStr];
        }

        QList<QPair<QString, DailyAppStat>> sortedList;
        for (auto it = todayStats.constBegin(); it != todayStats.constEnd(); ++it) {
            sortedList.append(qMakePair(it.key(), it.value()));
            globalRx += it.value().rxBytes;
            globalTx += it.value().txBytes;

            quint64 appTotal = it.value().rxBytes + it.value().txBytes;
            if (appTotal > topAppTotal) {
                topAppTotal = appTotal;
                topApp = it.key();
            }
        }

        std::sort(sortedList.begin(), sortedList.end(), [](const QPair<QString, DailyAppStat>& a, const QPair<QString, DailyAppStat>& b) {
            return (a.second.rxBytes + a.second.txBytes) > (b.second.rxBytes + b.second.txBytes);
        });

        for (const auto &pair : sortedList) {
            QTreeWidgetItem *appItem = new QTreeWidgetItem(ui->treeHistory);
            appItem->setText(0, pair.first);
            appItem->setIcon(0, m_iconCache.value(pair.first, defaultAppIcon));
            appItem->setText(1, fmtBytes(pair.second.rxBytes));
            appItem->setText(2, fmtBytes(pair.second.txBytes));
            appItem->setText(3, fmtBytes(pair.second.rxBytes + pair.second.txBytes));
        }

    } else {
        // Mode: MULTI-DAY -> Tree grouped by Date
        QMap<QString, quint64> overallAppTotals;

        auto dateIt = m_historyData.constEnd();
        while (dateIt != m_historyData.constBegin()) {
            --dateIt;
            QDate nodeDate = QDate::fromString(dateIt.key(), "yyyy-MM-dd");
            bool includeDate = false;

            if (filterIdx == 1 && nodeDate.daysTo(today) <= 7 && nodeDate.daysTo(today) >= 0) includeDate = true;
            else if (filterIdx == 2 && nodeDate.year() == today.year() && nodeDate.month() == today.month()) includeDate = true;
            else if (filterIdx == 3) includeDate = true;

            if (!includeDate) continue;

            quint64 dateRx = 0, dateTx = 0;
            QTreeWidgetItem *dateItem = new QTreeWidgetItem(ui->treeHistory);
            dateItem->setText(0, dateIt.key());

            QList<QPair<QString, DailyAppStat>> sortedDailyList;
            for (auto appIt = dateIt.value().constBegin(); appIt != dateIt.value().constEnd(); ++appIt) {
                sortedDailyList.append(qMakePair(appIt.key(), appIt.value()));
                dateRx += appIt.value().rxBytes;
                dateTx += appIt.value().txBytes;

                globalRx += appIt.value().rxBytes;
                globalTx += appIt.value().txBytes;

                overallAppTotals[appIt.key()] += (appIt.value().rxBytes + appIt.value().txBytes);
            }

            dateItem->setText(1, fmtBytes(dateRx));
            dateItem->setText(2, fmtBytes(dateTx));
            dateItem->setText(3, fmtBytes(dateRx + dateTx));

            dateItem->setIcon(0, qApp->style()->standardIcon(QStyle::SP_FileDialogDetailedView));

            std::sort(sortedDailyList.begin(), sortedDailyList.end(), [](const QPair<QString, DailyAppStat>& a, const QPair<QString, DailyAppStat>& b) {
                return (a.second.rxBytes + a.second.txBytes) > (b.second.rxBytes + b.second.txBytes);
            });

            for (const auto &pair : sortedDailyList) {
                QTreeWidgetItem *appItem = new QTreeWidgetItem(dateItem);
                appItem->setText(0, pair.first);
                appItem->setIcon(0, m_iconCache.value(pair.first, defaultAppIcon));
                appItem->setText(1, fmtBytes(pair.second.rxBytes));
                appItem->setText(2, fmtBytes(pair.second.txBytes));
                appItem->setText(3, fmtBytes(pair.second.rxBytes + pair.second.txBytes));
            }

            // FIX: Restore the expanded state for this specific date node
            if (expandedDates.contains(dateItem->text(0))) {
                dateItem->setExpanded(true);
            }
        }

        for (auto it = overallAppTotals.constBegin(); it != overallAppTotals.constEnd(); ++it) {
            if (it.value() > topAppTotal) {
                topAppTotal = it.value();
                topApp = it.key();
            }
        }
    }

    // Update Summary Cards
    ui->lblHistRx->setText(fmtBytes(globalRx));
    ui->lblHistTx->setText(fmtBytes(globalTx));

    if (topApp != "No Data") {
        QString iconAppName = topApp;
        topApp[0] = topApp[0].toUpper();
        topApp = topApp.remove(".exe");
        ui->lblHistTopApp->setText(QString("%1\n%2").arg(topApp, fmtBytes(topAppTotal)));
        ui->lblHistTopIcon->setPixmap(m_iconCache.value(iconAppName, defaultAppIcon).pixmap(35, 35));
    } else {
        ui->lblHistTopApp->setText("No Data");
        ui->lblHistTopIcon->clear();
    }
}

void MainWindow::onHistoryFilterChanged(int)
{
    refreshHistoryUI();
}

void MainWindow::onClearHistory()
{
    if (QMessageBox::question(this, "Clear History", "Are you sure you want to permanently delete all network history data?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        m_historyData.clear();
        saveHistoryToJson();
        refreshHistoryUI();
    }
}

void MainWindow::onExportHistory()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Export History", QDir::homePath() + "/NetGuard_History.csv", "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Export Error", "Could not create the file.");
        return;
    }

    QTextStream out(&file);
    out << "Category,Download,Upload,Total Traffic\\n";

    for (int i = 0; i < ui->treeHistory->topLevelItemCount(); ++i) {
        QTreeWidgetItem *top = ui->treeHistory->topLevelItem(i);
        out << QString("\"%1\",\"%2\",\"%3\",\"%4\"\\n")
                   .arg(top->text(0), top->text(1), top->text(2), top->text(3));

        for (int j = 0; j < top->childCount(); ++j) {
            QTreeWidgetItem *child = top->child(j);
            out << QString("\" - %1\",\"%2\",\"%3\",\"%4\"\\n")
                       .arg(child->text(0), child->text(1), child->text(2), child->text(3));
        }
    }

    file.close();
    QMessageBox::information(this, "Export Successful", "Network history exported successfully!");
}

// JSON Load/Save functions remain the same as the previous iteration...
void MainWindow::loadHistoryFromJson()
{
    QFile file(m_historyFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) return;

    QJsonObject rootObj = doc.object();
    for (const QString &dateKey : rootObj.keys()) {
        QJsonObject appsObj = rootObj.value(dateKey).toObject();
        for (const QString &appName : appsObj.keys()) {
            QJsonObject statsObj = appsObj.value(appName).toObject();
            DailyAppStat stat;
            stat.rxBytes = static_cast<quint64>(statsObj.value("rx").toDouble());
            stat.txBytes = static_cast<quint64>(statsObj.value("tx").toDouble());
            m_historyData[dateKey][appName] = stat;
        }
    }
}

void MainWindow::saveHistoryToJson()
{
    if (m_historyFilePath.isEmpty()) return;

    QJsonObject rootObj;
    for (auto dateIt = m_historyData.constBegin(); dateIt != m_historyData.constEnd(); ++dateIt) {
        QJsonObject appsObj;
        for (auto appIt = dateIt.value().constBegin(); appIt != dateIt.value().constEnd(); ++appIt) {
            QJsonObject statObj;
            statObj["rx"] = static_cast<double>(appIt.value().rxBytes);
            statObj["tx"] = static_cast<double>(appIt.value().txBytes);
            appsObj[appIt.key()] = statObj;
        }
        rootObj[dateIt.key()] = appsObj;
    }

    QJsonDocument doc(rootObj);
    QFile file(m_historyFilePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(doc.toJson(QJsonDocument::Compact));
        file.close();
    }
}

// ============================================================================
// Settings Management
// ============================================================================

void MainWindow::loadSettings()
{
    QSettings settings("AliSakkaf", "NetGuardAIO");

    ui->chkRunAtStartup->setChecked(settings.value("General/RunAtStartup", true).toBool());
    ui->chkStartMinimized->setChecked(settings.value("General/StartMinimized", false).toBool());

    bool isDark = settings.value("General/DarkTheme", true).toBool();
    if (!isDark && StyleManager::instance().isDark()) {
        onToggleTheme();
    }

    ui->chkEnableOverlay->setChecked(settings.value("Overlay/Enabled", true).toBool());
    ui->spinOverlayFontSize->setValue(settings.value("Overlay/FontSize", 8).toInt());
    ui->spinOverlayOpacity->setValue(settings.value("Overlay/Opacity", 0).toInt());
    ui->cmbOverlayTextColor->setCurrentText(settings.value("Overlay/TextColor", "White").toString());
    ui->cmbOverlayBgColor->setCurrentText(settings.value("Overlay/BgColor", "Transparent").toString());

    applySettingsToOverlay();

    if (ui->chkStartMinimized->isChecked()) {
        QTimer::singleShot(100, this, &MainWindow::hide);
    }
}

void MainWindow::saveSettings()
{
    QSettings settings("AliSakkaf", "NetGuardAIO");

    settings.setValue("General/RunAtStartup", ui->chkRunAtStartup->isChecked());
    settings.setValue("General/StartMinimized", ui->chkStartMinimized->isChecked());
    settings.setValue("General/DarkTheme", StyleManager::instance().isDark());

    settings.setValue("Overlay/Enabled", ui->chkEnableOverlay->isChecked());
    settings.setValue("Overlay/FontSize", ui->spinOverlayFontSize->value());
    settings.setValue("Overlay/Opacity", ui->spinOverlayOpacity->value());
    settings.setValue("Overlay/TextColor", ui->cmbOverlayTextColor->currentText());
    settings.setValue("Overlay/BgColor", ui->cmbOverlayBgColor->currentText());
}

void MainWindow::applySettingsToOverlay()
{
    if (!m_overlay) return;

    m_overlay->setOverlayVisible(ui->chkEnableOverlay->isChecked());

    int fontSize = ui->spinOverlayFontSize->value();
    int opacity = ui->spinOverlayOpacity->value();

    QString txtColorHex = "white";
    QString selTxt = ui->cmbOverlayTextColor->currentText();
    if (selTxt == "Black") txtColorHex = "black";
    else if (selTxt == "Green") txtColorHex = "#34D399";
    else if (selTxt == "Blue") txtColorHex = "#60A5FA";
    else if (selTxt == "Yellow") txtColorHex = "#FBBF24";

    QColor bgColor(28, 33, 40);
    QString selBg = ui->cmbOverlayBgColor->currentText();
    if (selBg == "Solid Black") bgColor = Qt::black;
    else if (selBg == "Solid Blue") bgColor = QColor("#1D4ED8");
    else if (selBg == "Transparent") bgColor = Qt::transparent;

    m_overlay->setCustomStyle(fontSize, txtColorHex, opacity, bgColor);
    saveSettings();
}

void MainWindow::applyRunAtStartup(bool enable)
{
    QSettings boot("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    if (enable) {
        // Always use the canonical installed path (Program Files\NetGuard)
        QString appPath = QDir::toNativeSeparators(AppInstaller::installedExePath());
        boot.setValue("NetGuardAIO", "\"" + appPath + "\"");
    } else {
        boot.remove("NetGuardAIO");
    }
}

// ============================================================================
// Core Window Methods
// ============================================================================

void MainWindow::setupWidgets()
{
    m_netWidget = new NetworkWidget(this);
    m_fwWidget  = new FirewallWidget(this);
    m_hwWidget  = new HardwareWidget(this);

    ui->tabNetwork ->layout()->addWidget(m_netWidget);
    ui->tabFirewall->layout()->addWidget(m_fwWidget);
    ui->tabHardware->layout()->addWidget(m_hwWidget);

    m_overlay = new TaskbarOverlay;
    m_overlay->show();

    connect(m_netWidget, &NetworkWidget::speedUpdated, this, &MainWindow::onNetworkSpeed, Qt::QueuedConnection);
    connect(m_netWidget, &NetworkWidget::speedUpdated, m_overlay, &TaskbarOverlay::updateSpeed, Qt::QueuedConnection);

    connect(m_hwWidget->monitor(), &HardwareMonitor::snapshotReady, this, [this](const HardwareSnapshot &snap){
        onHardwareSnapshot(snap);
        if (m_overlay) {
            m_overlay->updateHardware(static_cast<int>(snap.cpuLoadPct), static_cast<int>(snap.ramLoadPct));
        }
    }, Qt::QueuedConnection);

    connect(m_overlay, &TaskbarOverlay::showMainWindowRequested, this, [this](){
        showNormal();
        activateWindow();
        raise();
    });

    connect(m_overlay, &TaskbarOverlay::minimizeRequested, this, [this](){
        hide();
    });

    connect(m_overlay, &TaskbarOverlay::exitRequested, qApp, &QApplication::quit);
}

void MainWindow::setupTray()
{
    m_trayMenu = new QMenu(this);

    QAction *actShow  = m_trayMenu->addAction("Show / Hide");
    QAction *actTheme = m_trayMenu->addAction("Toggle Theme");
    m_trayMenu->addSeparator();
    QAction *actQuit  = m_trayMenu->addAction("Exit");

    connect(actShow,  &QAction::triggered, this, [this](){
        isVisible() ? hide() : (showNormal(), activateWindow());
    });
    connect(actTheme, &QAction::triggered, this, &MainWindow::onToggleTheme);
    connect(actQuit,  &QAction::triggered, qApp, &QApplication::quit);

    QIcon trayIcon(":/icons/Ultimate_NetGuard_AIO.png");
    if (trayIcon.isNull()) trayIcon = QIcon(":/icons/app.ico");

    m_tray = new QSystemTrayIcon(trayIcon, this);
    m_tray->setContextMenu(m_trayMenu);
    m_tray->setToolTip("Ultimate NetGuard AIO");
    m_tray->show();

    connect(m_tray, &QSystemTrayIcon::activated, this, &MainWindow::onTrayActivated);
}

void MainWindow::onToggleTheme()
{
    StyleManager::instance().toggleTheme();
    m_theme = StyleManager::instance().currentMode();
    ui->btnThemeToggle->setChecked(m_theme == AppTheme::Light);
    ui->btnThemeToggle->setText(m_theme == AppTheme::Dark ? "☀" : "🌙");
    saveSettings();
}

void MainWindow::onMinimizeToTray()
{
    hide();
    m_tray->showMessage("NetGuard AIO", "Running in the background. Click the tray icon to restore.", QSystemTrayIcon::Information, 2000);
}

void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
        isVisible() ? hide() : (showNormal(), activateWindow(), raise());
}

void MainWindow::onNetworkSpeed(quint64 rxBps, quint64 txBps)
{
    ui->rxLabel->setText("↓ " + fmtSpeed(rxBps));
    ui->txLabel->setText("↑ " + fmtSpeed(txBps));
    m_tray->setToolTip(QString("NetGuard AIO\n↓ %1  ↑ %2").arg(fmtSpeed(rxBps), fmtSpeed(txBps)));
}

void MainWindow::onHardwareSnapshot(const HardwareSnapshot &snap)
{
    ui->cpuLabel->setText(QString("CPU %1%").arg(static_cast<int>(snap.cpuLoadPct)));
    ui->ramLabel->setText(QString("RAM %1%").arg(static_cast<int>(snap.ramLoadPct)));
}

void MainWindow::animateLiveBadge()
{
    m_badgeLit = !m_badgeLit;
    ui->liveBadge->setStyleSheet(m_badgeLit ? "color:#2EA043; font-weight:700; font-size:8pt;" : "color:#1A3A22; font-weight:700; font-size:8pt;");
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    hide();
    m_tray->showMessage("NetGuard AIO", "Running in the background.\nDouble-click the tray icon to restore.", QSystemTrayIcon::Information, 2000);
    event->ignore();
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange && isMinimized()) {
        hide();
        event->ignore();
        return;
    }
    QMainWindow::changeEvent(event);
}

QString MainWindow::fmtSpeed(quint64 bps)
{
    if (bps < 1024)      return QString("%1 B/s").arg(bps);
    if (bps < 1024*1024) return QString("%1 KB/s").arg(bps/1024.0,0,'f',1);
    return                     QString("%1 MB/s").arg(bps/(1024.0*1024),0,'f',2);
}

QString MainWindow::fmtBytes(quint64 bytes)
{
    if (bytes < 1024)             return QString("%1 B").arg(bytes);
    if (bytes < 1024*1024)        return QString("%1 KB").arg(bytes/1024.0, 0, 'f', 1);
    if (bytes < 1024*1024*1024ULL)return QString("%1 MB").arg(bytes/(1024.0*1024), 0, 'f', 2);
    return                               QString("%1 GB").arg(bytes/(1024.0*1024*1024), 0, 'f', 2);
}
