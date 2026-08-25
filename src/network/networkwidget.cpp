/**
 * @file    networkwidget.cpp
 * @author  Ali Sakkaf
 */
#include "networkwidget.h"
#include "ui_networkwidget.h"

// ── Theme Manager ──
#include "stylemanager.h"
#include "apptheme.h"

#include <QHeaderView>
#include <QMessageBox>
#include <QMetaObject>
#include <QStyle>
#include <QApplication>
#include <QMenu>
#include <QClipboard>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QDir>
#include <QProcess>
#include <QFileInfo>
#include <QFrame>
#include <QLineEdit>
#include <QPalette>
#include <QVariant>
#include <QPixmap>
#ifndef _WIN32_WINNT
#  define _WIN32_WINNT 0x0601
#endif
#include <windows.h>
#include <winsvc.h>
#include <shellapi.h>
#include <psapi.h>
#include <QtWin>
#include <vector>

// ============================================================================
// ── Windows Service Manager ──
// ============================================================================
class WindowsServiceManager {
public:
    static QString getServiceKeyName(const QString &displayName) {
        SC_HANDLE hSCM = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
        if (!hSCM) return displayName;

        std::wstring wDisp = displayName.toStdWString();
        DWORD dwSize = 0;
        GetServiceKeyNameW(hSCM, wDisp.c_str(), nullptr, &dwSize);
        if (dwSize > 0) {
            std::vector<wchar_t> buf(dwSize + 1);
            if (GetServiceKeyNameW(hSCM, wDisp.c_str(), buf.data(), &dwSize)) {
                CloseServiceHandle(hSCM);
                return QString::fromWCharArray(buf.data());
            }
        }
        CloseServiceHandle(hSCM);
        return displayName;
    }

    static bool stopService(const QString &svcName) {
        QString keyName = getServiceKeyName(svcName);
        SC_HANDLE hSCM = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
        if (!hSCM) return false;
        SC_HANDLE hSvc = OpenServiceW(hSCM, keyName.toStdWString().c_str(), SERVICE_STOP | SERVICE_QUERY_STATUS);
        if (!hSvc) {
            CloseServiceHandle(hSCM);
            return (QProcess::execute("net", {"stop", keyName}) == 0);
        }
        SERVICE_STATUS status;
        bool ok = ControlService(hSvc, SERVICE_CONTROL_STOP, &status);
        CloseServiceHandle(hSvc);
        CloseServiceHandle(hSCM);
        if (!ok) QProcess::execute("net", {"stop", keyName});
        return true;
    }

    static bool startService(const QString &svcName) {
        QString keyName = getServiceKeyName(svcName);
        SC_HANDLE hSCM = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
        if (!hSCM) return false;
        SC_HANDLE hSvc = OpenServiceW(hSCM, keyName.toStdWString().c_str(), SERVICE_START);
        if (!hSvc) {
            CloseServiceHandle(hSCM);
            return (QProcess::execute("net", {"start", keyName}) == 0);
        }
        bool ok = StartServiceW(hSvc, 0, nullptr);
        CloseServiceHandle(hSvc);
        CloseServiceHandle(hSCM);
        if (!ok) QProcess::execute("net", {"start", keyName});
        return true;
    }

    static bool restartService(const QString &svcName) {
        stopService(svcName);
        QThread::msleep(1000);
        return startService(svcName);
    }

    static bool setServiceDisabled(const QString &svcName, bool disable) {
        QString keyName = getServiceKeyName(svcName);
        SC_HANDLE hSCM = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
        if (!hSCM) return false;
        SC_HANDLE hSvc = OpenServiceW(hSCM, keyName.toStdWString().c_str(), SERVICE_CHANGE_CONFIG);
        if (!hSvc) {
            CloseServiceHandle(hSCM);
            return (QProcess::execute("sc", {"config", keyName, "start=", disable ? "disabled" : "demand"}) == 0);
        }
        bool ok = ChangeServiceConfigW(hSvc, SERVICE_NO_CHANGE, disable ? SERVICE_DISABLED : SERVICE_DEMAND_START, SERVICE_NO_CHANGE, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
        CloseServiceHandle(hSvc);
        CloseServiceHandle(hSCM);
        if (!ok) QProcess::execute("sc", {"config", keyName, "start=", disable ? "disabled" : "demand"});
        return ok;
    }
};

// ============================================================================
// ── Elevate Permissions to allow reading Process Paths (Crucial for Icons) ──
// ============================================================================
static void EnableDebugPrivilege() {
    HANDLE hToken;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        TOKEN_PRIVILEGES tp;
        LUID luid;
        if (LookupPrivilegeValue(nullptr, SE_DEBUG_NAME, &luid)) {
            tp.PrivilegeCount = 1;
            tp.Privileges[0].Luid = luid;
            tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr);
        }
        CloseHandle(hToken);
    }
}

