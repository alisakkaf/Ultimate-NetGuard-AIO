/**
 * @file    firewallwidget.cpp
 * @author  Ali Sakkaf
 * @brief   UI interactions for Firewall. Features Tab Direction toggles and Hotkeys.
 */
#include "firewallwidget.h"
#include "ui_firewallwidget.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileDialog>
#include <QMessageBox>
#include <QMenu>
#include <QAction>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QPushButton>
#include <QTimer>
#include <QKeySequence>
#include <shlobj.h>

FirewallWidget::FirewallWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FirewallWidget)
{
    ui->setupUi(this);

    if (!IsUserAnAdmin()) {
        QMessageBox::critical(this, "Admin Required",
                              "Ultimate NetGuard must be run as Administrator!\nRules will fail to apply.");
    }

    setAcceptDrops(true);
    m_mgr = new FirewallManager(this);
    m_mgr->initialize();

    // ── Tab A: NetGuard Core ──────────────────────────────────────────────────
    m_coreModel = new FirewallModel(m_mgr, true, this);
    m_coreProxy = new QSortFilterProxyModel(this);
    m_coreProxy->setSourceModel(m_coreModel);
    m_coreProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_coreProxy->setFilterKeyColumn(-1);

    ui->tableCoreFW->setModel(m_coreProxy);
    ui->tableCoreFW->setContextMenuPolicy(Qt::CustomContextMenu);

    // Dynamic Width & Movable columns
    ui->tableCoreFW->horizontalHeader()->setSectionsMovable(true);
    ui->tableCoreFW->verticalHeader()->setSectionsMovable(true);
    ui->tableCoreFW->verticalHeader()->setSectionResizeMode(QHeaderView::Interactive);

    ui->tableCoreFW->horizontalHeader()->setStretchLastSection(true);
    ui->tableCoreFW->horizontalHeader()->setSectionResizeMode(FirewallModel::COL_APP, QHeaderView::Stretch);
    ui->tableCoreFW->horizontalHeader()->setSectionResizeMode(FirewallModel::COL_NAME, QHeaderView::Stretch);
    ui->tableCoreFW->horizontalHeader()->setSectionResizeMode(FirewallModel::COL_ACTION, QHeaderView::ResizeToContents);
    ui->tableCoreFW->horizontalHeader()->setSectionResizeMode(FirewallModel::COL_PROTO, QHeaderView::ResizeToContents);

    // Direction Toggles for Core
    connect(ui->btnCoreInbound, &QPushButton::toggled, this, [this](bool checked){
        if(checked) m_coreModel->setDirection(NET_FW_RULE_DIR_IN);
    });
    connect(ui->btnCoreOutbound, &QPushButton::toggled, this, [this](bool checked){
        if(checked) m_coreModel->setDirection(NET_FW_RULE_DIR_OUT);
    });

    connect(ui->btnBlock,  &QPushButton::clicked, this, &FirewallWidget::onAddBlock);
    connect(ui->btnAllow,  &QPushButton::clicked, this, &FirewallWidget::onAddAllow);
    connect(ui->btnRemove, &QPushButton::clicked, this, &FirewallWidget::onRemoveRule);
    connect(ui->btnExport, &QPushButton::clicked, this, &FirewallWidget::onExportRules);
    connect(ui->btnImport, &QPushButton::clicked, this, &FirewallWidget::onImportRules);
    connect(ui->edtCoreFilter, &QLineEdit::textChanged, this, &FirewallWidget::onCoreFilter);

    connect(ui->tableCoreFW, &QTableView::customContextMenuRequested, this, &FirewallWidget::onNetGuardContextMenu);
    connect(ui->tableCoreFW, &QTableView::doubleClicked, this, &FirewallWidget::onNetGuardClicked);

    // ── Tab B: System Rules ───────────────────────────────────────────────────
    m_sysModel = new FirewallModel(m_mgr, false, this);
    m_sysProxy = new QSortFilterProxyModel(this);
    m_sysProxy->setSourceModel(m_sysModel);
    m_sysProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_sysProxy->setFilterKeyColumn(-1);

    ui->tableSystemFW->setModel(m_sysProxy);
    ui->tableSystemFW->setContextMenuPolicy(Qt::CustomContextMenu);

    ui->tableSystemFW->horizontalHeader()->setSectionsMovable(true);
    ui->tableSystemFW->verticalHeader()->setSectionsMovable(true);
    ui->tableSystemFW->verticalHeader()->setSectionResizeMode(QHeaderView::Interactive);

    ui->tableSystemFW->horizontalHeader()->setStretchLastSection(true);
    ui->tableSystemFW->horizontalHeader()->setSectionResizeMode(FirewallModel::COL_APP, QHeaderView::Stretch);
    ui->tableSystemFW->horizontalHeader()->setSectionResizeMode(FirewallModel::COL_NAME, QHeaderView::Stretch);

    // Direction Toggles for System
    connect(ui->btnSysInbound, &QPushButton::toggled, this, [this](bool checked){
        if(checked) m_sysModel->setDirection(NET_FW_RULE_DIR_IN);
    });
    connect(ui->btnSysOutbound, &QPushButton::toggled, this, [this](bool checked){
        if(checked) m_sysModel->setDirection(NET_FW_RULE_DIR_OUT);
    });

    connect(ui->btnSysRefresh, &QPushButton::clicked, this, &FirewallWidget::onSysRefresh);
    connect(ui->btnSysToggle,  &QPushButton::clicked, this, &FirewallWidget::onSysToggle);
    connect(ui->dangerBtn,     &QPushButton::clicked, this, &FirewallWidget::onSysDelete);
    connect(ui->edtSysFilter,  &QLineEdit::textChanged, this, &FirewallWidget::onSysFilter);

    connect(ui->tableSystemFW, &QTableView::customContextMenuRequested, this, &FirewallWidget::onSysContextMenu);
    connect(ui->tableSystemFW, &QTableView::doubleClicked, this, &FirewallWidget::onSysClicked);

    connect(ui->chkWhitelist, &QCheckBox::toggled, this, &FirewallWidget::onWhitelistToggled);

    setupShortcuts();
}

