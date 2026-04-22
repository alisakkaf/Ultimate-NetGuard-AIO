/*
 =============================================================================
    Copyright (c) 2026  AliSakkaf  |  All Rights Reserved
 =============================================================================
*/
#include "appinstaller.h"
#include "version.h"
#include "stylemanager.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QThread>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QMouseEvent>
#include <QScreen>
#include <QDebug>

// Windows APIs (project already links shell32, ole32, user32, advapi32)
#include <windows.h>
#include <tlhelp32.h>
#include <shlobj.h>
#include <objbase.h>

// ============================================================================
//  Path helpers
// ============================================================================

QString AppInstaller::installDir()
{
    // Use the official %ProgramFiles% which is typically "C:\Program Files"
    QString pf = qEnvironmentVariable("ProgramFiles", "C:\\Program Files");
    return pf + "\\NetGuard";
}

QString AppInstaller::installedExePath()
{
    return installDir() + "\\UltimateNetGuard.exe";
}

bool AppInstaller::isRunningFromInstallDir()
{
    QString current   = QDir::toNativeSeparators(QCoreApplication::applicationFilePath()).toLower();
    QString installed = QDir::toNativeSeparators(installedExePath()).toLower();
    return (current == installed);
}

bool AppInstaller::isAlreadyInstalled()
{
    return QFile::exists(installedExePath());
}

// ============================================================================
//  Version file  (installDir()/version.dat)
// ============================================================================

QString AppInstaller::readInstalledVersion()
{
    QFile f(installDir() + "\\version.dat");
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    return QString::fromUtf8(f.readAll()).trimmed();
}

void AppInstaller::writeVersionFile()
{
    QFile f(installDir() + "\\version.dat");
    if (f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        f.write(APP_VERSION_STR);
        f.close();
    }
    setFullPermissions(installDir() + "\\version.dat");
}

// ============================================================================
//  Permissions
// ============================================================================

void AppInstaller::setFullPermissions(const QString &path)
{
    QFile file(path);
    file.setPermissions(
        QFileDevice::ReadOwner  | QFileDevice::WriteOwner  | QFileDevice::ExeOwner  |
        QFileDevice::ReadUser   | QFileDevice::WriteUser   | QFileDevice::ExeUser   |
        QFileDevice::ReadGroup  | QFileDevice::WriteGroup  | QFileDevice::ExeGroup  |
        QFileDevice::ReadOther  | QFileDevice::WriteOther  | QFileDevice::ExeOther);
}

// ============================================================================
//  run()  — called from main() before MainWindow
// ============================================================================

bool AppInstaller::run()
{
    // ── Already in Program Files → just make sure version.dat is current ──
    if (isRunningFromInstallDir()) {
        writeVersionFile();
        return true;                   // continue normal startup
    }

    // ── Running from elsewhere ──
    if (isAlreadyInstalled()) {
        QString oldVer = readInstalledVersion();

        if (!oldVer.isEmpty() && oldVer == APP_VERSION_STR) {
            // Same version already installed → relaunch silently
            launchInstalledAndExit();
            return false;
        }

        // Different version → show Update dialog
        performUpdate(oldVer.isEmpty() ? "Unknown" : oldVer);
    } else {
        // First time → show Install dialog
        performFirstInstall();
    }

    launchInstalledAndExit();
    return false;
}

// ============================================================================
//  Kill other running instances of the exe
// ============================================================================

void AppInstaller::killOtherInstances()
{
    DWORD myPid = GetCurrentProcessId();
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);

    if (Process32FirstW(snap, &pe)) {
        do {
            if (pe.th32ProcessID == myPid) continue;

            QString name = QString::fromWCharArray(pe.szExeFile);
            if (name.compare("UltimateNetGuard.exe", Qt::CaseInsensitive) == 0) {
                HANDLE hProc = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE,
                                           FALSE, pe.th32ProcessID);
                if (hProc) {
                    TerminateProcess(hProc, 0);
                    WaitForSingleObject(hProc, 3000);
                    CloseHandle(hProc);
                }
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
}

// ============================================================================
//  Copy current exe → install dir
// ============================================================================

bool AppInstaller::copyExe()
{
    QString src = QCoreApplication::applicationFilePath();
    QString dst = installedExePath();

    // Remove old copy if it exists
    if (QFile::exists(dst)) {
        setFullPermissions(dst);
        QFile::remove(dst);
        QThread::msleep(300);          // brief pause for handle release
    }

    bool ok = QFile::copy(src, dst);
    if (ok) setFullPermissions(dst);
    return ok;
}

// ============================================================================
//  Desktop shortcut via IShellLink COM
// ============================================================================

bool AppInstaller::createDesktopShortcut()
{
    QString targetPath   = installedExePath();
    QString desktopPath  = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString shortcutPath = desktopPath + "\\NetGuard AIO.lnk";

    CoInitialize(nullptr);

    IShellLinkW *psl = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_IShellLinkW, reinterpret_cast<void**>(&psl));
    bool success = false;

    if (SUCCEEDED(hr) && psl) {
        std::wstring wTarget  = targetPath.toStdWString();
        std::wstring wWorkDir = QFileInfo(targetPath).absolutePath().toStdWString();
        std::wstring wDesc    = L"Ultimate NetGuard AIO - Network Monitor & Firewall Manager";

        psl->SetPath(wTarget.c_str());
        psl->SetWorkingDirectory(wWorkDir.c_str());
        psl->SetDescription(wDesc.c_str());
        psl->SetIconLocation(wTarget.c_str(), 0);

        IPersistFile *ppf = nullptr;
        hr = psl->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&ppf));
        if (SUCCEEDED(hr) && ppf) {
            hr = ppf->Save(shortcutPath.toStdWString().c_str(), TRUE);
            success = SUCCEEDED(hr);
            ppf->Release();
        }
        psl->Release();
    }

    CoUninitialize();
    return success;
}