// ============================================================================
// ── Process Properties Dialog Implementation ──
// ============================================================================
ProcessInfoDialog::ProcessInfoDialog(const QString &name, quint32 pid, const QString &path,
                                     const QString &rx, const QString &tx, const QString &total, const QString &pkts,
                                     const QIcon &icon, QWidget *parent)
    : QDialog(parent), m_path(path)
{
    setWindowTitle("Process Properties");
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setMinimumWidth(640);

    // ── Dynamic Theme Palette ──
    bool isDark = (StyleManager::instance().currentMode() == AppTheme::Dark);

    QString bg       = isDark ? "#1F2328" : "#F6F8FA";
    QString fg       = isDark ? "#E6EDF3" : "#24292E";
    QString border   = isDark ? "#30363D" : "#D0D7DE";
    QString btnBg    = isDark ? "#21262D" : "#EBECF0";
    QString btnHover = isDark ? "#30363D" : "#F3F4F6";
    QString textMuted= isDark ? "#8B949E" : "#57606A";
    QString selectBg = isDark ? "#1F6FEB" : "#0969DA";

    setStyleSheet(QString(
                      "QDialog { background-color: %1; color: %2; }"
                      "QLabel { color: %2; font-size: 10pt; }"
                      "QLineEdit { background: transparent; border: none; color: %2; font-size: 10pt; selection-background-color: %7; selection-color: #FFFFFF; }"
                      "QPushButton { background-color: %4; border: 1px solid %3; border-radius: 6px; padding: 6px 16px; color: %2; font-weight: bold; }"
                      "QPushButton:hover { background-color: %5; }"
                      "QPushButton:disabled { color: %6; background-color: %1; }"
                      "QGroupBox { border: 1px solid %3; border-radius: 6px; margin-top: 12px; font-weight: bold; color: %2; }"
                      "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 10px; }"
                      ).arg(bg, fg, border, btnBg, btnHover, textMuted, selectBg));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // ── Header (Icon, Name, Type) ──
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *iconLabel = new QLabel();
    iconLabel->setPixmap(icon.pixmap(45, 45));
    iconLabel->setFixedSize(45, 45);
    iconLabel->setScaledContents(true);
    headerLayout->addWidget(iconLabel);

    QVBoxLayout *nameLayout = new QVBoxLayout();
    QLabel *nameLabel = new QLabel(name);
    nameLabel->setWordWrap(true);
    nameLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    nameLabel->setStyleSheet("font-size: 13pt; font-weight: bold;");

    QString typeStr = "Application (EXE)";
    if (pid == 0 || pid == 4) typeStr = "System OS / Kernel Overhead";
    else if (name.startsWith("Service:")) typeStr = "Windows Service";

    QLabel *typeLabel = new QLabel(typeStr);
    typeLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(textMuted));

    nameLayout->addWidget(nameLabel);
    nameLayout->addWidget(typeLabel);
    nameLayout->addStretch();

    headerLayout->addLayout(nameLayout);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);

    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet(QString("background-color: %1;").arg(border));
    mainLayout->addWidget(line);

    // ── Process Info ──
    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(12);

    QLineEdit *pidEdit = new QLineEdit(QString::number(pid));
    pidEdit->setReadOnly(true);
    formLayout->addRow("<b>Process ID:</b>", pidEdit);

    QLabel *pathLabel = new QLabel(path.isEmpty() ? "N/A (System / Protected)" : path);
    pathLabel->setWordWrap(true);
    pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    formLayout->addRow("<b>File Path:</b>", pathLabel);

    mainLayout->addLayout(formLayout);

    // ── Traffic Stats ──
    QGroupBox *usageGroup = new QGroupBox(" Network Traffic Usage ");
    QFormLayout *usageLayout = new QFormLayout(usageGroup);
    usageLayout->setContentsMargins(15, 20, 15, 15);
    usageLayout->setSpacing(10);

    auto createStatEdit = [](const QString& text) {
        QLineEdit *le = new QLineEdit(text);
        le->setReadOnly(true);
        return le;
    };

    usageLayout->addRow("<b>Download Speed:</b>", createStatEdit(rx));
    usageLayout->addRow("<b>Upload Speed:</b>", createStatEdit(tx));
    usageLayout->addRow("<b>Total Consumed:</b>", createStatEdit(total));
    usageLayout->addRow("<b>Total Packets:</b>", createStatEdit(pkts));
    mainLayout->addWidget(usageGroup);

    // ── Action Buttons ──
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *btnOpenLoc = new QPushButton("📂 Open File Location");
    QPushButton *btnClose = new QPushButton("Close");

    if (path.isEmpty() || pid == 0 || pid == 4) btnOpenLoc->setEnabled(false);

    btnLayout->addWidget(btnOpenLoc);
    btnLayout->addStretch();
    btnLayout->addWidget(btnClose);

    mainLayout->addLayout(btnLayout);

    connect(btnOpenLoc, &QPushButton::clicked, this, &ProcessInfoDialog::onOpenLocation);
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
}

