/**
 * @file    networkwidget.cpp
 * @author  Ali Sakkaf
 */
#include "networkwidget.h"
#include "ui_networkwidget.h"

// ── استدعاء مدير الثيمات الخاص ببرنامجك ──
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
#include <shellapi.h>
#include <psapi.h>
#include <QtWin>

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
    setMinimumWidth(520);

    // ── الربط المباشر مع مدير الثيمات الخاص بك ──
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
    iconLabel->setPixmap(icon.pixmap(150, 150));
    headerLayout->addWidget(iconLabel);

    QVBoxLayout *nameLayout = new QVBoxLayout();
    QLineEdit *nameEdit = new QLineEdit(name);
    nameEdit->setReadOnly(true);
    nameEdit->setStyleSheet("font-size: 13pt; font-weight: bold;");

    QString typeStr = "Application (EXE)";
    if (pid == 0 || pid == 4) typeStr = "System OS / Kernel Overhead";
    else if (name.startsWith("Service:")) typeStr = "Windows Service";

    QLabel *typeLabel = new QLabel(typeStr);
    typeLabel->setStyleSheet(QString("color: %1;").arg(textMuted));

    nameLayout->addWidget(nameEdit);
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

    QLineEdit *pathEdit = new QLineEdit(path.isEmpty() ? "N/A (System / Protected)" : path);
    pathEdit->setReadOnly(true);
    pathEdit->setCursorPosition(0);
    formLayout->addRow("<b>File Path:</b>", pathEdit);

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
    EnableDebugPrivilege();

    // ── [عربي] السطر السحري لفك حظر الويندوز عن الـ Download (يعمل بصمت تام) ──
    QString appPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    QProcess::execute("netsh", {"advfirewall", "firewall", "add", "rule", "name=NetGuardAIO", "dir=in", "action=allow", "program=" + appPath, "enable=yes"});

    qRegisterMetaType<QList<CapturedPacketInfo>>("QList<CapturedPacketInfo>");

    m_model = new NetworkTreeModel(this);
    m_proxy = new QSortFilterProxyModel(this);
    m_proxy->setSourceModel(m_model);
    m_proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxy->setFilterKeyColumn(-1);
    m_proxy->setRecursiveFilteringEnabled(true);

    // ── FIX: Set the custom SortRole for precise numeric sorting ──
    m_proxy->setSortRole(Qt::UserRole + 5);

    ui->treeView->setModel(m_proxy);
    ui->treeView->setUniformRowHeights(true);
    ui->treeView->setAnimated(true);
    ui->treeView->setExpandsOnDoubleClick(true);
    ui->treeView->setRootIsDecorated(true);
    ui->treeView->setAlternatingRowColors(false);
    ui->treeView->setIconSize(QSize(18, 18));
    ui->treeView->setIndentation(24);
    ui->treeView->setItemsExpandable(true);
    ui->treeView->setSortingEnabled(true);
    ui->treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeView, &QTreeView::customContextMenuRequested, this, &NetworkWidget::onTreeViewContextMenu);

    QHeaderView *hv = ui->treeView->header();
    hv->setMinimumSectionSize(50);
    hv->setSectionResizeMode(0, QHeaderView::Interactive);       // COL_NAME
    hv->setSectionResizeMode(1, QHeaderView::ResizeToContents);  // COL_SRC
    hv->setSectionResizeMode(2, QHeaderView::ResizeToContents);  // COL_DST
    hv->setSectionResizeMode(3, QHeaderView::ResizeToContents);  // COL_SERVICE
    hv->setSectionResizeMode(4, QHeaderView::Fixed);             // COL_RX
    hv->setSectionResizeMode(5, QHeaderView::Fixed);             // COL_TX
    hv->setSectionResizeMode(6, QHeaderView::ResizeToContents);  // COL_BYTES
    hv->setSectionResizeMode(7, QHeaderView::Stretch);           // COL_PKTS

    hv->resizeSection(0, 260); // COL_NAME
    hv->resizeSection(4, 110); // COL_RX
    hv->resizeSection(5, 110); // COL_TX
    hv->setHighlightSections(false);
    hv->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    m_monitor = new NetworkMonitor(this);

    connect(m_monitor, &NetworkMonitor::packetsCapturedBatch, this, &NetworkWidget::onPacketsCapturedBatch, Qt::QueuedConnection);
    connect(m_monitor, &NetworkMonitor::speedUpdated, this, &NetworkWidget::onSpeedUpdated, Qt::QueuedConnection);
    connect(m_monitor, &NetworkMonitor::captureError, this, &NetworkWidget::onCaptureError, Qt::QueuedConnection);
    connect(m_monitor, &NetworkMonitor::captureStarted, this, &NetworkWidget::onCaptureStarted, Qt::QueuedConnection);
    connect(m_monitor, &NetworkMonitor::captureStopped, this, &NetworkWidget::onCaptureStopped, Qt::QueuedConnection);

    connect(ui->cmbAdapter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &NetworkWidget::onAdapterChanged);
    connect(ui->btnStartStop, &QPushButton::clicked, this, &NetworkWidget::onStartStop);
    connect(ui->btnClear, &QPushButton::clicked, this, &NetworkWidget::onClearTable);
    connect(ui->btnExpandAll, &QPushButton::clicked, this, &NetworkWidget::onExpandAll);
    connect(ui->edtFilter, &QLineEdit::textChanged, this, &NetworkWidget::onFilterChanged);

    // ── Auto-refresh adapters when the ComboBox dropdown is opened ──
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

    // ── الربط المباشر لثيم القائمة المنسدلة ──
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

    if (pid != 0 && pid != 4) {
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

void NetworkWidget::autoStart() {
    if (m_adapters.isEmpty()) return;
    int idx = NetworkMonitor::recommendedAdapterIndex(m_adapters);
    ui->cmbAdapter->setCurrentIndex(idx);
    onAdapterChanged(idx);
    m_monitor->startCapture();
}
void NetworkWidget::populateAdapterCombo() {
    // Save current selection so we can restore it after refresh
    QString prevIP;
    if (ui->cmbAdapter->currentIndex() >= 0)
        prevIP = ui->cmbAdapter->currentData().toString();

    m_adapters = NetworkMonitor::enumerateAdapters();
    ui->cmbAdapter->blockSignals(true);
    ui->cmbAdapter->clear();
    for (const auto &ai : m_adapters)
        ui->cmbAdapter->addItem(ai.description, ai.ip);
    ui->cmbAdapter->blockSignals(false);

    if (!m_adapters.isEmpty()) {
        // Try to restore previous selection
        int restoreIdx = -1;
        if (!prevIP.isEmpty()) {
            for (int i = 0; i < m_adapters.size(); ++i) {
                if (m_adapters[i].ip == prevIP) { restoreIdx = i; break; }
            }
        }
        if (restoreIdx < 0)
            restoreIdx = NetworkMonitor::recommendedAdapterIndex(m_adapters);
        ui->cmbAdapter->setCurrentIndex(restoreIdx);
        ui->lblStatus->setText("Auto-selected: " + m_adapters[restoreIdx].description);
    } else {
        ui->lblStatus->setText("No network adapters found.");
    }
}

// ── Event filter: refresh adapters when ComboBox dropdown is opened ──
bool NetworkWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->cmbAdapter && event->type() == QEvent::MouseButtonPress) {
        if (!m_capturing) {
            populateAdapterCombo();
        }
    }
    return QWidget::eventFilter(obj, event);
}

void NetworkWidget::onAdapterChanged(int idx) {
    if (idx >= 0 && idx < m_adapters.size())
        m_monitor->setAdapterIP(m_adapters[idx].ip);
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
    ui->lblRxVal->setText(NetworkTreeModel::formatSpeed(rxBps));
    ui->lblTxVal->setText(NetworkTreeModel::formatSpeed(txBps));
    emit speedUpdated(rxBps, txBps);
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



