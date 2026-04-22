/**
 * @file    firewallmodel.cpp
 * @author  Ali Sakkaf
 * @brief   Implementation of the Flat Table Firewall Model.
 */
#include "firewallmodel.h"
#include <QColor>
#include <QFont>
#include <QFileInfo>
#include <QIcon>
#include <QApplication>
#include <QStyle>
#include "stylemanager.h"
#include "apptheme.h"

FirewallModel::FirewallModel(FirewallManager *mgr, bool netGuardOnly, QObject *parent)
    : QAbstractTableModel(parent), m_mgr(mgr), m_ngOnly(netGuardOnly)
{
    m_currentDir = NET_FW_RULE_DIR_IN; // Default to Inbound
    refresh();
}

void FirewallModel::setDirection(NET_FW_RULE_DIRECTION_ dir)
{
    m_currentDir = dir;
    refresh();
}

int FirewallModel::rowCount(const QModelIndex &) const
{
    return m_rules.size();
}

int FirewallModel::columnCount(const QModelIndex &) const
{
    return COL_COUNT;
}

QVariant FirewallModel::headerData(int s, Qt::Orientation o, int role) const
{
    if (o != Qt::Horizontal || role != Qt::DisplayRole) return {};
    static const char *H[COL_COUNT] = {
        "Application", "Rule Name", "Action", "Protocol", "Status", "Type"
    };
    return QString::fromUtf8(H[s]);
}

Qt::ItemFlags FirewallModel::flags(const QModelIndex &idx) const
{
    return idx.isValid() ? Qt::ItemIsEnabled | Qt::ItemIsSelectable : Qt::NoItemFlags;
}

QVariant FirewallModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid() || idx.row() >= m_rules.size()) return {};

    const FirewallRule &r = m_rules[idx.row()];
    int col = idx.column();

    // Hidden backend data roles
    if (role == Qt::UserRole) return r.name;
    if (role == Qt::UserRole + 1) return r.appPath;

    // Visual Display
    if (role == Qt::DisplayRole) {
        switch (col) {
        case COL_APP:    return r.appPath.isEmpty() ? "Any / System" : QFileInfo(r.appPath).fileName();
        case COL_NAME:   return r.name;
        case COL_ACTION: return r.action == NET_FW_ACTION_ALLOW ? QString::fromUtf8("✅ Allow") : QString::fromUtf8("🚫 Block");
        case COL_PROTO: {
            if (r.protocol == 256) return "ANY";
            if (r.protocol == 6)   return "TCP";
            if (r.protocol == 17)  return "UDP";
            return QString::number(r.protocol);
        }
        case COL_ENABLED: return r.enabled ? QString::fromUtf8("✔ ON") : QString::fromUtf8("❌ OFF");
        case COL_NETGUARD:return r.isNetGuard ? QString::fromUtf8("★ NG") : "OS";
        }
    }

    // Tooltip to show full path since we only show fileName in COL_APP
    if (role == Qt::ToolTipRole && col == COL_APP) {
        return r.appPath.isEmpty() ? "System Rule" : r.appPath;
    }

    // Application Icon resolution (Now perfectly in Column 0)
    if (role == Qt::DecorationRole && col == COL_APP) {
        if (!r.realAppPath.isEmpty()) {
            QFileInfo fi(r.realAppPath);
            if (fi.exists()) {
                return m_iconProvider.icon(fi);
            }
        }
        return QApplication::style()->standardIcon(QStyle::SP_ComputerIcon);
    }

    // Smart Colors depending on Dark/Light Theme
    if (role == Qt::ForegroundRole) {
        bool isDark = (StyleManager::instance().currentMode() == AppTheme::Dark);

        if (col == COL_ACTION) {
            if (r.action == NET_FW_ACTION_ALLOW)
                return isDark ? QColor("#3FB950") : QColor("#1A7F37");
            else
                return isDark ? QColor("#F85149") : QColor("#D1242F");
        }
        if (col == COL_ENABLED && !r.enabled) {
            return isDark ? QColor("#8B949E") : QColor("#6E7781");
        }
        if (col == COL_NETGUARD && r.isNetGuard) {
            return QColor("#F59E0B"); // Gold for NetGuard rules
        }
    }

    return {};
}

void FirewallModel::refresh()
{
    beginResetModel();

    QList<FirewallRule> allRules = m_mgr->listRules(m_ngOnly);
    m_rules.clear();

    // Filter purely by the active Direction Tab (Inbound or Outbound)
    for (const FirewallRule &rule : allRules) {
        if (rule.direction == m_currentDir) {
            m_rules.append(rule);
        }
    }

    endResetModel();
}
