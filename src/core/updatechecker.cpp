/*
 =============================================================================
    Copyright (c) 2026  AliSakkaf  |  All Rights Reserved
 =============================================================================
*/
#include "updatechecker.h"
#include "version.h"
#include "stylemanager.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QDesktopServices>
#include <QUrl>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QApplication>
#include <QDebug>

static const char *UPDATE_URL      = "https://pastebin.com/raw/QHESkZXK";
static const char *DOWNLOAD_PAGE   = "https://alisakkaf.com/en/windows-software/ultimate-netguard-aio-network-monitor-firewall-manager";

// ============================================================================
//  UpdateChecker
// ============================================================================

UpdateChecker::UpdateChecker(QWidget *parentWindow, QObject *parent)
    : QObject(parent)
    , m_parentWindow(parentWindow)
    , m_nam(new QNetworkAccessManager(this))
{
    connect(m_nam, &QNetworkAccessManager::finished,
            this,  &UpdateChecker::onReplyFinished);
}

void UpdateChecker::checkNow()
{
    QNetworkRequest req((QUrl(UPDATE_URL)));
    req.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    m_nam->get(req);
}

void UpdateChecker::onReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "[UpdateChecker] Network error:" << reply->errorString();
        return;                                    // Fail silently
    }

    QString body = QString::fromUtf8(reply->readAll()).trimmed();
    if (body.isEmpty()) return;

    // ── Parse format ──
    //   1.0.1
    //   Changelog=>
    //   - Fix Bugs
    //   - New Feature
    QStringList lines = body.split('\n');
    if (lines.isEmpty()) return;

    QString remoteVersion = lines.first().trimmed();

    // Extract changelog (everything after "Changelog=>")
    QString changelog;
    bool inChangelog = false;
    for (int i = 1; i < lines.size(); ++i) {
        QString line = lines[i];
        if (!inChangelog) {
            if (line.trimmed().startsWith("Changelog=>", Qt::CaseInsensitive)) {
                inChangelog = true;
            }
        } else {
            changelog += line + "\n";
        }
    }
    changelog = changelog.trimmed();

    // ── Version comparison ──
    if (!isNewerVersion(remoteVersion, APP_VERSION_STR)) {
        qDebug() << "[UpdateChecker] Up to date. Local:" << APP_VERSION_STR
                 << " Remote:" << remoteVersion;
        return;
    }

    qDebug() << "[UpdateChecker] New version available:" << remoteVersion;

    // ── Show the update dialog on the GUI thread ──
    UpdateDialog *dlg = new UpdateDialog(remoteVersion, changelog, m_parentWindow);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->exec();
}

bool UpdateChecker::isNewerVersion(const QString &remote, const QString &local)
{
    QStringList rParts = remote.split('.');
    QStringList lParts = local.split('.');

    int maxLen = qMax(rParts.size(), lParts.size());
    for (int i = 0; i < maxLen; ++i) {
        int rNum = (i < rParts.size()) ? rParts[i].toInt() : 0;
        int lNum = (i < lParts.size()) ? lParts[i].toInt() : 0;

        if (rNum > lNum) return true;   // Remote is newer
        if (rNum < lNum) return false;  // Local is newer (!)
    }
    return false;                       // Exact same version
}

// ============================================================================
//  UpdateDialog — frameless, draggable, modern, theme-aware
// ============================================================================

