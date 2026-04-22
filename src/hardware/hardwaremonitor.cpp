/**
 * @file    hardwaremonitor.cpp
 * @author  Ali Sakkaf
 * @brief   Hardware monitoring backend with fixed Variant string parsing and robust WMI fallbacks.
 */
#include "hardwaremonitor.h"
#include <QThread>
#include <QMutexLocker>
#include <QDebug>
#include <QString>
#include <algorithm>

// COM init for WMI
#include <iphlpapi.h>
#include <objbase.h>
#include <comdef.h>

HardwareMonitor::HardwareMonitor(QObject *parent) : QThread(parent)
{
    m_stop.storeRelaxed(0);
}

HardwareMonitor::~HardwareMonitor()
{
    stopMonitor();
    wait(4000);
    shutdownPdh();
    shutdownWmi();
}

void HardwareMonitor::startMonitor()
{
    if (isRunning()) return;
    m_stop.storeRelaxed(0);
    start(QThread::LowPriority);
}

void HardwareMonitor::stopMonitor()
{
    m_stop.storeRelaxed(1);
}

HardwareSnapshot HardwareMonitor::lastSnapshot() const
{
    QMutexLocker lk(&m_snapMutex);
    return m_lastSnap;
}

// ────────────────────────────────────────────────────────────────────────────
void HardwareMonitor::run()
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool comOk = SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE;

    if (comOk) { initWmi(); }
    initPdh();

    while (m_stop.loadRelaxed() == 0)
    {
        HardwareSnapshot snap;
        collectPdh(snap);
        collectRam(snap);
        collectNetwork(snap);

        if (m_wmiReady) {
            snap.gpuLoadPct = queryGpuLoad();
            collectTemperatures(snap);
        }

        { QMutexLocker lk(&m_snapMutex); m_lastSnap = snap; }
        emit snapshotReady(snap);

        QThread::sleep(1);
    }

    shutdownPdh();
    shutdownWmi();
    if (comOk) CoUninitialize();
}

// ── WMI Setup ───────────────────────────────────────────────────────────────
bool HardwareMonitor::initWmi()
{
    HRESULT hr = CoCreateInstance(__uuidof(WbemLocator), nullptr,
                                  CLSCTX_INPROC_SERVER,
                                  __uuidof(IWbemLocator),
                                  reinterpret_cast<void**>(&m_wmiLocator));
    if (FAILED(hr)) return false;

    // Helper to connect to namespaces
    auto connectNamespace = [&](const wchar_t* ns, IWbemServices** svc) {
        BSTR bNs = SysAllocString(ns);
        HRESULT h = m_wmiLocator->ConnectServer(bNs, nullptr, nullptr, nullptr, 0, nullptr, nullptr, svc);
        SysFreeString(bNs);
        if (SUCCEEDED(h) && *svc) {
            CoSetProxyBlanket(*svc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr,
                              RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);
        }
    };

    connectNamespace(L"ROOT\\CIMV2", &m_wmiServices);
    connectNamespace(L"ROOT\\WMI", &m_wmiRootWmi);
    connectNamespace(L"ROOT\\Microsoft\\Windows\\Storage", &m_wmiStorage);

    m_wmiReady = (m_wmiServices != nullptr);
    return m_wmiReady;
}

void HardwareMonitor::shutdownWmi()
{
    if (m_wmiServices) { m_wmiServices->Release(); m_wmiServices = nullptr; }
    if (m_wmiRootWmi)  { m_wmiRootWmi->Release();  m_wmiRootWmi = nullptr; }
    if (m_wmiStorage)  { m_wmiStorage->Release();  m_wmiStorage = nullptr; }
    if (m_wmiLocator)  { m_wmiLocator->Release();  m_wmiLocator = nullptr; }
    m_wmiReady = false;
}

// Fixed function to parse ALL types including Text (VT_BSTR) which was causing Type 8 errors
double HardwareMonitor::getVariantAsDouble(const VARIANT &v)
{
    switch (v.vt) {
    case VT_I4:   return static_cast<double>(v.lVal);
    case VT_UI4:  return static_cast<double>(v.ulVal);
    case VT_R4:   return static_cast<double>(v.fltVal);
    case VT_R8:   return v.dblVal;
    case VT_I2:   return static_cast<double>(v.iVal);
    case VT_UI2:  return static_cast<double>(v.uiVal);
    case VT_I8:   return static_cast<double>(v.llVal);
    case VT_UI8:  return static_cast<double>(v.ullVal);
    case VT_BSTR: // Handle Type 8 (String) correctly
        if (v.bstrVal) {
            QString str = QString::fromWCharArray(v.bstrVal);
            return str.toDouble();
        }
        return 0.0;
    default:
        return 0.0;
    }
}

