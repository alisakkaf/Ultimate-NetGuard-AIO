/**
 * @file    appinstaller.h
 * @author  Ali Sakkaf
 * @brief   Self-installer and auto-updater.
 *          - Copies the running exe to Program Files\NetGuard on first launch.
 *          - Creates a professional desktop shortcut.
 *          - On subsequent launches from outside the install dir, detects and
 *            replaces the old version with a visual progress dialog.
 */
#pragma once

#include <QDialog>
#include <QLabel>
#include <QProgressBar>
#include <QPoint>
#include <QString>

// ============================================================================
// AppInstaller — static utility class
// ============================================================================
class AppInstaller
{
public:
    /// Canonical install directory  (e.g. C:\Program Files\NetGuard)
    static QString installDir();

    /// Full path to the installed exe
    static QString installedExePath();

    /// Is the *current* process running from the install directory?
    static bool isRunningFromInstallDir();

    /// Does an installed copy already exist?
    static bool isAlreadyInstalled();

    /// Read the version string stored in version.dat next to the installed exe
    static QString readInstalledVersion();

    /**
     * @brief  Entry point called from main() BEFORE MainWindow.
     * @return true  → continue normal startup (we are in install dir)
     *         false → app was relaunched from install dir; caller must return 0
     */
    static bool run();

private:
    static bool performFirstInstall();
    static bool performUpdate(const QString &oldVersion);
    static void killOtherInstances();
    static bool copyExe();
    static bool createDesktopShortcut();
    static void writeVersionFile();
    static void setFullPermissions(const QString &path);
    static void launchInstalledAndExit();
};

// ============================================================================
// InstallProgressDialog — frameless, theme-aware progress overlay
// ============================================================================
class InstallProgressDialog : public QDialog
{
    Q_OBJECT
public:
    /**
     * @param mode        "install" or "update"
     * @param oldVersion  previous installed version (empty for first install)
     * @param newVersion  version being installed (APP_VERSION_STR)
     */
    InstallProgressDialog(const QString &mode,
                          const QString &oldVersion,
                          const QString &newVersion,
                          QWidget *parent = nullptr);

    void setStep(int step, int total, const QString &statusText);
    void setFinished(const QString &statusText);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event)  override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void applyTheme();

    QLabel       *m_titleLabel   = nullptr;
    QLabel       *m_versionLabel = nullptr;
    QLabel       *m_statusLabel  = nullptr;
    QProgressBar *m_progressBar  = nullptr;

    QPoint m_dragPos;
    bool   m_dragging = false;
};
