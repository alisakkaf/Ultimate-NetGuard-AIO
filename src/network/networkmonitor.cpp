/**
 * @file    networkmonitor.cpp
 * @brief   Thread-safe capture with dedicated caching thread and exact process/service mapping.
 * Includes Smart Auto-Detect & Dual-Socket IPv4/IPv6 Capture Engine!
 * @author  Ali Sakkaf
 */
#ifndef _WIN32_WINNT
#  define _WIN32_WINNT 0x0601
#endif

#include "networkmonitor.h"
#include <QMutexLocker>
#include <QDateTime>
#include <thread>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <mstcpip.h>
#include <netioapi.h>
#include <psapi.h>

QList<AdapterInfo> NetworkMonitor::enumerateAdapters()
{
    QList<AdapterInfo> result;
    ULONG sz = 20000;
    QByteArray buf(static_cast<int>(sz), '\0');
    auto *addrs = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buf.data());
    ULONG flags = GAA_FLAG_INCLUDE_GATEWAYS | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;

    if (GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, addrs, &sz) != NO_ERROR)
        return result;

    for (auto *a = addrs; a; a = a->Next) {
        if (a->OperStatus != IfOperStatusUp || a->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
        bool gw = (a->FirstGatewayAddress != nullptr);

        QString ipv4, ipv6;
        for (auto *ua = a->FirstUnicastAddress; ua; ua = ua->Next) {
            SOCKADDR *sa = ua->Address.lpSockaddr;
            if (sa->sa_family == AF_INET) {
                char ipBuf[INET_ADDRSTRLEN] = {};
                inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in*>(sa)->sin_addr, ipBuf, sizeof(ipBuf));
                ipv4 = QString::fromLatin1(ipBuf);
            } else if (sa->sa_family == AF_INET6) {
                char ipBuf[INET6_ADDRSTRLEN] = {};
                inet_ntop(AF_INET6, &reinterpret_cast<sockaddr_in6*>(sa)->sin6_addr, ipBuf, sizeof(ipBuf));
                ipv6 = QString::fromLatin1(ipBuf);
            }
        }

        if (!ipv4.isEmpty()) {
            AdapterInfo ai;
            ai.ip = ipv4;
            ai.ipv6 = ipv6; // ── [عربي] التقاط الـ IPv6 للكرت إن وجد ──
            ai.hasGateway = gw;
            ai.index = a->IfIndex;
            QString fn = a->FriendlyName ? QString::fromWCharArray(a->FriendlyName) : ai.ip;
            ai.description = fn + "  [" + ai.ip + "]" + (gw ? "  ★" : "");
            result.append(ai);
        }
    }
    return result;
}

// ── [عربي] الدالة الذكية لاختيار كرت الشبكة التلقائي (Smart Auto-Detect) ──
int NetworkMonitor::recommendedAdapterIndex(const QList<AdapterInfo> &list)
{
    if (list.isEmpty()) return 0;

    // 1. ── الذكاء الاصطناعي: نسأل الويندوز عن مسار الإنترنت الفعلي ──
    DWORD bestIfIndex = 0;
    if (GetBestInterface(inet_addr("8.8.8.8"), &bestIfIndex) == NO_ERROR) {
        for (int i = 0; i < list.size(); ++i) {
            if (list[i].index == bestIfIndex) {
                return i;
            }
        }
    }

    // 2. ── الخطة البديلة (Fallback): استبعاد الكروت الوهمية ──
    for (int i = 0; i < list.size(); ++i) {
        QString desc = list[i].description.toLower();
        if (list[i].hasGateway &&
            !desc.contains("vmware") &&
            !desc.contains("virtual") &&
            !desc.contains("radmin") &&
            !desc.contains("pseudo") &&
            !desc.contains("zerotier"))
        {
            return i;
        }
    }

    // 3. ── الملاذ الأخير ──
    for (int i = 0; i < list.size(); ++i) {
        if (list[i].hasGateway) return i;
    }

    return 0;
}