// ── WMI Temperature Fetcher ─────────────────────────────────────────────────
void HardwareMonitor::collectTemperatures(HardwareSnapshot &snap)
{
    snap.cpuTempC  = 0.0;
    snap.mbTempC   = 0.0;
    snap.diskTempC = 0.0;
    snap.gpuTempC  = 0.0;

    // 1. Fetch Disk Temperature (Fixed Invalid Query)
    if (m_wmiStorage) {
        IEnumWbemClassObject* enm = nullptr;
        // Using StorageReliabilityCounter which accurately holds the temperature in Windows 10/11
        BSTR q = SysAllocString(L"SELECT Temperature FROM MSFT_StorageReliabilityCounter");
        BSTR wql = SysAllocString(L"WQL");
        if (SUCCEEDED(m_wmiStorage->ExecQuery(wql, q, WBEM_FLAG_FORWARD_ONLY, nullptr, &enm)) && enm) {
            IWbemClassObject* obj = nullptr;
            ULONG ret = 0;
            double maxDiskTemp = 0.0;
            while (enm->Next(WBEM_INFINITE, 1, &obj, &ret) == S_OK && obj) {
                VARIANT v; VariantInit(&v);
                if (SUCCEEDED(obj->Get(L"Temperature", 0, &v, nullptr, nullptr))) {
                    double t = getVariantAsDouble(v);
                    if (t > 0.0 && t < 150.0 && t > maxDiskTemp) {
                        maxDiskTemp = t;
                    }
                }
                VariantClear(&v);
                obj->Release();
            }
            if (maxDiskTemp > 0.0) snap.diskTempC = maxDiskTemp;
            enm->Release();
        }
        SysFreeString(q); SysFreeString(wql);
    }

    // 2. Fetch CPU Temperature via ACPI
    if (m_wmiRootWmi) {
        IEnumWbemClassObject* enm = nullptr;
        BSTR q = SysAllocString(L"SELECT CurrentTemperature FROM MSAcpi_ThermalZoneTemperature");
        BSTR wql = SysAllocString(L"WQL");

        // This fails with 0x8004100c (NOT SUPPORTED) on many modern motherboards
        if (SUCCEEDED(m_wmiRootWmi->ExecQuery(wql, q, WBEM_FLAG_FORWARD_ONLY, nullptr, &enm)) && enm) {
            IWbemClassObject* obj = nullptr;
            ULONG ret = 0;
            std::vector<double> temps;
            while (enm->Next(WBEM_INFINITE, 1, &obj, &ret) == S_OK && obj) {
                VARIANT v; VariantInit(&v);
                if (SUCCEEDED(obj->Get(L"CurrentTemperature", 0, &v, nullptr, nullptr))) {
                    double raw = getVariantAsDouble(v);
                    if (raw > 2000.0) {
                        double t = (raw - 2732.0) / 10.0; // Convert 10th Kelvin to Celsius
                        if (t > 0.0 && t < 150.0) temps.push_back(t);
                    }
                }
                VariantClear(&v);
                obj->Release();
            }
            enm->Release();

            if (!temps.empty()) {
                std::sort(temps.begin(), temps.end());
                snap.cpuTempC = temps.back();
                snap.mbTempC  = temps.front();
            }
        }
        SysFreeString(q); SysFreeString(wql);
    }

    // 3. Fallbacks for CPU Temperature if ACPI is blocked by BIOS
    if (snap.cpuTempC <= 0.0 && m_wmiServices) {

        // Fallback A: Win32_TemperatureProbe
        IEnumWbemClassObject* enmProbe = nullptr;
        BSTR qProbe = SysAllocString(L"SELECT CurrentReading FROM Win32_TemperatureProbe");
        BSTR wql = SysAllocString(L"WQL");
        if (SUCCEEDED(m_wmiServices->ExecQuery(wql, qProbe, WBEM_FLAG_FORWARD_ONLY, nullptr, &enmProbe)) && enmProbe) {
            IWbemClassObject* obj = nullptr;
            ULONG ret = 0;
            if (enmProbe->Next(WBEM_INFINITE, 1, &obj, &ret) == S_OK && obj) {
                VARIANT v; VariantInit(&v);
                if (SUCCEEDED(obj->Get(L"CurrentReading", 0, &v, nullptr, nullptr))) {
                    double t = getVariantAsDouble(v);
                    if (t > 0.0 && t < 150.0) snap.cpuTempC = t;
                }
                VariantClear(&v);
                obj->Release();
            }
            enmProbe->Release();
        }
        SysFreeString(qProbe);

        // Fallback B: PerfFormattedData Counters
        if (snap.cpuTempC <= 0.0) {
            IEnumWbemClassObject* enmPerf = nullptr;
            BSTR qPerf = SysAllocString(L"SELECT Temperature FROM Win32_PerfFormattedData_Counters_ThermalZoneInformation");
            if (SUCCEEDED(m_wmiServices->ExecQuery(wql, qPerf, WBEM_FLAG_FORWARD_ONLY, nullptr, &enmPerf)) && enmPerf) {
                IWbemClassObject* obj = nullptr;
                ULONG ret = 0;
                if (enmPerf->Next(WBEM_INFINITE, 1, &obj, &ret) == S_OK && obj) {
                    VARIANT v; VariantInit(&v);
                    if (SUCCEEDED(obj->Get(L"Temperature", 0, &v, nullptr, nullptr))) {
                        double raw = getVariantAsDouble(v);
                        if (raw > 2000.0) snap.cpuTempC = (raw - 2732.0) / 10.0;
                        else if (raw > 0.0 && raw < 150.0) snap.cpuTempC = raw;
                    }
                    VariantClear(&v);
                    obj->Release();
                }
                enmPerf->Release();
            }
            SysFreeString(qPerf);
        }
        SysFreeString(wql);
    }
}