void ProcessInfoDialog::onOpenLocation() {
    if (m_path.isEmpty()) return;
    QFileInfo fi(m_path);
    if (fi.exists()) {
        QString param = QString("/select,\"%1\"").arg(QDir::toNativeSeparators(m_path));
        QProcess::startDetached("explorer.exe", {param});
    } else {
        QMessageBox::warning(this, "Not Found", "The executable no longer exists at this path.");
    }
}

// ============================================================================
// ── NetworkWidget Implementation ──
// ============================================================================
NetworkWidget::NetworkWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::NetworkWidget)
{
    ui->setupUi(this);
    // ── Ensure Windows Firewall Inbound Rule for Application Traffic ──
    QString appPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    QProcess::execute("netsh", {"advfirewall", "firewall", "add", "rule", "name=NetGuardAIO", "dir=in", "action=allow", "program=" + appPath, "enable=yes"});

    // ── Elevate Process Privileges ──
    EnableDebugPrivilege();

    m_monitor = new NetworkMonitor(this);

    connect(m_monitor, &NetworkMonitor::speedUpdated,          this, &NetworkWidget::onSpeedUpdated);
    connect(m_monitor, &NetworkMonitor::packetsCapturedBatch,  this, &NetworkWidget::onPacketsCapturedBatch);
    connect(m_monitor, &NetworkMonitor::captureStarted,       this, &NetworkWidget::onCaptureStarted);
    connect(m_monitor, &NetworkMonitor::captureStopped,       this, &NetworkWidget::onCaptureStopped);
    connect(m_monitor, &NetworkMonitor::captureError,         this, &NetworkWidget::onCaptureError);

    m_model = new NetworkTreeModel(this);
    m_proxy = new QSortFilterProxyModel(this);
    m_proxy->setSourceModel(m_model);
    m_proxy->setRecursiveFilteringEnabled(true);
    m_proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);

    ui->treeView->setModel(m_proxy);
    ui->treeView->setSortingEnabled(true);
    ui->treeView->sortByColumn(COL_BYTES, Qt::DescendingOrder);

    ui->treeView->header()->setSectionResizeMode(COL_NAME,  QHeaderView::Stretch);
    ui->treeView->header()->setSectionResizeMode(COL_SRC,   QHeaderView::ResizeToContents);
    ui->treeView->header()->setSectionResizeMode(COL_DST,   QHeaderView::ResizeToContents);
    ui->treeView->header()->setSectionResizeMode(COL_SRC,   QHeaderView::ResizeToContents);
    ui->treeView->header()->setSectionResizeMode(COL_RX,    QHeaderView::ResizeToContents);
    ui->treeView->header()->setSectionResizeMode(COL_TX,    QHeaderView::ResizeToContents);
    ui->treeView->header()->setSectionResizeMode(COL_BYTES, QHeaderView::ResizeToContents);
    ui->treeView->header()->setSectionResizeMode(COL_PKTS,  QHeaderView::ResizeToContents);

    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeView, &QTreeView::customContextMenuRequested, this, &NetworkWidget::onTreeViewContextMenu);

    connect(ui->btnStartStop, &QPushButton::clicked, this, &NetworkWidget::onStartStop);
    connect(ui->btnClear,     &QPushButton::clicked, this, &NetworkWidget::onClearTable);
    connect(ui->btnExpandAll, &QPushButton::clicked, this, [this]() {
        static bool expanded = false;
        if (!expanded) { ui->treeView->expandAll(); ui->btnExpandAll->setText("Collapse All"); }
        else           { ui->treeView->collapseAll(); ui->btnExpandAll->setText("Expand All"); }
        expanded = !expanded;
    });

    connect(ui->edtFilter, &QLineEdit::textChanged, this, &NetworkWidget::onFilterChanged);
    connect(ui->cmbAdapter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &NetworkWidget::onAdapterChanged);
    ui->cmbAdapter->installEventFilter(this);

    populateAdapterCombo();
}