FirewallWidget::~FirewallWidget()
{
    delete ui;
}

// ── HOTKEYS (Ctrl+A & Delete) ──
void FirewallWidget::setupShortcuts()
{
    QShortcut *delCore = new QShortcut(QKeySequence::Delete, ui->tableCoreFW);
    connect(delCore, &QShortcut::activated, this, &FirewallWidget::onRemoveRule);

    QShortcut *selAllCore = new QShortcut(QKeySequence::SelectAll, ui->tableCoreFW);
    connect(selAllCore, &QShortcut::activated, ui->tableCoreFW, &QTableView::selectAll);

    QShortcut *delSys = new QShortcut(QKeySequence::Delete, ui->tableSystemFW);
    connect(delSys, &QShortcut::activated, this, &FirewallWidget::onSysDelete);

    QShortcut *selAllSys = new QShortcut(QKeySequence::SelectAll, ui->tableSystemFW);
    connect(selAllSys, &QShortcut::activated, ui->tableSystemFW, &QTableView::selectAll);
}

void FirewallWidget::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasUrls()) e->acceptProposedAction();
}

void FirewallWidget::dropEvent(QDropEvent *e)
{
    for (const QUrl &url : e->mimeData()->urls()) {
        QString rawPath = url.toLocalFile();
        QString realPath = m_mgr->resolveShortcut(rawPath); // Crucial shortcut resolution

        QFileInfo fi(realPath);
        if (fi.suffix().toLower() == "exe" || realPath.contains("System32", Qt::CaseInsensitive)) {
            blockOrAllowPath(realPath);
        }
    }
}

void FirewallWidget::blockOrAllowPath(const QString &path)
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("New Rule");
    msgBox.setText(QString("Target: %1\nWhat action to apply?").arg(path));
    QPushButton *btnBlock = msgBox.addButton("🚫 Block", QMessageBox::RejectRole);
    QPushButton *btnAllow = msgBox.addButton("✅ Allow", QMessageBox::AcceptRole);
    msgBox.addButton("Cancel", QMessageBox::NoRole);
    msgBox.exec();

    if (msgBox.clickedButton() == btnBlock) m_mgr->blockApp(path);
    else if (msgBox.clickedButton() == btnAllow) m_mgr->allowApp(path);
    refreshBothTabs();
}