QString NetworkMonitor::getServiceNameFromPid(quint32 pid) const
{
    SC_HANDLE scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE);
    if (!scm) return QString();

    DWORD bytesNeeded = 0, servicesReturned = 0, resumeHandle = 0;
    EnumServicesStatusExW(scm, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL,
                          nullptr, 0, &bytesNeeded, &servicesReturned, &resumeHandle, nullptr);

    QByteArray buffer(bytesNeeded, 0);
    LPENUM_SERVICE_STATUS_PROCESSW services = reinterpret_cast<LPENUM_SERVICE_STATUS_PROCESSW>(buffer.data());

    if (EnumServicesStatusExW(scm, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_STATE_ALL,
                              reinterpret_cast<LPBYTE>(services), bytesNeeded, &bytesNeeded,
                              &servicesReturned, &resumeHandle, nullptr)) {
        for (DWORD i = 0; i < servicesReturned; ++i) {
            if (services[i].ServiceStatusProcess.dwProcessId == pid) {
                QString displayName = QString::fromWCharArray(services[i].lpDisplayName);
                CloseServiceHandle(scm);
                return displayName;
            }
        }
    }
    CloseServiceHandle(scm);
    return QString();
}

NetworkMonitor::NetworkMonitor(QObject *parent) : QThread(parent)
{
    m_speedTimer = new QTimer(this);
    m_speedTimer->setInterval(1000);
    connect(m_speedTimer, &QTimer::timeout, this, [this]() {
        quint64 sysRx = 0, sysTx = 0;

        if (m_adapterIndex != 0) {
            MIB_IF_ROW2 row;
            ZeroMemory(&row, sizeof(row));
            row.InterfaceIndex = m_adapterIndex;
            if (GetIfEntry2(&row) == NO_ERROR) {
                sysRx = row.InOctets;
                sysTx = row.OutOctets;
            }
        }

        auto now = std::chrono::steady_clock::now();
        double dt = std::chrono::duration<double>(now - m_lastTick).count();
        if (dt < 0.001) dt = 1.0;
        m_lastTick = now;

        quint64 rxD = (sysRx >= m_sysRxLast) ? (sysRx - m_sysRxLast) : 0;
        quint64 txD = (sysTx >= m_sysTxLast) ? (sysTx - m_sysTxLast) : 0;

        m_sysRxLast = sysRx;
        m_sysTxLast = sysTx;

        if (m_firstTick) { rxD = 0; txD = 0; m_firstTick = false; }
        emit speedUpdated(static_cast<quint64>(rxD / dt), static_cast<quint64>(txD / dt));
    });
}

NetworkMonitor::~NetworkMonitor()
{
    m_stopFlag.store(true);
    if (!wait(2000)) terminate();
}

void NetworkMonitor::setAdapterIP(const QString &ip)
{
    m_adapterIP = ip;
    m_adapterIPv6.clear();
    refreshLocalIPs();
    m_adapterIndex = 0;
    QList<AdapterInfo> adps = enumerateAdapters();
    for (const auto &ai : adps) {
        if (ai.ip == ip) {
            m_adapterIndex = ai.index;
            m_adapterIPv6 = ai.ipv6; // حفظ الـ IPv6
            break;
        }
    }
}

void NetworkMonitor::startCapture()
{
    if (isRunning()) return;
    m_stopFlag.store(false);
    m_firstTick = true;
    m_lastTick = std::chrono::steady_clock::now();
    m_speedTimer->start();
    start(QThread::HighPriority);
}

void NetworkMonitor::stopCapture()
{
    m_stopFlag.store(true);
    m_speedTimer->stop();
}