NetworkWidget::~NetworkWidget()
{
    m_monitor->stopCapture();
    m_monitor->wait(1500);
    delete ui;
}

void NetworkWidget::onTreeViewContextMenu(const QPoint &pos)
{
    QModelIndex proxyIdx = ui->treeView->indexAt(pos);
    if (!proxyIdx.isValid()) return;

    QModelIndex idx = m_proxy->mapToSource(proxyIdx);

    quint32 pid = idx.data(Qt::UserRole).toUInt();
    QString name = idx.data(Qt::UserRole + 1).toString();
    bool isProcessNode = idx.data(Qt::UserRole + 2).toBool();

    QMenu menu(this);

    // ── Context Menu Styling ──
    bool isDark = (StyleManager::instance().currentMode() == AppTheme::Dark);
    if (isDark) {
        menu.setStyleSheet(
            "QMenu { background-color: #1F2328; color: #E6EDF3; border: 1px solid #30363D; padding: 4px; border-radius: 6px; }"
            "QMenu::item { padding: 6px 24px 6px 24px; border-radius: 4px; margin: 1px 4px; }"
            "QMenu::item:selected { background-color: #238636; color: #FFFFFF; }"
            "QMenu::separator { height: 1px; background: #21262D; margin: 4px 10px; }"
            );
    } else {
        menu.setStyleSheet(
            "QMenu { background-color: #FFFFFF; color: #24292E; border: 1px solid #D0D7DE; padding: 4px; border-radius: 6px; }"
            "QMenu::item { padding: 6px 24px 6px 24px; border-radius: 4px; margin: 1px 4px; }"
            "QMenu::item:selected { background-color: #0969DA; color: #FFFFFF; }"
            "QMenu::separator { height: 1px; background: #D1D5DA; margin: 4px 10px; }"
            );
    }

    bool isServiceNode = name.startsWith("Service:") || (name.compare("svchost.exe", Qt::CaseInsensitive) == 0);
    QString serviceCleanName = name.startsWith("Service:") ? name.mid(8).trimmed() : QString();

    if (isServiceNode && !serviceCleanName.isEmpty()) {
        QAction *actStop = menu.addAction("⏹ Stop Service");
        connect(actStop, &QAction::triggered, this, [this, serviceCleanName]() {
            if (QMessageBox::question(this, "Stop Service", QString("Are you sure you want to stop service '%1'?").arg(serviceCleanName)) == QMessageBox::Yes) {
                if (WindowsServiceManager::stopService(serviceCleanName)) {
                    QMessageBox::information(this, "Service Control", "Service stop command sent successfully.");
                } else {
                    QMessageBox::warning(this, "Error", "Could not stop service. Make sure you run as Administrator.");
                }
            }
        });

        QAction *actRestart = menu.addAction("🔄 Restart Service");
        connect(actRestart, &QAction::triggered, this, [this, serviceCleanName]() {
            if (QMessageBox::question(this, "Restart Service", QString("Are you sure you want to restart service '%1'?").arg(serviceCleanName)) == QMessageBox::Yes) {
                if (WindowsServiceManager::restartService(serviceCleanName)) {
                    QMessageBox::information(this, "Service Control", "Service restarted successfully.");
                } else {
                    QMessageBox::warning(this, "Error", "Could not restart service. Make sure you run as Administrator.");
                }
            }
        });

        QAction *actDisable = menu.addAction("🚫 Disable Service");
        connect(actDisable, &QAction::triggered, this, [this, serviceCleanName]() {
            if (QMessageBox::question(this, "Disable Service", QString("Are you sure you want to set service '%1' startup type to Disabled?").arg(serviceCleanName)) == QMessageBox::Yes) {
                if (WindowsServiceManager::setServiceDisabled(serviceCleanName, true)) {
                    QMessageBox::information(this, "Service Control", "Service startup type set to Disabled.");
                } else {
                    QMessageBox::warning(this, "Error", "Could not disable service. Make sure you run as Administrator.");
                }
            }
        });

        QAction *actStart = menu.addAction("▶ Start / Enable Service");
        connect(actStart, &QAction::triggered, this, [this, serviceCleanName]() {
            WindowsServiceManager::setServiceDisabled(serviceCleanName, false);
            if (WindowsServiceManager::startService(serviceCleanName)) {
                QMessageBox::information(this, "Service Control", "Service started successfully.");
            } else {
                QMessageBox::warning(this, "Error", "Could not start service. Make sure you run as Administrator.");
            }
        });

        menu.addSeparator();
    } else if (pid != 0 && pid != 4) {
        QAction *actKill = menu.addAction("☠ Kill Process (End Task)");
        connect(actKill, &QAction::triggered, this, [this, pid]() {
            if (QMessageBox::question(this, "Kill Process", QString("Are you sure you want to forcibly terminate PID %1?").arg(pid)) == QMessageBox::Yes) {
                HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
                if (hProc) { TerminateProcess(hProc, 0); CloseHandle(hProc); }
                else { QMessageBox::warning(this, "Error", "Access Denied. Make sure you run as Administrator."); }
            }
        });
        menu.addSeparator();
    }

    if (isProcessNode) {
        QAction *actInfo = menu.addAction("ℹ Process Properties...");
        connect(actInfo, &QAction::triggered, this, [this, idx, pid, name]() {
            QString path = getExactProcessPath(pid, name);

            QString rx = idx.siblingAtColumn(4).data().toString(); // COL_RX
            QString tx = idx.siblingAtColumn(5).data().toString(); // COL_TX
            QString total = idx.siblingAtColumn(6).data().toString(); // COL_BYTES
            QString pkts = idx.siblingAtColumn(7).data().toString();  // COL_PKTS

            QVariant iconVar = idx.siblingAtColumn(0).data(Qt::DecorationRole); // COL_NAME
            QIcon icon;
            if (!iconVar.isNull()) {
                QPixmap pm = iconVar.value<QPixmap>();
                if (!pm.isNull()) icon = QIcon(pm);
            }
            if (icon.isNull()) icon = qApp->style()->standardIcon(QStyle::SP_ComputerIcon);

            ProcessInfoDialog dlg(name, pid, path, rx, tx, total, pkts, icon, this);
            dlg.exec();
        });
    } else {
        QAction *actCopy = menu.addAction("📋 Copy Connection Details");
        connect(actCopy, &QAction::triggered, this, [name]() {
            QApplication::clipboard()->setText(name);
        });
    }

    menu.exec(ui->treeView->viewport()->mapToGlobal(pos));
}