void FirewallWidget::onAddBlock()
{
    // Now accepting shortcuts (.lnk) via QFileDialog
    QString p = QFileDialog::getOpenFileName(this, "Select App to Block", "", "Programs (*.exe *.lnk)");
    if (!p.isEmpty()) {
        QString realPath = m_mgr->resolveShortcut(p);
        if (m_mgr->blockApp(realPath)) refreshBothTabs();
        else QMessageBox::warning(this, "Error", "Failed to add block rule.");
    }
}

void FirewallWidget::onAddAllow()
{
    // Now accepting shortcuts (.lnk) via QFileDialog
    QString p = QFileDialog::getOpenFileName(this, "Select App to Allow", "", "Programs (*.exe *.lnk)");
    if (!p.isEmpty()) {
        QString realPath = m_mgr->resolveShortcut(p);
        if (m_mgr->allowApp(realPath)) refreshBothTabs();
        else QMessageBox::warning(this, "Error", "Failed to add allow rule.");
    }
}

void FirewallWidget::onRemoveRule()
{
    QModelIndexList selected = ui->tableCoreFW->selectionModel()->selectedRows();
    if(selected.isEmpty()) return;

    for (const QModelIndex &idx : selected) {
        QModelIndex srcIdx = m_coreProxy->mapToSource(idx);
        QString ruleName = m_coreModel->data(srcIdx, Qt::UserRole).toString();
        if (!ruleName.isEmpty()) m_mgr->removeRuleByName(ruleName);
    }
    refreshBothTabs();
}

void FirewallWidget::refreshBothTabs()
{
    m_coreModel->refresh();
    m_sysModel->refresh();
}

void FirewallWidget::onExportRules()
{
    QString file = QFileDialog::getSaveFileName(this, "Export Rules", "", "JSON Files (*.json)");
    if(file.isEmpty()) return;

    QJsonArray arr;
    QList<FirewallRule> rules = m_mgr->listRules(true);

    for (const FirewallRule &r : rules) {
        QJsonObject o;
        o["name"]    = r.name;
        o["appPath"] = r.appPath;
        o["action"]  = (r.action == NET_FW_ACTION_ALLOW) ? "ALLOW" : "BLOCK";
        o["dir"]     = (r.direction == NET_FW_RULE_DIR_IN) ? "IN" : "OUT";
        o["enabled"] = r.enabled;
        arr.append(o);
    }

    QJsonDocument doc(arr);
    QFile f(file);
    if(f.open(QIODevice::WriteOnly)) {
        f.write(doc.toJson());
        f.close();
        QMessageBox::information(this, "Success", "Rules exported successfully.");
    }
}

