/**
 * @file    firewallmanager.cpp
 * @author  Ali Sakkaf
 * @brief   Firewall Manager interacting with Windows COM API.
 * Includes Ultimate Lockdown Engine and strict Path Sanitization.
 */
#include "firewallmanager.h"
#include <QFileInfo>
#include <QProcess>
#include <QStringList>
#include <QDir>
#include <comdef.h>
#include <shobjidl.h>
#include <shlguid.h>

static const CLSID CLSID_NetFwRule_Compat =
    {0x2C5BC43E,0x3369,0x4C33,{0xAB,0x0C,0xBE,0x94,0x69,0x67,0x7A,0xF4}};

// Helper to convert QString to BSTR for COM interop
static BSTR Q2B(const QString &s)
{
    return SysAllocString(s.toStdWString().c_str());
}

FirewallManager::FirewallManager(QObject *parent) : QObject(parent) {}
FirewallManager::~FirewallManager() { shutdown(); }

bool FirewallManager::initialize()
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) { setError("CoInitializeEx failed."); return false; }
    m_comInitialized = true;

    hr = CoCreateInstance(__uuidof(NetFwPolicy2), nullptr, CLSCTX_INPROC_SERVER,
                          __uuidof(INetFwPolicy2), reinterpret_cast<void**>(&m_fwPolicy));
    if (FAILED(hr)) { setError("Failed to create INetFwPolicy2."); return false; }

    hr = m_fwPolicy->get_Rules(&m_fwRules);
    if (FAILED(hr)) { setError("Failed to get Rules collection."); return false; }

    return true;
}

void FirewallManager::shutdown()
{
    if (m_fwRules) { m_fwRules->Release(); m_fwRules = nullptr; }
    if (m_fwPolicy) { m_fwPolicy->Release(); m_fwPolicy = nullptr; }
    if (m_comInitialized) { CoUninitialize(); m_comInitialized = false; }
}

bool FirewallManager::blockApp(const QString &appPath)
{
    QString cleanPath = expandAndCleanPath(appPath);
    return addRuleInternal(cleanPath, makeRuleName(cleanPath, "BLOCK_OUT"), NET_FW_ACTION_BLOCK, NET_FW_RULE_DIR_OUT) &&
           addRuleInternal(cleanPath, makeRuleName(cleanPath, "BLOCK_IN"),  NET_FW_ACTION_BLOCK, NET_FW_RULE_DIR_IN);
}

bool FirewallManager::allowApp(const QString &appPath)
{
    QString cleanPath = expandAndCleanPath(appPath);
    return addRuleInternal(cleanPath, makeRuleName(cleanPath, "ALLOW_OUT"), NET_FW_ACTION_ALLOW, NET_FW_RULE_DIR_OUT) &&
           addRuleInternal(cleanPath, makeRuleName(cleanPath, "ALLOW_IN"),  NET_FW_ACTION_ALLOW, NET_FW_RULE_DIR_IN);
}

bool FirewallManager::removeAppRules(const QString &appPath)
{
    if (!m_fwRules) return false;
    bool success = true;

    QString cleanPath = expandAndCleanPath(appPath);
    QString base = "NetGuard_" + QFileInfo(cleanPath).completeBaseName();

    QString bOut = base + "_BLOCK_OUT";
    QString bIn  = base + "_BLOCK_IN";
    QString aOut = base + "_ALLOW_OUT";
    QString aIn  = base + "_ALLOW_IN";

    BSTR b;
    b = Q2B(bOut); m_fwRules->Remove(b); SysFreeString(b);
    b = Q2B(bIn);  m_fwRules->Remove(b); SysFreeString(b);
    b = Q2B(aOut); m_fwRules->Remove(b); SysFreeString(b);
    b = Q2B(aIn);  m_fwRules->Remove(b); SysFreeString(b);

    emit rulesChanged();
    return success;
}

bool FirewallManager::removeRuleByName(const QString &name)
{
    if (!m_fwRules) return false;
    BSTR b = Q2B(name);
    HRESULT hr = m_fwRules->Remove(b);
    SysFreeString(b);
    if (SUCCEEDED(hr)) emit rulesChanged();
    return SUCCEEDED(hr);
}