void NetworkWidget::setFilterVirtualAdapters(bool filterVirtual) {
    m_filterVirtualAdapters = filterVirtual;
    populateAdapterCombo();
}

void NetworkWidget::autoStart() {
    populateAdapterCombo();
    if (ui->cmbAdapter->count() > 0) {
        ui->cmbAdapter->setCurrentIndex(0); // Smart Auto-Detect
        onAdapterChanged(0);
    }
    m_monitor->startCapture();
}

void NetworkWidget::populateAdapterCombo() {
    QString prevData;
    if (ui->cmbAdapter && ui->cmbAdapter->currentIndex() >= 0)
        prevData = ui->cmbAdapter->currentData().toString();

    m_adapters = NetworkMonitor::enumerateAdapters(m_filterVirtualAdapters);
    ui->cmbAdapter->blockSignals(true);
    ui->cmbAdapter->clear();

    // ── Smart Auto-Detect & Select All Options ──
    ui->cmbAdapter->addItem("⚡ Smart Auto-Detect (Auto)", "AUTO");
    ui->cmbAdapter->addItem("🌐 All Network Adapters (Select All)", "ALL");

    for (const auto &ai : m_adapters)
        ui->cmbAdapter->addItem(ai.description, ai.ip);
    ui->cmbAdapter->blockSignals(false);

    int restoreIdx = 0; // Default to Auto
    if (!prevData.isEmpty()) {
        for (int i = 0; i < ui->cmbAdapter->count(); ++i) {
            if (ui->cmbAdapter->itemData(i).toString() == prevData) { restoreIdx = i; break; }
        }
    }

    ui->cmbAdapter->setCurrentIndex(restoreIdx);
    onAdapterChanged(restoreIdx);
}