// ============================================================================
//  Launch installed exe then exit current process
// ============================================================================

void AppInstaller::launchInstalledAndExit()
{
    QProcess::startDetached(installedExePath(), QStringList());
    // caller returns 0 from main()
}

// ============================================================================
//  First install flow
// ============================================================================

bool AppInstaller::performFirstInstall()
{
    InstallProgressDialog dlg("install", QString(), APP_VERSION_STR);
    dlg.show();
    QCoreApplication::processEvents();

    // 1
    dlg.setStep(1, 5, "Creating installation directory...");
    QDir().mkpath(installDir());
    QThread::msleep(400);
    QCoreApplication::processEvents();

    // 2
    dlg.setStep(2, 5, "Copying application files...");
    bool ok = copyExe();
    QThread::msleep(400);
    QCoreApplication::processEvents();

    if (!ok) {
        dlg.setFinished("Installation failed — could not copy files.");
        QThread::msleep(2500);
        return false;
    }

    // 3
    dlg.setStep(3, 5, "Setting file permissions...");
    setFullPermissions(installedExePath());
    QThread::msleep(350);
    QCoreApplication::processEvents();

    // 4
    dlg.setStep(4, 5, "Creating desktop shortcut...");
    createDesktopShortcut();
    QThread::msleep(350);
    QCoreApplication::processEvents();

    // 5
    dlg.setStep(5, 5, "Writing version data...");
    writeVersionFile();
    QThread::msleep(300);
    QCoreApplication::processEvents();

    dlg.setFinished("Installation complete!");
    QThread::msleep(900);
    QCoreApplication::processEvents();

    return true;
}

// ============================================================================
//  Update flow
// ============================================================================

bool AppInstaller::performUpdate(const QString &oldVersion)
{
    InstallProgressDialog dlg("update", oldVersion, APP_VERSION_STR);
    dlg.show();
    QCoreApplication::processEvents();

    // 1
    dlg.setStep(1, 6, "Closing previous version...");
    killOtherInstances();
    QThread::msleep(700);
    QCoreApplication::processEvents();

    // 2
    dlg.setStep(2, 6, "Removing old version...");
    setFullPermissions(installedExePath());
    QFile::remove(installedExePath());
    QThread::msleep(400);
    QCoreApplication::processEvents();

    // 3
    dlg.setStep(3, 6, QString("Installing version %1...").arg(APP_VERSION_STR));
    bool ok = copyExe();
    QThread::msleep(400);
    QCoreApplication::processEvents();

    if (!ok) {
        dlg.setFinished("Update failed — could not copy files.");
        QThread::msleep(2500);
        return false;
    }

    // 4
    dlg.setStep(4, 6, "Setting file permissions...");
    setFullPermissions(installedExePath());
    QThread::msleep(350);
    QCoreApplication::processEvents();

    // 5
    dlg.setStep(5, 6, "Updating shortcuts...");
    createDesktopShortcut();
    QThread::msleep(350);
    QCoreApplication::processEvents();

    // 6
    dlg.setStep(6, 6, "Finalizing update...");
    writeVersionFile();
    QThread::msleep(300);
    QCoreApplication::processEvents();

    dlg.setFinished("Update complete!");
    QThread::msleep(900);
    QCoreApplication::processEvents();

    return true;
}

// ============================================================================
//  InstallProgressDialog
// ============================================================================