bool FirewallManager::toggleRule(const QString &name, bool enable)
{
    if (!m_fwRules) return false;
    BSTR bName = Q2B(name);
    INetFwRule *rule = nullptr;
    HRESULT hr = m_fwRules->Item(bName, &rule);
    SysFreeString(bName);

    if (SUCCEEDED(hr) && rule) {
        hr = rule->put_Enabled(enable ? VARIANT_TRUE : VARIANT_FALSE);
        rule->Release();
        if (SUCCEEDED(hr)) emit rulesChanged();
        return SUCCEEDED(hr);
    }
    return false;
}

// ============================================================================
// ─── Whitelist Lockdown Engine ──────────────────────────────────────────────
// ============================================================================
bool FirewallManager::setWhitelistMode(bool enabled)
{
    if (!m_fwPolicy) return false;

    // Force Firewall ON to ensure policies are respected
    netshExec({"advfirewall", "set", "allprofiles", "state", "on"});

    NET_FW_ACTION_ action = enabled ? NET_FW_ACTION_BLOCK : NET_FW_ACTION_ALLOW;
    bool success = true;

    NET_FW_PROFILE_TYPE2 profiles[] = {
        NET_FW_PROFILE2_DOMAIN,
        NET_FW_PROFILE2_PRIVATE,
        NET_FW_PROFILE2_PUBLIC
    };

    for (int i = 0; i < 3; ++i) {
        HRESULT hr = m_fwPolicy->put_DefaultOutboundAction(profiles[i], action);
        if (FAILED(hr)) success = false;
    }

    QString policy = enabled ? "blockinbound,blockoutbound" : "blockinbound,allowoutbound";
    netshExec({"advfirewall", "set", "allprofiles", "firewallpolicy", policy});

    if (enabled) {
        // Aggressive Disable: Turn off other Allow rules that bypass the block
        IUnknown *pUnk = nullptr;
        if (SUCCEEDED(m_fwRules->get__NewEnum(&pUnk))) {
            IEnumVARIANT *pEnum = nullptr;
            if (SUCCEEDED(pUnk->QueryInterface(IID_IEnumVARIANT, reinterpret_cast<void**>(&pEnum)))) {
                VARIANT var;
                VariantInit(&var);
                ULONG cFetched = 0;
                while (pEnum->Next(1, &var, &cFetched) == S_OK && cFetched == 1) {
                    INetFwRule *rule = nullptr;
                    if (SUCCEEDED(var.pdispVal->QueryInterface(__uuidof(INetFwRule), reinterpret_cast<void**>(&rule)))) {
                        NET_FW_RULE_DIRECTION_ dir;
                        NET_FW_ACTION_ act;
                        VARIANT_BOOL isEnabledVar;

                        rule->get_Direction(&dir);
                        rule->get_Action(&act);
                        rule->get_Enabled(&isEnabledVar);

                        if (dir == NET_FW_RULE_DIR_OUT && act == NET_FW_ACTION_ALLOW && isEnabledVar == VARIANT_TRUE) {
                            BSTR bName;
                            if (SUCCEEDED(rule->get_Name(&bName)) && bName != nullptr) {
                                QString rName = QString::fromWCharArray(bName);
                                SysFreeString(bName);

                                if (!rName.startsWith("NetGuard_")) {
                                    rule->put_Enabled(VARIANT_FALSE);
                                }
                            }
                        }
                        rule->Release();
                    }
                    VariantClear(&var);
                }
                pEnum->Release();
            }
            pUnk->Release();
        }

        // Network Survival Wires: Ensure core OS components are permitted
        QString sys32 = QDir::fromNativeSeparators(qgetenv("WINDIR")) + "\\System32\\svchost.exe";
        allowApp("System");
        allowApp(sys32);
    }

    if (success) {
        emit rulesChanged();
        return true;
    } else {
        setError("Failed to apply Lockdown mode to Windows Firewall profiles.");
        return false;
    }
}