void FirewallWidget::onImportRules()
{
    QString file = QFileDialog::getOpenFileName(this, "Import Rules", "", "JSON Files (*.json)");
    if(file.isEmpty()) return;

    QFile f(file);
    if(!f.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if(!doc.isArray()) return;

    QJsonArray arr = doc.array();
    for(int i=0; i<arr.size(); ++i) {
        QJsonObject o = arr[i].toObject();
        QString appPath = o["appPath"].toString();

        if(o["action"].toString() == "ALLOW") m_mgr->allowApp(appPath);
        else m_mgr->blockApp(appPath);
    }
    refreshBothTabs();
    QMessageBox::information(this, "Success", "Rules imported successfully.");
}

void FirewallWidget::onWhitelistToggled(bool on)
{
    if (on) {
        if(QMessageBox::warning(this, "Lockdown Warning",
                                 "Whitelist Mode will block ALL outbound traffic.\nOnly explicitly allowed apps (and core OS services) will connect.\nProceed?",
                                 QMessageBox::Yes|QMessageBox::No) != QMessageBox::Yes) {
            ui->chkWhitelist->blockSignals(true);
            ui->chkWhitelist->setChecked(false);
            ui->chkWhitelist->blockSignals(false);
            return;
        }
    }
    m_mgr->setWhitelistMode(on);
    refreshBothTabs();
}

void FirewallWidget::onNetGuardContextMenu(const QPoint &pos)
{
    QModelIndex proxyIdx = ui->tableCoreFW->indexAt(pos);
    if (!proxyIdx.isValid()) return;

    QModelIndex idx = m_coreProxy->mapToSource(proxyIdx);
    QString name = m_coreModel->data(idx, Qt::UserRole).toString();

    QMenu m(this);
    QAction *aTog = m.addAction("Toggle Enable/Disable");
    QAction *aDel = m.addAction("Delete Rule");

    connect(aTog, &QAction::triggered, [this, idx, name](){
        bool isEnabled = m_coreModel->data(idx.siblingAtColumn(FirewallModel::COL_ENABLED)).toString().contains("ON");
        m_mgr->toggleRule(name, !isEnabled);
        refreshBothTabs();
    });
    connect(aDel, &QAction::triggered, [this, name](){
        m_mgr->removeRuleByName(name);
        refreshBothTabs();
    });
    m.exec(ui->tableCoreFW->viewport()->mapToGlobal(pos));
}

void FirewallWidget::onNetGuardClicked(const QModelIndex &proxyIdx)
{
    QModelIndex idx = m_coreProxy->mapToSource(proxyIdx);
    if (!idx.isValid()) return;

    QString name = m_coreModel->data(idx, Qt::UserRole).toString();
    bool isEnabled = m_coreModel->data(idx.siblingAtColumn(FirewallModel::COL_ENABLED)).toString().contains("ON");
    m_mgr->toggleRule(name, !isEnabled);
    refreshBothTabs();
}

void FirewallWidget::onCoreFilter(const QString &text) {
    m_coreProxy->setFilterFixedString(text);
}

void FirewallWidget::onSysRefresh() {
    m_sysModel->refresh();
}

void FirewallWidget::onSysToggle()
{
    QModelIndexList selected = ui->tableSystemFW->selectionModel()->selectedRows();
    if(selected.isEmpty()) return;

    for(const QModelIndex &idx : selected) {
        QModelIndex srcIdx = m_sysProxy->mapToSource(idx);
        QString name = m_sysModel->data(srcIdx, Qt::UserRole).toString();
        bool isEnabled = m_sysModel->data(srcIdx.siblingAtColumn(FirewallModel::COL_ENABLED)).toString().contains("ON");
        m_mgr->toggleRule(name, !isEnabled);
    }
    onSysRefresh();
}

void FirewallWidget::onSysDelete()
{
    QModelIndexList selected = ui->tableSystemFW->selectionModel()->selectedRows();
    if(selected.isEmpty()) return;

    if (QMessageBox::question(this, "Delete", QString("Delete %1 system rule(s)?").arg(selected.size()), QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes) {
        for(const QModelIndex &idx : selected) {
            QModelIndex srcIdx = m_sysProxy->mapToSource(idx);
            QString name = m_sysModel->data(srcIdx, Qt::UserRole).toString();
            m_mgr->removeRuleByName(name);
        }
        onSysRefresh();
    }
}

void FirewallWidget::onSysFilter(const QString &text) {
    m_sysProxy->setFilterFixedString(text);
}

void FirewallWidget::onSysContextMenu(const QPoint &pos)
{
    QModelIndex proxyIdx = ui->tableSystemFW->indexAt(pos);
    if (!proxyIdx.isValid()) return;

    QModelIndex idx = m_sysProxy->mapToSource(proxyIdx);
    QString name = m_sysModel->data(idx, Qt::UserRole).toString();

    QMenu m(this);
    QAction *aTog = m.addAction("Toggle Rule");
    QAction *aDel = m.addAction("Delete Rule");

    connect(aTog, &QAction::triggered, [this, idx, name](){
        bool isEnabled = m_sysModel->data(idx.siblingAtColumn(FirewallModel::COL_ENABLED)).toString().contains("ON");
        m_mgr->toggleRule(name, !isEnabled);
        onSysRefresh();
    });
    connect(aDel, &QAction::triggered, [this, name](){
        m_mgr->removeRuleByName(name);
        onSysRefresh();
    });
    m.exec(ui->tableSystemFW->viewport()->mapToGlobal(pos));
}

void FirewallWidget::onSysClicked(const QModelIndex &proxyIdx)
{
    QModelIndex idx = m_sysProxy->mapToSource(proxyIdx);
    if (!idx.isValid()) return;

    QString name = m_sysModel->data(idx, Qt::UserRole).toString();
    bool isEnabled = m_sysModel->data(idx.siblingAtColumn(FirewallModel::COL_ENABLED)).toString().contains("ON");
    m_mgr->toggleRule(name, !isEnabled);
    onSysRefresh();
}