void NetworkWidget::selectAdapterByIPOrAuto(const QString &ipOrAuto)
{
    for (int i = 0; i < ui->cmbAdapter->count(); ++i) {
        if (ui->cmbAdapter->itemData(i).toString() == ipOrAuto) {
            ui->cmbAdapter->setCurrentIndex(i);
            break;
        }
    }
}

// ── Event filter: refresh adapters when ComboBox dropdown is opened ──
bool NetworkWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->cmbAdapter && event->type() == QEvent::MouseButtonPress) {
        populateAdapterCombo();
    }
    return QWidget::eventFilter(obj, event);
}

void NetworkWidget::onAdapterChanged(int idx) {
    if (idx < 0 || idx >= ui->cmbAdapter->count()) return;

    QString selectedData = ui->cmbAdapter->itemData(idx).toString();
    QString targetIP;

    if (selectedData == "AUTO") {
        int bestIdx = NetworkMonitor::recommendedAdapterIndex(m_adapters, m_filterVirtualAdapters);
        if (bestIdx >= 0 && bestIdx < m_adapters.size()) {
            targetIP = m_adapters[bestIdx].ip;
            ui->lblStatus->setText("⚡ Auto-Detecting: " + m_adapters[bestIdx].description);
        }
    } else if (selectedData == "ALL") {
        targetIP = "ALL";
        ui->lblStatus->setText("🌐 Capturing All Network Interfaces (Select All)");
    } else {
        targetIP = selectedData;
        ui->lblStatus->setText("Selected Adapter: " + ui->cmbAdapter->currentText());
    }

    if (!targetIP.isEmpty()) {
        bool wasCapturing = m_capturing;
        if (wasCapturing) {
            m_monitor->stopCapture();
            m_monitor->wait(1000);
        }
        m_monitor->setAdapterIP(targetIP);
        if (wasCapturing) {
            m_monitor->startCapture();
        }
    }
}

void NetworkWidget::onStartStop()
{
    if (!m_capturing) {
        if (m_adapters.isEmpty()) return;
        m_monitor->startCapture();
    } else {
        // ── Force stop: try graceful first, then terminate ──
        m_monitor->stopCapture();
        if (!m_monitor->wait(2000)) {
            // Thread didn't stop in 2s → force terminate
            m_monitor->terminate();
            m_monitor->wait(1000);
        }
        onCaptureStopped();
    }
}

void NetworkWidget::onCaptureStarted() {
    m_capturing = true;
    ui->btnStartStop->setText("⏹  Stop");
    ui->cmbAdapter->setEnabled(false);
    ui->lblStatus->setText("● Live capture…");
}
void NetworkWidget::onCaptureStopped() {
    m_capturing = false;
    ui->btnStartStop->setText("▶  Start");
    ui->cmbAdapter->setEnabled(true);
    ui->lblStatus->setText("Stopped.");
}

void NetworkWidget::onPacketsCapturedBatch(const QList<CapturedPacketInfo> &batch)
{
    m_model->addPackets(batch);
    m_totalPkts += batch.size();
    ui->lblPackets->setText(QString::number(m_totalPkts));

    for (const auto &info : batch) {
        if (!info.proc.isEmpty() && !m_iconCache.contains(info.proc)) {
            if (info.pid == 4 || info.pid == 0 || info.isService) {
                QIcon svcIcon = style()->standardIcon(QStyle::SP_ComputerIcon);
                m_iconCache.insert(info.proc, svcIcon);
                m_model->setProcessIcon(info.proc, svcIcon);
            } else {
                m_iconCache.insert(info.proc, QIcon());
                QMetaObject::invokeMethod(this, [this, pid = info.pid, proc = info.proc]() {
                    QIcon icon = getIconByPid(pid, proc);
                    if (!icon.isNull()) m_model->setProcessIcon(proc, icon);
                }, Qt::QueuedConnection);
            }
        }
    }
}

