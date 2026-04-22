/**
 * @file    updatechecker.h
 * @author  Ali Sakkaf
 * @brief   Silent background update checker. Compares the local version (from version.h)
 *          against a remote pastebin endpoint, parses the changelog, and shows a
 *          beautiful frameless QDialog when a newer version is available.
 */
#pragma once

#include <QObject>
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QPoint>

class QNetworkAccessManager;
class QNetworkReply;

// ============================================================================
// UpdateChecker — fires once on app start, runs in background
// ============================================================================
class UpdateChecker : public QObject
{
    Q_OBJECT
public:
    explicit UpdateChecker(QWidget *parentWindow, QObject *parent = nullptr);
    void checkNow();

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    static bool isNewerVersion(const QString &remote, const QString &local);

    QWidget                *m_parentWindow = nullptr;
    QNetworkAccessManager  *m_nam          = nullptr;
};

// ============================================================================
// UpdateDialog — frameless, modern, theme-aware popup
// ============================================================================
class UpdateDialog : public QDialog
{
    Q_OBJECT
public:
    UpdateDialog(const QString &remoteVersion,
                 const QString &changelog,
                 QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void applyCurrentTheme();

    QPoint  m_dragPos;
    bool    m_dragging = false;
};