InstallProgressDialog::InstallProgressDialog(const QString &mode,
                                             const QString &oldVersion,
                                             const QString &newVersion,
                                             QWidget *parent)
    : QDialog(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Dialog)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedWidth(460);

    // ── Card container ──
    QWidget *card = new QWidget(this);
    card->setObjectName("instCard");

    QVBoxLayout *outerLay = new QVBoxLayout(this);
    outerLay->setContentsMargins(14, 14, 14, 14);
    outerLay->addWidget(card);

    QVBoxLayout *lay = new QVBoxLayout(card);
    lay->setSpacing(12);
    lay->setContentsMargins(28, 24, 28, 28);

    // ── Icon + Title ──
    QHBoxLayout *titleRow = new QHBoxLayout;
    QLabel *icon = new QLabel;
    icon->setPixmap(QPixmap(":/icons/Ultimate_NetGuard_AIO.png")
                        .scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    icon->setFixedSize(32, 32);
    titleRow->addWidget(icon);
    titleRow->addSpacing(8);

    m_titleLabel = new QLabel(mode == "update" ? "Updating NetGuard AIO" : "Installing NetGuard AIO");
    m_titleLabel->setObjectName("instTitle");
    titleRow->addWidget(m_titleLabel, 1);
    lay->addLayout(titleRow);

    // ── Version info ──
    if (mode == "update" && !oldVersion.isEmpty()) {
        m_versionLabel = new QLabel(
            QString("v%1  \u2192  v%2").arg(oldVersion, newVersion));
    } else {
        m_versionLabel = new QLabel(QString("Version %1").arg(newVersion));
    }
    m_versionLabel->setObjectName("instVersion");
    m_versionLabel->setAlignment(Qt::AlignCenter);
    lay->addWidget(m_versionLabel);

    // ── Progress bar ──
    m_progressBar = new QProgressBar;
    m_progressBar->setObjectName("instProgress");
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(100);
    m_progressBar->setValue(0);
    m_progressBar->setFixedHeight(8);
    m_progressBar->setTextVisible(false);
    lay->addWidget(m_progressBar);

    // ── Status label ──
    m_statusLabel = new QLabel("Preparing...");
    m_statusLabel->setObjectName("instStatus");
    lay->addWidget(m_statusLabel);

    // ── Shadow ──
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(28);
    shadow->setOffset(0, 5);
    shadow->setColor(QColor(0, 0, 0, 100));
    card->setGraphicsEffect(shadow);

    applyTheme();
    adjustSize();

    // Center on screen
    if (QScreen *scr = QApplication::primaryScreen()) {
        QRect geo = scr->availableGeometry();
        move(geo.center() - rect().center());
    }
}

void InstallProgressDialog::setStep(int step, int total, const QString &statusText)
{
    int pct = static_cast<int>((step * 100.0) / total);
    m_progressBar->setValue(pct);
    m_statusLabel->setText(statusText);
    QCoreApplication::processEvents();
}

void InstallProgressDialog::setFinished(const QString &statusText)
{
    m_progressBar->setValue(100);
    m_statusLabel->setText(statusText);
    QCoreApplication::processEvents();
}

// ── Drag support ──
void InstallProgressDialog::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragPos  = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

void InstallProgressDialog::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragPos);
        event->accept();
    }
}

void InstallProgressDialog::mouseReleaseEvent(QMouseEvent *)
{
    m_dragging = false;
}

// ── Theme ──
void InstallProgressDialog::applyTheme()
{
    bool dark = StyleManager::instance().isDark();

    QString bgCard       = dark ? "#1E1E2E" : "#FFFFFF";
    QString borderCard   = dark ? "#2D2D3F" : "#E0E0E0";
    QString textPrimary  = dark ? "#E4E4E7" : "#18181B";
    QString textSec      = dark ? "#A1A1AA" : "#52525B";
    QString accent       = "#3B82F6";
    QString badgeBg      = dark ? "#1A2744" : "#DBEAFE";
    QString badgeText    = dark ? "#60A5FA" : "#1D4ED8";
    QString trackBg      = dark ? "#27273A" : "#E4E4E7";

    setStyleSheet(QString(R"(
        #instCard {
            background: %1;
            border: 1px solid %2;
            border-radius: 14px;
        }
        #instTitle {
            font-size: 16px;
            font-weight: 700;
            color: %3;
            background: transparent;
        }
        #instVersion {
            background: %5;
            color: %6;
            font-size: 13px;
            font-weight: 600;
            padding: 7px 16px;
            border-radius: 7px;
        }
        #instProgress {
            background: %7;
            border: none;
            border-radius: 4px;
        }
        #instProgress::chunk {
            background: %8;
            border-radius: 4px;
        }
        #instStatus {
            font-size: 12px;
            color: %4;
            background: transparent;
        }
    )")
    .arg(bgCard, borderCard, textPrimary, textSec,
         badgeBg, badgeText, trackBg, accent));
}