void NetworkMonitor::run()
{
    m_captureRunning.store(true);
    emit captureStarted();

    if (m_adapterIP.isEmpty()) {
        emit captureError("No adapter IP set.");
        m_captureRunning.store(false); emit captureStopped(); return;
    }

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa)) {
        emit captureError("WSAStartup failed.");
        m_captureRunning.store(false); emit captureStopped(); return;
    }

    // ── [عربي] فتح مقبسين (Sockets) لالتقاط شبكة IPv4 و IPv6 في نفس الوقت ──
    SOCKET sock4 = WSASocket(AF_INET, SOCK_RAW, IPPROTO_IP, nullptr, 0, WSA_FLAG_OVERLAPPED);
    SOCKET sock6 = INVALID_SOCKET;

    if (sock4 == INVALID_SOCKET) {
        emit captureError("WSASocket failed — run as Administrator.");
        WSACleanup(); m_captureRunning.store(false); emit captureStopped(); return;
    }

    int rcvBufSize = 32 * 1024 * 1024;
    setsockopt(sock4, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&rcvBufSize), sizeof(rcvBufSize));
    sockaddr_in sa4 = {}; sa4.sin_family = AF_INET; sa4.sin_addr.s_addr = inet_addr(m_adapterIP.toLatin1().constData());

    if (bind(sock4, reinterpret_cast<sockaddr*>(&sa4), sizeof(sa4)) == SOCKET_ERROR) {
        emit captureError("bind() IPv4 failed.");
        closesocket(sock4); WSACleanup(); m_captureRunning.store(false); emit captureStopped(); return;
    }

    DWORD dw = 0, on = RCVALL_ON;
    WSAIoctl(sock4, SIO_RCVALL, &on, sizeof(on), nullptr, 0, &dw, nullptr, nullptr);

    // ── [عربي] إعداد مسار الـ IPv6 ──
    if (!m_adapterIPv6.isEmpty()) {
        sock6 = WSASocket(AF_INET6, SOCK_RAW, IPPROTO_IPV6, nullptr, 0, WSA_FLAG_OVERLAPPED);
        if (sock6 != INVALID_SOCKET) {
            setsockopt(sock6, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&rcvBufSize), sizeof(rcvBufSize));
            sockaddr_in6 sa6 = {};
            sa6.sin6_family = AF_INET6;
            inet_pton(AF_INET6, m_adapterIPv6.toLatin1().constData(), &sa6.sin6_addr);
            if (bind(sock6, reinterpret_cast<sockaddr*>(&sa6), sizeof(sa6)) != SOCKET_ERROR) {
                WSAIoctl(sock6, SIO_RCVALL, &on, sizeof(on), nullptr, 0, &dw, nullptr, nullptr);
            } else {
                closesocket(sock6); sock6 = INVALID_SOCKET;
            }
        }
    }

    QByteArray buf(1024 * 1024, '\0');
    QList<CapturedPacketInfo> batch;
    batch.reserve(5000);
    qint64 lastBatchTime = QDateTime::currentMSecsSinceEpoch();

    std::atomic<bool> cacheStop{false};
    std::thread cacheThread([this, &cacheStop]() {
        while (!cacheStop.load()) {
            updateCaches();
            std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        }
    });

    while (!m_stopFlag.load())
    {
        // ── [عربي] الاستماع للشبكتين بذكاء ──
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(sock4, &readSet);
        if (sock6 != INVALID_SOCKET) FD_SET(sock6, &readSet);

        timeval tv = {0, 100000}; // 100ms timeout
        int res = select(0, &readSet, nullptr, nullptr, &tv);

        if (res > 0) {
            SOCKET activeSock = FD_ISSET(sock4, &readSet) ? sock4 : (sock6 != INVALID_SOCKET && FD_ISSET(sock6, &readSet) ? sock6 : INVALID_SOCKET);
            if (activeSock == INVALID_SOCKET) continue;

            int len = recv(activeSock, buf.data(), buf.size(), 0);
            if (len == SOCKET_ERROR) { if (WSAGetLastError() == WSAEMSGSIZE) len = buf.size(); else continue; }
            else if (len <= 0) continue;

            ParsedPacket pkt = PacketParser::parseIP(reinterpret_cast<const uint8_t*>(buf.constData()), static_cast<uint32_t>(len));
            if (!pkt.valid) continue;

            const bool srcIsLocal = m_localIPs.contains(pkt.srcIP) || pkt.srcIP == m_adapterIP || pkt.srcIP == m_adapterIPv6;
            const bool dstIsLocal = m_localIPs.contains(pkt.dstIP) || pkt.dstIP == m_adapterIP || pkt.dstIP == m_adapterIPv6;
            pkt.isDownload = dstIsLocal && !srcIsLocal;

            quint32 pid  = 0;
            QString proc = "Unknown";
            bool isSvc = false;

            if (pkt.protocol.startsWith("TCP") || pkt.protocol.startsWith("UDP")) {
                QString sPortStr = QString::number(pkt.srcPort);
                QString dPortStr = QString::number(pkt.dstPort);
                QString tcpF = pkt.srcIP + ":" + sPortStr + "-" + pkt.dstIP + ":" + dPortStr;
                QString tcpR = pkt.dstIP + ":" + dPortStr + "-" + pkt.srcIP + ":" + sPortStr;
                QString udpS = pkt.srcIP + ":" + sPortStr;
                QString udpD = pkt.dstIP + ":" + dPortStr;

                QMutexLocker lk(&m_cacheMutex);
                if (pkt.protocol.startsWith("TCP")) {
                    if (m_tcpCache.contains(tcpF)) pid = m_tcpCache.value(tcpF);
                    else if (m_tcpCache.contains(tcpR)) pid = m_tcpCache.value(tcpR);

                    if (pid == 0) {
                        quint16 localPort = pkt.isDownload ? pkt.dstPort : pkt.srcPort;
                        if (m_tcpListenCache.contains(localPort)) pid = m_tcpListenCache.value(localPort);
                    }
                } else {
                    if (m_udpCache.contains(udpS)) pid = m_udpCache.value(udpS);
                    else if (m_udpCache.contains(udpD)) pid = m_udpCache.value(udpD);

                    if (pid == 0) {
                        quint16 localPort = pkt.isDownload ? pkt.dstPort : pkt.srcPort;
                        if (m_udpAnyCache.contains(localPort)) pid = m_udpAnyCache.value(localPort);
                    }
                }

                if (pid == 0) {
                    proc = "System Idle / Network Overhead";
                    isSvc = true;
                } else if (pid == 4) {
                    proc = "System Kernel (ntoskrnl)";
                    isSvc = true;
                } else {
                    proc = m_procCache.value(pid, "Unknown");
                    if (proc.startsWith("Service:")) isSvc = true;
                }
            }

            batch.append({pkt, pid, proc, isSvc});

            qint64 now = QDateTime::currentMSecsSinceEpoch();
            if (now - lastBatchTime >= 250 || batch.size() >= 3000) {
                emit packetsCapturedBatch(batch);
                batch.clear();
                lastBatchTime = now;
            }
        }
    }

    if (!batch.isEmpty()) emit packetsCapturedBatch(batch);

    cacheStop.store(true);
    cacheThread.join();

    DWORD off = RCVALL_OFF;
    WSAIoctl(sock4, SIO_RCVALL, &off, sizeof(off), nullptr, 0, &dw, nullptr, nullptr);
    closesocket(sock4);
    if (sock6 != INVALID_SOCKET) {
        WSAIoctl(sock6, SIO_RCVALL, &off, sizeof(off), nullptr, 0, &dw, nullptr, nullptr);
        closesocket(sock6);
    }
    WSACleanup();

    m_captureRunning.store(false); emit captureStopped();
}

