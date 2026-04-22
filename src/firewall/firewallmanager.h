/**
 * @file    firewallmanager.h
 * @author  Ali Sakkaf
 * @brief   Firewall Manager interacting with Windows COM API.
 */
#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#  define NOMINMAX
#endif
#include <windows.h>
#include <oleauto.h>
#include <netfw.h>

#ifndef NET_FW_IP_PROTOCOL_ANY
#  define NET_FW_IP_PROTOCOL_ANY 256
#endif

#include <QObject>
#include <QString>
#include <QList>

// Firewall rule data structure
struct FirewallRule {
    QString                name;
    QString                appPath;
    QString                realAppPath; // Clean native path used for icon extraction
    bool                   enabled   = false;
    NET_FW_RULE_DIRECTION_ direction = NET_FW_RULE_DIR_OUT;
    NET_FW_ACTION_         action    = NET_FW_ACTION_ALLOW;
    NET_FW_IP_PROTOCOL_    protocol  = static_cast<NET_FW_IP_PROTOCOL_>(NET_FW_IP_PROTOCOL_ANY);
    bool                   isNetGuard= false;
};

class FirewallManager : public QObject
{
    Q_OBJECT
public:
    explicit FirewallManager(QObject *parent = nullptr);
    ~FirewallManager() override;

    bool initialize();
    void shutdown();

    // Core rule modifications
    bool blockApp           (const QString &appPath);
    bool allowApp           (const QString &appPath);
    bool removeAppRules     (const QString &appPath);
    bool removeRuleByName   (const QString &name);
    bool toggleRule         (const QString &name, bool enable);

    // Whitelist & Lockdown Mode
    bool setWhitelistMode   (bool enabled);
    bool addToWhitelist     (const QString &appPath);
    bool removeFromWhitelist(const QString &appPath);
    bool isBlocked          (const QString &appPath) const;

    // Rule retrieval and parsing
    QList<FirewallRule> listRules(bool netGuardOnly) const;
    QString resolveShortcut(const QString &path) const;

    QString lastError() const { return m_lastError; }

signals:
    void rulesChanged();
    void error(const QString &msg);

private:
    bool    addRuleInternal(const QString &appPath, const QString &name,
                         NET_FW_ACTION_         action,
                         NET_FW_RULE_DIRECTION_ direction, bool enabled = true);
    QString makeRuleName   (const QString &appPath, const QString &suffix) const;
    void    setError       (const QString &msg);

    // Core function to sanitize paths for Windows Firewall API
    QString expandAndCleanPath(const QString &path) const;

    // Fallback functions using netsh
    bool    netshExec  (const QStringList &args);
    bool    netshBlock (const QString &appPath, const QString &name);
    bool    netshAllow (const QString &appPath, const QString &name);
    bool    netshDelete(const QString &name);

    INetFwPolicy2 *m_fwPolicy      = nullptr;
    INetFwRules   *m_fwRules       = nullptr;
    bool           m_comInitialized= false;
    QString        m_lastError;
};
