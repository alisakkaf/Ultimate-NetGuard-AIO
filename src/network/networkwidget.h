/**
 * @file    networkwidget.h
 * @author  Ali Sakkaf
 */
#pragma once

#include <QWidget>
#include <QSortFilterProxyModel>
#include <QDialog>
#include <QEvent>
#include "networkmonitor.h"
#include "networkmodel.h"

namespace Ui { class NetworkWidget; }

// ── Professional Process Properties Dialog ──
class ProcessInfoDialog : public QDialog
{
    Q_OBJECT
public:
    ProcessInfoDialog(const QString &name, quint32 pid, const QString &path,
                      const QString &rx, const QString &tx, const QString &total, const QString &pkts,
                      const QIcon &icon, QWidget *parent = nullptr);
private slots:
    void onOpenLocation();
private:
    QString m_path;
};

class NetworkWidget : public QWidget
{
    Q_OBJECT
public:
    explicit NetworkWidget(QWidget *parent = nullptr);
    ~NetworkWidget() override;

    void autoStart();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

signals:
    void speedUpdated(quint64 rxBps, quint64 txBps);

private slots:
    void onAdapterChanged(int idx);
    void onStartStop();
    void onClearTable();
    void onExpandAll();
    void onFilterChanged(const QString &text);

    void onPacketsCapturedBatch(const QList<CapturedPacketInfo> &batch);
    void onSpeedUpdated(quint64 rxBps, quint64 txBps);
    void onCaptureError(const QString &msg);
    void onCaptureStarted();
    void onCaptureStopped();

    void onTreeViewContextMenu(const QPoint &pos);

private:
    void populateAdapterCombo();
    QString getExactProcessPath(quint32 pid, const QString &procName);
    QIcon getIconByPid(quint32 pid, const QString &procName);

    Ui::NetworkWidget       *ui = nullptr;
    NetworkMonitor          *m_monitor = nullptr;
    NetworkTreeModel        *m_model = nullptr;
    QSortFilterProxyModel   *m_proxy = nullptr;

    QList<AdapterInfo>      m_adapters;
    bool                    m_capturing = false;
    quint64                 m_totalPkts = 0;

    QHash<QString, QIcon>   m_iconCache;
};
