/**
 * @file    firewallmodel.h
 * @author  Ali Sakkaf
 * @brief   Flat Table Model for Firewall Rules with external direction filtering.
 */
#pragma once
#include <QAbstractTableModel>
#include <QFileIconProvider>
#include "firewallmanager.h"

class FirewallModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    // COL_APP is now first (index 0) so the Icon appears on the far left.
    enum Col { COL_APP=0, COL_NAME, COL_ACTION, COL_PROTO, COL_ENABLED, COL_NETGUARD, COL_COUNT };

    explicit FirewallModel(FirewallManager *mgr, bool netGuardOnly, QObject *parent=nullptr);

    int      rowCount   (const QModelIndex &p={}) const override;
    int      columnCount(const QModelIndex &p={}) const override;
    QVariant data       (const QModelIndex &idx, int role=Qt::DisplayRole) const override;
    QVariant headerData (int s, Qt::Orientation o, int role=Qt::DisplayRole) const override;
    Qt::ItemFlags flags (const QModelIndex &idx) const override;

    // Set the view direction (Inbound/Outbound) and refresh
    void setDirection(NET_FW_RULE_DIRECTION_ dir);
    void refresh();

private:
    FirewallManager       *m_mgr;
    bool                   m_ngOnly;
    NET_FW_RULE_DIRECTION_ m_currentDir; // Filter state
    QList<FirewallRule>    m_rules;      // Currently visible rules

    mutable QFileIconProvider m_iconProvider;
};
