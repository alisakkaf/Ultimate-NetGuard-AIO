/**
 * @file    networkmonitor.h
 * @brief   Raw-socket packet capture thread with background caching and true hardware speed.
 * @author  Ali Sakkaf
 */
#pragma once

#include <QThread>
#include <QStringList>
#include <QMutex>
#include <QTimer>
#include <QList>
#include <QHash>
#include <atomic>
#include <chrono>
#include "packetparser.h"

struct AdapterInfo {
    QString ip;
    QString ipv6; // ── [عربي] دعم الـ IPv6 ──
    QString description;
    bool    hasGateway = false;
    quint32 index = 0;
};

struct CapturedPacketInfo {
    ParsedPacket pkt;
    quint32 pid;
    QString proc;
    bool isService;
};
Q_DECLARE_METATYPE(QList<CapturedPacketInfo>)

class NetworkMonitor : public QThread
{
    Q_OBJECT
public:
    explicit NetworkMonitor(QObject *parent = nullptr);
    ~NetworkMonitor() override;

    static QList<AdapterInfo> enumerateAdapters();
    static int recommendedAdapterIndex(const QList<AdapterInfo> &list);

    void setAdapterIP(const QString &ip);
    void startCapture();
    void stopCapture();

    bool    isCapturing()  const { return m_captureRunning.load(); }

signals:
    void packetsCapturedBatch(const QList<CapturedPacketInfo> &batch);
    void speedUpdated(quint64 rxBps, quint64 txBps);
    void captureError(const QString &msg);
    void captureStarted();
    void captureStopped();

protected:
    void run() override;

private:
    void refreshLocalIPs();
    void updateCaches();
    QString getServiceNameFromPid(quint32 pid) const;

    QString m_adapterIP;
    QString m_adapterIPv6; // ── [عربي] متغير لتخزين الـ IPv6 ──
    quint32 m_adapterIndex = 0;

    std::atomic<bool> m_stopFlag{false};
    std::atomic<bool> m_captureRunning{false};

    QTimer *m_speedTimer = nullptr;
    QStringList m_localIPs;

    quint64 m_sysRxLast = 0;
    quint64 m_sysTxLast = 0;
    bool m_firstTick = true;
    std::chrono::steady_clock::time_point m_lastTick;

    QMutex m_cacheMutex;
    QHash<QString, quint32> m_tcpCache;
    QHash<QString, quint32> m_udpCache;
    QHash<quint16, quint32> m_tcpListenCache;
    QHash<quint16, quint32> m_udpAnyCache;
    QHash<quint32, QString> m_procCache;
};