void NetworkWidget::onSpeedUpdated(quint64 rxBps, quint64 txBps) {
    quint64 modelRx = m_model ? m_model->getTotalRxSpeed() : 0;
    quint64 modelTx = m_model ? m_model->getTotalTxSpeed() : 0;

    quint64 finalRx = qMax(rxBps, modelRx);
    quint64 finalTx = (modelTx > 0) ? qMax(modelTx, qMin(txBps, modelTx * 3 + 10240)) : qMin(txBps, static_cast<quint64>(20480));

    ui->lblRxVal->setText(NetworkTreeModel::formatSpeed(finalRx));
    ui->lblTxVal->setText(NetworkTreeModel::formatSpeed(finalTx));
    emit speedUpdated(finalRx, finalTx);
}
// void NetworkWidget::onCaptureError(const QString &msg) { ui->lblStatus->setText("⚠ " + msg); QMessageBox::critical(this, "Capture Error", msg); onCaptureStopped(); }

void NetworkWidget::onCaptureError(const QString &msg) {
    ui->lblStatus->setText("⚠ " + msg);
    onCaptureStopped();
    // ── If there are low-level socket bind errors, don't show annoying popups.
    // Force a complete app restart silently to clean up lingering raw sockets.
    if (msg.contains("bind", Qt::CaseInsensitive)) {
        QProcess::startDetached(QCoreApplication::applicationFilePath(), QStringList());
        QCoreApplication::quit();
    }
}

void NetworkWidget::onFilterChanged(const QString &text) { m_proxy->setFilterFixedString(text); if (!text.isEmpty()) ui->treeView->expandAll(); }

void NetworkWidget::onClearTable()
{
    m_model->clear();
    m_totalPkts = 0;
    ui->lblPackets->setText("0");
    m_iconCache.clear();
    ui->btnExpandAll->setText("Expand All");
}

void NetworkWidget::onExpandAll()
{
    if (ui->btnExpandAll->text().contains("Expand")) {
        ui->treeView->expandAll();
        ui->btnExpandAll->setText("Collapse All");
    } else {
        ui->treeView->collapseAll();
        ui->btnExpandAll->setText("Expand All");
    }
}

QString NetworkWidget::getExactProcessPath(quint32 pid, const QString &procName)
{
    if (pid == 0 || pid == 4) return QString();

    QString result;
    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (hProc) {
        wchar_t path[MAX_PATH] = {};
        DWORD size = MAX_PATH;
        if (QueryFullProcessImageNameW(hProc, 0, path, &size)) {
            result = QString::fromWCharArray(path, static_cast<int>(size));
        }
        CloseHandle(hProc);
    }

    if (result.isEmpty() && !procName.isEmpty()) {
        wchar_t sysDir[MAX_PATH] = {};
        GetSystemDirectoryW(sysDir, MAX_PATH);
        QString fallback = QString::fromWCharArray(sysDir) + "\\" + procName;
        if (QFileInfo::exists(fallback)) return fallback;
    }

    return result;
}

QIcon NetworkWidget::getIconByPid(quint32 pid, const QString &procName)
{
    QString fullPath = getExactProcessPath(pid, procName);
    SHFILEINFOW sfi = {};
    QIcon icon;

    if (!fullPath.isEmpty()) {
        if (SHGetFileInfoW(fullPath.toStdWString().c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_SMALLICON)) {
            if (sfi.hIcon) { icon = QIcon(QtWin::fromHICON(sfi.hIcon)); DestroyIcon(sfi.hIcon); return icon; }
        }
    }

    if (SHGetFileInfoW(procName.toStdWString().c_str(), FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES)) {
        if (sfi.hIcon) { icon = QIcon(QtWin::fromHICON(sfi.hIcon)); DestroyIcon(sfi.hIcon); }
    }
    return icon;
}