void NetworkMonitor::updateCaches()
{
    QHash<QString, quint32> tCache, uCache;
    QHash<quint16, quint32> tListenCache, uAnyCache;
    QHash<quint32, QString> pCache;

    // ── [عربي] 1. استخراج عمليات IPv4 ──
    DWORD szTcp = 0;
    GetExtendedTcpTable(nullptr, &szTcp, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
    QByteArray bufTcp(szTcp += 1024*20, '\0');
    if (GetExtendedTcpTable(bufTcp.data(), &szTcp, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0) == NO_ERROR) {
        auto *t = reinterpret_cast<const MIB_TCPTABLE_OWNER_PID*>(bufTcp.constData());
        for (DWORD i = 0; i < t->dwNumEntries; ++i) {
            const auto &row = t->table[i];
            quint16 lport = ntohs(static_cast<uint16_t>(row.dwLocalPort));
            quint32 pid = static_cast<quint32>(row.dwOwningPid);
            if (row.dwState == MIB_TCP_STATE_LISTEN) tListenCache.insert(lport, pid);
            IN_ADDR la, ra; la.S_un.S_addr = row.dwLocalAddr; ra.S_un.S_addr = row.dwRemoteAddr;
            char ls[INET_ADDRSTRLEN]={}, rs[INET_ADDRSTRLEN]={};
            inet_ntop(AF_INET, &la, ls, sizeof(ls)); inet_ntop(AF_INET, &ra, rs, sizeof(rs));
            tCache.insert(QString::fromLatin1(ls) + ":" + QString::number(lport) + "-" + QString::fromLatin1(rs) + ":" + QString::number(ntohs(static_cast<uint16_t>(row.dwRemotePort))), pid);
        }
    }

    DWORD szUdp = 0;
    GetExtendedUdpTable(nullptr, &szUdp, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0);
    QByteArray bufUdp(szUdp += 1024*20, '\0');
    if (GetExtendedUdpTable(bufUdp.data(), &szUdp, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0) == NO_ERROR) {
        auto *u = reinterpret_cast<const MIB_UDPTABLE_OWNER_PID*>(bufUdp.constData());
        for (DWORD i = 0; i < u->dwNumEntries; ++i) {
            const auto &row = u->table[i];
            quint16 lport = ntohs(static_cast<uint16_t>(row.dwLocalPort));
            quint32 pid = static_cast<quint32>(row.dwOwningPid);
            if (row.dwLocalAddr == 0) uAnyCache.insert(lport, pid);
            IN_ADDR la; la.S_un.S_addr = row.dwLocalAddr;
            char ls[INET_ADDRSTRLEN]={}; inet_ntop(AF_INET, &la, ls, sizeof(ls));
            uCache.insert(QString::fromLatin1(ls) + ":" + QString::number(lport), pid);
        }
    }

    // ── [عربي] 2. استخراج عمليات IPv6 (مع إصلاح توافق المترجم const_cast) ──
    DWORD szTcp6 = 0;
    GetExtendedTcpTable(nullptr, &szTcp6, FALSE, AF_INET6, TCP_TABLE_OWNER_PID_ALL, 0);
    QByteArray bufTcp6(szTcp6 += 1024*20, '\0');
    if (GetExtendedTcpTable(bufTcp6.data(), &szTcp6, FALSE, AF_INET6, TCP_TABLE_OWNER_PID_ALL, 0) == NO_ERROR) {
        auto *t6 = reinterpret_cast<const MIB_TCP6TABLE_OWNER_PID*>(bufTcp6.constData());
        for (DWORD i = 0; i < t6->dwNumEntries; ++i) {
            const auto &row = t6->table[i];
            quint16 lport = ntohs(static_cast<uint16_t>(row.dwLocalPort));
            quint32 pid = static_cast<quint32>(row.dwOwningPid);
            if (row.dwState == MIB_TCP_STATE_LISTEN) tListenCache.insert(lport, pid);

            char ls[INET6_ADDRSTRLEN]={}, rs[INET6_ADDRSTRLEN]={};
            // استخدام const_cast لتفادي أخطاء MinGW القديمة
            inet_ntop(AF_INET6, const_cast<UCHAR*>(row.ucLocalAddr), ls, sizeof(ls));
            inet_ntop(AF_INET6, const_cast<UCHAR*>(row.ucRemoteAddr), rs, sizeof(rs));

            tCache.insert(QString::fromLatin1(ls) + ":" + QString::number(lport) + "-" + QString::fromLatin1(rs) + ":" + QString::number(ntohs(static_cast<uint16_t>(row.dwRemotePort))), pid);
        }
    }

    DWORD szUdp6 = 0;
    GetExtendedUdpTable(nullptr, &szUdp6, FALSE, AF_INET6, UDP_TABLE_OWNER_PID, 0);
    QByteArray bufUdp6(szUdp6 += 1024*20, '\0');
    if (GetExtendedUdpTable(bufUdp6.data(), &szUdp6, FALSE, AF_INET6, UDP_TABLE_OWNER_PID, 0) == NO_ERROR) {
        auto *u6 = reinterpret_cast<const MIB_UDP6TABLE_OWNER_PID*>(bufUdp6.constData());
        for (DWORD i = 0; i < u6->dwNumEntries; ++i) {
            const auto &row = u6->table[i];
            quint16 lport = ntohs(static_cast<uint16_t>(row.dwLocalPort));
            quint32 pid = static_cast<quint32>(row.dwOwningPid);
            bool isAny = true;
            for(int j=0; j<16; ++j) if(row.ucLocalAddr[j] != 0) { isAny = false; break; }
            if (isAny) uAnyCache.insert(lport, pid);

            char ls[INET6_ADDRSTRLEN]={};
            inet_ntop(AF_INET6, const_cast<UCHAR*>(row.ucLocalAddr), ls, sizeof(ls));

            uCache.insert(QString::fromLatin1(ls) + ":" + QString::number(lport), pid);
        }
    }

    // ── [عربي] 3. جلب أسماء العمليات ──
    DWORD aProcs[1024], cbNeeded, cProcs;
    if (EnumProcesses(aProcs, sizeof(aProcs), &cbNeeded)) {
        cProcs = cbNeeded / sizeof(DWORD);
        for (unsigned i = 0; i < cProcs; i++) {
            if (aProcs[i] != 0) {
                HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, aProcs[i]);
                if (hProc) {
                    wchar_t path[MAX_PATH] = {};
                    DWORD size = MAX_PATH;
                    if (QueryFullProcessImageNameW(hProc, 0, path, &size)) {
                        QString fullPath = QString::fromWCharArray(path, static_cast<int>(size));
                        int idx = fullPath.lastIndexOf(QLatin1Char('\\'));
                        QString procName = (idx >= 0) ? fullPath.mid(idx + 1) : fullPath;

                        if (procName.compare("svchost.exe", Qt::CaseInsensitive) == 0) {
                            QString svcDisplayName = getServiceNameFromPid(aProcs[i]);
                            if (!svcDisplayName.isEmpty()) procName = "Service: " + svcDisplayName;
                        }
                        pCache.insert(aProcs[i], procName);
                    }
                    CloseHandle(hProc);
                }
            }
        }
    }

    QMutexLocker lk(&m_cacheMutex);
    m_tcpCache = tCache;
    m_udpCache = uCache;
    m_tcpListenCache = tListenCache;
    m_udpAnyCache = uAnyCache;
    m_procCache = pCache;
}

void NetworkMonitor::refreshLocalIPs()
{
    m_localIPs.clear();
    ULONG sz = 15000;
    QByteArray buf(static_cast<int>(sz), '\0');
    auto *a = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buf.data());
    if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_SKIP_ANYCAST|GAA_FLAG_SKIP_MULTICAST, nullptr, a, &sz) != NO_ERROR) return;
    for (; a; a = a->Next)
        for (auto *ua = a->FirstUnicastAddress; ua; ua = ua->Next) {
            SOCKADDR *s = ua->Address.lpSockaddr;
            char ip[INET6_ADDRSTRLEN] = {};
            if      (s->sa_family==AF_INET)  inet_ntop(AF_INET,  &reinterpret_cast<sockaddr_in *>(s)->sin_addr,  ip, sizeof(ip));
            else if (s->sa_family==AF_INET6) inet_ntop(AF_INET6, &reinterpret_cast<sockaddr_in6*>(s)->sin6_addr, ip, sizeof(ip));
            if (ip[0]) m_localIPs << QString::fromLatin1(ip);
        }
}