UpdateDialog::UpdateDialog(const QString &remoteVersion,
                           const QString &changelog,
                           QWidget *parent)
    : QDialog(parent, Qt::FramelessWindowHint | Qt::Dialog)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedWidth(520);
    setMinimumHeight(320);
    setMaximumHeight(620);

    // ── Central container (rounded, painted in paintEvent) ──
    QWidget *card = new QWidget(this);
    card->setObjectName("updateCard");

    QVBoxLayout *outerLay = new QVBoxLayout(this);
    outerLay->setContentsMargins(16, 16, 16, 16);   // shadow space
    outerLay->addWidget(card);

    QVBoxLayout *cardLay = new QVBoxLayout(card);
    cardLay->setSpacing(0);
    cardLay->setContentsMargins(28, 22, 28, 28);

    // ── Title row + close button ──
    QHBoxLayout *titleRow = new QHBoxLayout;
    titleRow->setContentsMargins(0, 0, 0, 0);

    QLabel *icon = new QLabel;
    icon->setPixmap(QPixmap(":/icons/Ultimate_NetGuard_AIO.png").scaled(36, 36, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    icon->setFixedSize(36, 36);
    titleRow->addWidget(icon);
    titleRow->addSpacing(10);

    QLabel *title = new QLabel("Update Available");
    title->setObjectName("updateTitle");
    titleRow->addWidget(title, 1);

    QPushButton *btnClose = new QPushButton("\u2715");         // ✕
    btnClose->setObjectName("updateCloseBtn");
    btnClose->setFixedSize(32, 32);
    btnClose->setCursor(Qt::PointingHandCursor);
    connect(btnClose, &QPushButton::clicked, this, &QDialog::reject);
    titleRow->addWidget(btnClose);

    cardLay->addLayout(titleRow);
    cardLay->addSpacing(16);

    // ── Version badge ──
    QLabel *versionBadge = new QLabel(
        QString("  v%1  \u2192  v%2  ").arg(APP_VERSION_STR, remoteVersion));
    versionBadge->setObjectName("versionBadge");
    versionBadge->setAlignment(Qt::AlignCenter);
    cardLay->addWidget(versionBadge);
    cardLay->addSpacing(14);

    // ── Subtitle ──
    QLabel *subtitle = new QLabel(
        "A new version of Ultimate NetGuard AIO is available.\n"
        "This update may include new features, improvements, and bug fixes.");
    subtitle->setObjectName("updateSubtitle");
    subtitle->setWordWrap(true);
    cardLay->addWidget(subtitle);
    cardLay->addSpacing(14);

    // ── Changelog area (scrollable) ──
    if (!changelog.isEmpty()) {
        QLabel *changelogHeader = new QLabel("Changelog");
        changelogHeader->setObjectName("changelogHeader");
        cardLay->addWidget(changelogHeader);
        cardLay->addSpacing(6);

        QScrollArea *scroll = new QScrollArea;
        scroll->setObjectName("changelogScroll");
        scroll->setWidgetResizable(true);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scroll->setMinimumHeight(80);
        scroll->setMaximumHeight(200);

        QLabel *changelogLabel = new QLabel(changelog);
        changelogLabel->setObjectName("changelogText");
        changelogLabel->setWordWrap(true);
        changelogLabel->setTextFormat(Qt::PlainText);
        changelogLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        changelogLabel->setContentsMargins(12, 10, 12, 10);

        scroll->setWidget(changelogLabel);
        cardLay->addWidget(scroll);
        cardLay->addSpacing(14);
    }

    // ── Button row ──
    QHBoxLayout *btnRow = new QHBoxLayout;
    btnRow->setSpacing(12);

    QPushButton *btnSkip = new QPushButton("  Skip This Update  ");
    btnSkip->setObjectName("btnSkipUpdate");
    btnSkip->setCursor(Qt::PointingHandCursor);
    connect(btnSkip, &QPushButton::clicked, this, &QDialog::reject);

    QPushButton *btnUpdate = new QPushButton("  Update Now  ");
    btnUpdate->setObjectName("btnUpdateNow");
    btnUpdate->setCursor(Qt::PointingHandCursor);
    connect(btnUpdate, &QPushButton::clicked, this, [this](){
        QDesktopServices::openUrl(QUrl(DOWNLOAD_PAGE));
        reject();
    });

    btnRow->addStretch();
    btnRow->addWidget(btnSkip);
    btnRow->addWidget(btnUpdate);

    cardLay->addLayout(btnRow);

    // ── Shadow ──
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(32);
    shadow->setOffset(0, 6);
    shadow->setColor(QColor(0, 0, 0, 120));
    card->setGraphicsEffect(shadow);

    // ── Apply theme ──
    applyCurrentTheme();

    adjustSize();
}

void UpdateDialog::paintEvent(QPaintEvent *)
{
    // Transparent shell — the card widget handles its own background via stylesheet
}

void UpdateDialog::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragPos  = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

void UpdateDialog::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragPos);
        event->accept();
    }
}