double HardwareMonitor::queryGpuLoad()
{
    double load = 0.0;
    if (m_wmiServices) {
        BSTR q = SysAllocString(L"SELECT UtilizationPercentage FROM Win32_PerfFormattedData_GPUPerformanceCounters_GPUEngine WHERE Name LIKE '%engtype_3D%'");
        BSTR wql = SysAllocString(L"WQL");
        IEnumWbemClassObject* enm = nullptr;

        if (SUCCEEDED(m_wmiServices->ExecQuery(wql, q, WBEM_FLAG_FORWARD_ONLY, nullptr, &enm)) && enm) {
            IWbemClassObject* obj = nullptr;
            ULONG ret = 0;
            double totalLoad = 0.0;
            int count = 0;

            while (enm->Next(WBEM_INFINITE, 1, &obj, &ret) == S_OK && obj) {
                VARIANT v; VariantInit(&v);
                if (SUCCEEDED(obj->Get(L"UtilizationPercentage", 0, &v, nullptr, nullptr))) {
                    // Type 8 Error fixed: getVariantAsDouble now securely parses strings
                    totalLoad += getVariantAsDouble(v);
                    count++;
                }
                VariantClear(&v);
                obj->Release();
            }
            if (count > 0) load = totalLoad;
            enm->Release();
        }
        SysFreeString(q); SysFreeString(wql);
    }
    return load;
}

// ── PDH & Core ──────────────────────────────────────────────────────────────
bool HardwareMonitor::initPdh()
{
    if (PdhOpenQuery(nullptr, 0, &m_pdhQuery) != ERROR_SUCCESS) return false;
    PdhAddCounterW(m_pdhQuery, L"\\Processor(_Total)\\% Processor Time", 0, &m_cpuCounter);
    PdhAddCounterW(m_pdhQuery, L"\\PhysicalDisk(_Total)\\% Disk Time", 0, &m_diskCounter);
    PdhCollectQueryData(m_pdhQuery);
    m_pdhReady = true;
    return true;
}

void HardwareMonitor::shutdownPdh()
{
    if (m_pdhQuery) { PdhCloseQuery(m_pdhQuery); m_pdhQuery = nullptr; }
    m_pdhReady = false;
}

void HardwareMonitor::collectPdh(HardwareSnapshot &snap)
{
    if (!m_pdhReady) return;
    PdhCollectQueryData(m_pdhQuery);
    PDH_FMT_COUNTERVALUE val;
    if (PdhGetFormattedCounterValue(m_cpuCounter, PDH_FMT_DOUBLE, nullptr, &val) == ERROR_SUCCESS)
        snap.cpuLoadPct = val.doubleValue;
    if (PdhGetFormattedCounterValue(m_diskCounter, PDH_FMT_DOUBLE, nullptr, &val) == ERROR_SUCCESS)
        snap.diskLoadPct = val.doubleValue;
}

void HardwareMonitor::collectRam(HardwareSnapshot &snap)
{
    MEMORYSTATUSEX ms; ms.dwLength = sizeof(ms);
    if (!GlobalMemoryStatusEx(&ms)) return;
    snap.ramTotalMB = ms.ullTotalPhys / (1024.0 * 1024.0);
    snap.ramUsedMB  = (ms.ullTotalPhys - ms.ullAvailPhys) / (1024.0 * 1024.0);
    snap.ramLoadPct = ms.dwMemoryLoad;
}

void HardwareMonitor::collectNetwork(HardwareSnapshot &snap)
{
    DWORD sz = 0;
    GetIfTable(nullptr, &sz, FALSE);
    if (!sz) return;
    QByteArray buf(static_cast<int>(sz), '\0');
    auto *t = reinterpret_cast<MIB_IFTABLE*>(buf.data());
    if (GetIfTable(t, &sz, FALSE) != NO_ERROR) return;

    quint64 rx = 0, tx = 0;
    for (DWORD i = 0; i < t->dwNumEntries; i++) {
        const MIB_IFROW &r = t->table[i];
        if (r.dwType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
        rx += r.dwInOctets;
        tx += r.dwOutOctets;
    }
    snap.netRxBps = (rx > m_prevRx) ? (rx - m_prevRx) : 0;
    snap.netTxBps = (tx > m_prevTx) ? (tx - m_prevTx) : 0;
    m_prevRx = rx; m_prevTx = tx;
}