bool FirewallManager::addToWhitelist(const QString &appPath) { return allowApp(appPath); }
bool FirewallManager::removeFromWhitelist(const QString &appPath) { return removeAppRules(appPath); }

bool FirewallManager::isBlocked(const QString &appPath) const
{
    QString cleanPath = expandAndCleanPath(appPath);
    QString nOut = makeRuleName(cleanPath, "BLOCK_OUT");
    if (!m_fwRules) return false;
    BSTR bName = Q2B(nOut);
    INetFwRule *rule = nullptr;
    HRESULT hr = m_fwRules->Item(bName, &rule);
    SysFreeString(bName);
    if (SUCCEEDED(hr) && rule) {
        rule->Release();
        return true;
    }
    return false;
}

// ============================================================================
// ─── Path & Rule Parsers ────────────────────────────────────────────────────
// ============================================================================

QString FirewallManager::resolveShortcut(const QString &path) const
{
    QString nativePath = QDir::toNativeSeparators(path);
    if (!nativePath.endsWith(".lnk", Qt::CaseInsensitive)) return nativePath;

    IShellLinkW *psl = nullptr;
    QString result = nativePath;

    HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW, reinterpret_cast<void**>(&psl));
    if (SUCCEEDED(hr)) {
        IPersistFile *ppf = nullptr;
        hr = psl->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&ppf));
        if (SUCCEEDED(hr)) {
            hr = ppf->Load(nativePath.toStdWString().c_str(), STGM_READ);
            if (SUCCEEDED(hr)) {
                psl->Resolve(nullptr, SLR_NO_UI | SLR_ANY_MATCH);
                wchar_t target[MAX_PATH];
                WIN32_FIND_DATAW wfd;
                hr = psl->GetPath(target, MAX_PATH, &wfd, SLGP_RAWPATH);
                if (SUCCEEDED(hr)) {
                    result = QString::fromWCharArray(target);
                }
            }
            ppf->Release();
        }
        psl->Release();
    }
    return expandAndCleanPath(result);
}

QString FirewallManager::expandAndCleanPath(const QString &path) const
{
    QString clean = path.trimmed();
    if (clean.startsWith("\"") && clean.endsWith("\"")) {
        clean = clean.mid(1, clean.length() - 2);
    }

    if (clean.contains("%")) {
        wchar_t expanded[MAX_PATH];
        if (ExpandEnvironmentStringsW(clean.toStdWString().c_str(), expanded, MAX_PATH)) {
            clean = QString::fromWCharArray(expanded);
        }
    }
    // EXTREMELY CRITICAL: Windows Firewall API only accepts native backslashes (\)
    return QDir::toNativeSeparators(clean);
}

QList<FirewallRule> FirewallManager::listRules(bool netGuardOnly) const
{
    QList<FirewallRule> list;
    if (!m_fwRules) return list;

    IUnknown *pUnk = nullptr;
    if (FAILED(m_fwRules->get__NewEnum(&pUnk))) return list;

    IEnumVARIANT *pEnum = nullptr;
    if (FAILED(pUnk->QueryInterface(IID_IEnumVARIANT, reinterpret_cast<void**>(&pEnum)))) {
        pUnk->Release();
        return list;
    }

    VARIANT var;
    VariantInit(&var);
    ULONG cFetched = 0;

    while (pEnum->Next(1, &var, &cFetched) == S_OK && cFetched == 1) {
        INetFwRule *rule = nullptr;
        if (SUCCEEDED(var.pdispVal->QueryInterface(__uuidof(INetFwRule), reinterpret_cast<void**>(&rule)))) {
            BSTR bName, bApp;
            rule->get_Name(&bName);
            rule->get_ApplicationName(&bApp);

            QString name = bName ? QString::fromWCharArray(bName) : "";
            QString app  = bApp ? QString::fromWCharArray(bApp) : "";

            if (bName) SysFreeString(bName);
            if (bApp) SysFreeString(bApp);

            bool isNG = name.startsWith("NetGuard_");
            if (!netGuardOnly || isNG) {
                FirewallRule r;
                r.name = name;
                r.appPath = app;
                r.realAppPath = expandAndCleanPath(app);
                r.isNetGuard = isNG;

                VARIANT_BOOL bEn;
                rule->get_Enabled(&bEn);
                r.enabled = (bEn == VARIANT_TRUE);

                NET_FW_RULE_DIRECTION_ dir;
                rule->get_Direction(&dir);
                r.direction = dir;

                NET_FW_ACTION_ act;
                rule->get_Action(&act);
                r.action = act;

                long proto;
                rule->get_Protocol(&proto);
                r.protocol = static_cast<NET_FW_IP_PROTOCOL_>(proto);

                list.append(r);
            }
            rule->Release();
        }
        VariantClear(&var);
    }

    pEnum->Release();
    pUnk->Release();
    return list;
}

