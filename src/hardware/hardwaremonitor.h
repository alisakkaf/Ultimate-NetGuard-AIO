/**
 * @file    hardwaremonitor.h
 * @brief   WMI + PDH hardware monitoring thread.
 * Features robust WMI connections for CPU, Disk, and Load fetching.
 */
#pragma once

#include <QThread>
#include <QMutex>
#include <QAtomicInt>
#include <QMetaType>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#  define NOMINMAX
#endif
#include <windows.h>
#include <pdh.h>
#include <wbemidl.h>

// ─── HardwareSnapshot ────────────────────────────────────────────────────────
struct HardwareSnapshot
{
    double  cpuLoadPct  = 0.0;
    double  cpuTempC    = 0.0;
    double  ramUsedMB   = 0.0;
    double  ramTotalMB  = 0.0;
    double  ramLoadPct  = 0.0;
    double  gpuLoadPct  = 0.0;
    double  gpuTempC    = 0.0;
    double  diskLoadPct = 0.0;
    double  diskTempC   = 0.0;
    double  mbTempC     = 0.0;
    quint64 netRxBps    = 0;
    quint64 netTxBps    = 0;
};
Q_DECLARE_METATYPE(HardwareSnapshot)

// ─── HardwareMonitor ─────────────────────────────────────────────────────────
class HardwareMonitor : public QThread
{
    Q_OBJECT
public:
    explicit HardwareMonitor(QObject *parent = nullptr);
    ~HardwareMonitor() override;

    void startMonitor();
    void stopMonitor();
    HardwareSnapshot lastSnapshot() const;

signals:
    void snapshotReady(const HardwareSnapshot &snap);

protected:
    void run() override;

private:
    bool   initWmi();
    void   shutdownWmi();

    // Core Fetchers
    double queryGpuLoad();
    void   collectTemperatures(HardwareSnapshot &snap);

    bool   initPdh();
    void   shutdownPdh();
    void   collectPdh(HardwareSnapshot &snap);
    void   collectRam(HardwareSnapshot &snap);
    void   collectNetwork(HardwareSnapshot &snap);

    // Helper to safely extract variant values
    double getVariantAsDouble(const VARIANT &v);

    QAtomicInt       m_stop;
    mutable QMutex   m_snapMutex;
    HardwareSnapshot m_lastSnap;

    // WMI Services for different namespaces
    IWbemLocator    *m_wmiLocator   = nullptr;
    IWbemServices   *m_wmiServices  = nullptr; // ROOT\CIMV2
    IWbemServices   *m_wmiRootWmi   = nullptr; // ROOT\WMI
    IWbemServices   *m_wmiStorage   = nullptr; // ROOT\Microsoft\Windows\Storage
    bool             m_wmiReady     = false;

    // PDH Counters
    PDH_HQUERY       m_pdhQuery    = nullptr;
    PDH_HCOUNTER     m_cpuCounter  = nullptr;
    PDH_HCOUNTER     m_diskCounter = nullptr;
    bool             m_pdhReady    = false;

    quint64          m_prevRx = 0, m_prevTx = 0;
};