void UpdateDialog::mouseReleaseEvent(QMouseEvent *)
{
    m_dragging = false;
}

void UpdateDialog::applyCurrentTheme()
{
    bool dark = StyleManager::instance().isDark();

    QString bgCard        = dark ? "#1E1E2E" : "#FFFFFF";
    QString borderCard    = dark ? "#2D2D3F" : "#E0E0E0";
    QString textPrimary   = dark ? "#E4E4E7" : "#18181B";
    QString textSecondary = dark ? "#A1A1AA" : "#52525B";
    QString accent        = "#3B82F6";
    QString accentHover   = "#2563EB";
    QString badgeBg       = dark ? "#1A2744" : "#DBEAFE";
    QString badgeText     = dark ? "#60A5FA" : "#1D4ED8";
    QString changelogBg   = dark ? "#16161E" : "#F4F4F5";
    QString changelogBdr  = dark ? "#27273A" : "#D4D4D8";
    QString scrollTrack   = dark ? "#16161E" : "#F4F4F5";
    QString scrollThumb   = dark ? "#3F3F5A" : "#A1A1AA";
    QString skipBg        = dark ? "#27273A" : "#F4F4F5";
    QString skipBgHover   = dark ? "#3F3F5A" : "#E4E4E7";
    QString skipText      = dark ? "#A1A1AA" : "#52525B";

    setStyleSheet(QString(R"(
        #updateCard {
            background: %1;
            border: 1px solid %2;
            border-radius: 16px;
        }
        #updateTitle {
            font-size: 17px;
            font-weight: 700;
            color: %3;
            background: transparent;
        }
        #updateCloseBtn {
            background: transparent;
            border: none;
            font-size: 16px;
            font-weight: bold;
            color: %4;
            border-radius: 16px;
        }
        #updateCloseBtn:hover {
            background: rgba(220, 38, 38, 0.15);
            color: #F87171;
        }
        #versionBadge {
            background: %5;
            color: %6;
            font-size: 14px;
            font-weight: 600;
            padding: 8px 18px;
            border-radius: 8px;
        }
        #updateSubtitle {
            font-size: 13px;
            color: %4;
            line-height: 1.6;
            background: transparent;
        }
        #changelogHeader {
            font-size: 13px;
            font-weight: 700;
            color: %3;
            background: transparent;
        }
        #changelogScroll {
            background: %7;
            border: 1px solid %8;
            border-radius: 8px;
        }
        #changelogText {
            font-size: 12px;
            color: %4;
            background: transparent;
        }
        QScrollBar:vertical {
            background: %9;
            width: 6px;
            border-radius: 3px;
        }
        QScrollBar::handle:vertical {
            background: %10;
            min-height: 30px;
            border-radius: 3px;
        }
        QScrollBar::add-line:vertical,
        QScrollBar::sub-line:vertical {
            height: 0px;
        }
        #btnSkipUpdate {
            background: %11;
            color: %13;
            border: none;
            border-radius: 8px;
            padding: 10px 22px;
            font-size: 13px;
            font-weight: 600;
        }
        #btnSkipUpdate:hover {
            background: %12;
        }
        #btnUpdateNow {
            background: %14;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 10px 22px;
            font-size: 13px;
            font-weight: 700;
        }
        #btnUpdateNow:hover {
            background: %15;
        }
    )")
    .arg(bgCard, borderCard, textPrimary, textSecondary,
         badgeBg, badgeText, changelogBg, changelogBdr,
         scrollTrack, scrollThumb, skipBg, skipBgHover,
         skipText, accent, accentHover));
}