bool FirewallManager::addRuleInternal(const QString &appPath, const QString &name,
                                      NET_FW_ACTION_ action, NET_FW_RULE_DIRECTION_ direction, bool enabled)
{
    if (!m_fwRules) return false;

    // Safety: ensure path is strictly native before sending to COM
    QString nativePath = expandAndCleanPath(appPath);

    removeRuleByName(name);

    INetFwRule *rule = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_NetFwRule_Compat, nullptr, CLSCTX_INPROC_SERVER,
                                  __uuidof(INetFwRule), reinterpret_cast<void**>(&rule));
    if (FAILED(hr)) return false;

    BSTR bN = Q2B(name);
    BSTR bD = Q2B("Managed by Ultimate NetGuard AIO");
    BSTR bG = Q2B("Ultimate NetGuard AIO");
    BSTR bA = nativePath.isEmpty() ? nullptr : Q2B(nativePath);

    rule->put_Name(bN);
    rule->put_Description(bD);
    rule->put_Grouping(bG);
    if (bA) rule->put_ApplicationName(bA);

    rule->put_Action(action);
    rule->put_Direction(direction);
    rule->put_Protocol(static_cast<NET_FW_IP_PROTOCOL_>(NET_FW_IP_PROTOCOL_ANY));
    rule->put_Enabled(enabled ? VARIANT_TRUE : VARIANT_FALSE);
    rule->put_Profiles(NET_FW_PROFILE2_ALL);

    hr = m_fwRules->Add(rule);

    SysFreeString(bN); SysFreeString(bD); SysFreeString(bG);
    if (bA) SysFreeString(bA);
    rule->Release();

    if (SUCCEEDED(hr)) emit rulesChanged();
    return SUCCEEDED(hr);
}

QString FirewallManager::makeRuleName(const QString &appPath, const QString &suffix) const
{
    return "NetGuard_" + QFileInfo(appPath).completeBaseName() + "_" + suffix;
}

void FirewallManager::setError(const QString &msg) {
    m_lastError = msg;
    emit error(msg);
}

// Netsh fallbacks for edge cases
bool FirewallManager::netshExec(const QStringList &args) {
    QProcess p; p.start("netsh",args); p.waitForFinished(8000);
    QString o = p.readAllStandardOutput() + p.readAllStandardError();
    return !o.contains("error",Qt::CaseInsensitive) && !o.contains("requires elevation", Qt::CaseInsensitive);
}
bool FirewallManager::netshBlock(const QString &a, const QString &n) {
    return netshExec({"advfirewall","firewall","add","rule","name="+n,"dir=out","action=block","program="+a,"enable=yes"}) &&
           netshExec({"advfirewall","firewall","add","rule","name="+n,"dir=in","action=block","program="+a,"enable=yes"});
}
bool FirewallManager::netshAllow(const QString &a, const QString &n) {
    return netshExec({"advfirewall","firewall","add","rule","name="+n,"dir=out","action=allow","program="+a,"enable=yes"}) &&
           netshExec({"advfirewall","firewall","add","rule","name="+n,"dir=in","action=allow","program="+a,"enable=yes"});
}
bool FirewallManager::netshDelete(const QString &n) {
    return netshExec({"advfirewall","firewall","delete","rule","name="+n});
}
