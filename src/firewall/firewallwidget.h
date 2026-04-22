/**
 * @file    firewallwidget.h
 * @author  Ali Sakkaf
 * @brief   UI interactions for Firewall. Supports Extended Selection and Shortcuts.
 */
#pragma once
#include <QWidget>
#include <QSortFilterProxyModel>
#include <QShortcut>
#include "firewallmanager.h"
#include "firewallmodel.h"

namespace Ui { class FirewallWidget; }

class FirewallWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FirewallWidget(QWidget *parent = nullptr);
    ~FirewallWidget() override;

protected:
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent     (QDropEvent      *e) override;

private slots:
    void onAddBlock();
    void onAddAllow();
    void onRemoveRule();
    void onExportRules();
    void onImportRules();
    void onWhitelistToggled(bool on);
    void onCoreFilter(const QString &text);

    // Mouse Interactions
    void onNetGuardContextMenu(const QPoint &pos);
    void onNetGuardClicked(const QModelIndex &idx);

    void onSysContextMenu(const QPoint &pos);
    void onSysClicked(const QModelIndex &idx);

    void onSysRefresh();
    void onSysToggle();
    void onSysDelete();
    void onSysFilter(const QString &text);

private:
    void blockOrAllowPath(const QString &path);
    void refreshBothTabs();
    void setupShortcuts();

    Ui::FirewallWidget      *ui = nullptr;
    FirewallManager         *m_mgr = nullptr;

    FirewallModel           *m_coreModel = nullptr;
    QSortFilterProxyModel   *m_coreProxy = nullptr;

    FirewallModel           *m_sysModel = nullptr;
    QSortFilterProxyModel   *m_sysProxy = nullptr;
};
