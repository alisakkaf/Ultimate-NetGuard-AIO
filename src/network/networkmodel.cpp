/**
 * @file    networkmodel.cpp
 * @brief   Network tree model implementation with silent batch additions and UserRoles.
 * @author  Ali Sakkaf
 */
#include "networkmodel.h"
#include "networkmonitor.h"
#include <QFont>
#include <QColor>
#include <QBrush>

NetworkTreeModel::NetworkTreeModel(QObject *parent) : QAbstractItemModel(parent)
{
    m_ticker = new QTimer(this);
    m_ticker->setInterval(1000);
    connect(m_ticker, &QTimer::timeout, this, &NetworkTreeModel::onTick);
    m_ticker->start();
}

QModelIndex NetworkTreeModel::index(int row, int col, const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        if (row < 0 || row >= m_procs.size()) return {};
        return createIndex(row, col, nullptr);
    }
    if (parent.internalPointer() != nullptr) return {};
    int pRow = parent.row();
    if (pRow < 0 || pRow >= m_procs.size()) return {};
    if (row < 0 || row >= m_procs[pRow].conns.size()) return {};
    return createIndex(row, col, reinterpret_cast<void*>(static_cast<uintptr_t>(pRow + 1)));
}

QModelIndex NetworkTreeModel::parent(const QModelIndex &child) const
{
    if (!child.isValid()) return {};
    if (child.internalPointer() == nullptr) return {};
    int pRow = static_cast<int>(reinterpret_cast<uintptr_t>(child.internalPointer())) - 1;
    return createIndex(pRow, 0, nullptr);
}

int NetworkTreeModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) return m_procs.size();
    if (parent.internalPointer() != nullptr) return 0;
    int pRow = parent.row();
    if (pRow < 0 || pRow >= m_procs.size()) return 0;
    return m_procs[pRow].conns.size();
}

int NetworkTreeModel::columnCount(const QModelIndex &) const { return COL_COUNT; }

Qt::ItemFlags NetworkTreeModel::flags(const QModelIndex &idx) const
{
    return idx.isValid() ? Qt::ItemIsEnabled | Qt::ItemIsSelectable : Qt::NoItemFlags;
}

QVariant NetworkTreeModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid()) return {};

    const bool isProcess = (idx.internalPointer() == nullptr);
    const int  col       = idx.column();

    if (isProcess)
    {
        if (idx.row() >= m_procs.size()) return {};
        const ProcessNode &p = m_procs[idx.row()];

        // ── Custom Roles for Context Menu (Task Kill/Info) ──
        if (role == Qt::UserRole) return p.pid;
        if (role == Qt::UserRole + 1) return p.name;
        if (role == Qt::UserRole + 2) return true; // isProcess flag

        // ── FIX: Custom Role for Numeric Sorting ──
        if (role == Qt::UserRole + 5) {
            switch (col) {
            case COL_NAME:   return p.name;
            case COL_RX:     return static_cast<qulonglong>(p.totalRxSpeed);
            case COL_TX:     return static_cast<qulonglong>(p.totalTxSpeed);
            case COL_BYTES:  return static_cast<qulonglong>(p.totalRxBytes + p.totalTxBytes);
            case COL_PKTS:   return static_cast<qulonglong>(p.totalPackets);
            default:         return QVariant();
            }
        }

        if (role == Qt::DecorationRole && col == COL_NAME)
            return p.icon.isNull() ? QVariant() : QVariant(p.icon.pixmap(18, 18));

        if (role == Qt::DisplayRole) {
            switch (col) {
            case COL_NAME: return QString("%1  (PID %2)  •  %3 conn%4").arg(p.name).arg(p.pid).arg(p.conns.size()).arg(p.conns.size() == 1 ? "" : "s");
            case COL_SRC:    return {};
            case COL_DST:    return {};
            case COL_SERVICE:return {};
            case COL_RX:     return "↓ " + formatSpeed(p.totalRxSpeed);
            case COL_TX:     return "↑ " + formatSpeed(p.totalTxSpeed);
            case COL_BYTES:  return formatBytes(p.totalRxBytes + p.totalTxBytes);
            case COL_PKTS:   return static_cast<qulonglong>(p.totalPackets);
            }
        }
        if (role == Qt::ForegroundRole) {
            if (col == COL_RX) return QColor("#3B82F6");
            if (col == COL_TX) return QColor("#10B981");
        }
        if (role == Qt::FontRole) { QFont f; f.setBold(true); return f; }
        if (role == Qt::BackgroundRole && (p.totalRxSpeed + p.totalTxSpeed) > 0) return QColor(59, 130, 246, 20);
        return {};
    }

    int pRow = static_cast<int>(reinterpret_cast<uintptr_t>(idx.internalPointer())) - 1;
    if (pRow < 0 || pRow >= m_procs.size()) return {};
    const ProcessNode    &p  = m_procs[pRow];
    if (idx.row() >= p.conns.size()) return {};
    const ConnectionStat &c  = p.conns[idx.row()];

    // ── Custom Roles for Connection child rows ──
    if (role == Qt::UserRole) return p.pid;
    if (role == Qt::UserRole + 1) return c.key.srcIP + ":" + QString::number(c.key.srcPort);
    if (role == Qt::UserRole + 2) return false; // not process node

    // ── FIX: Custom Role for Numeric Sorting ──
    if (role == Qt::UserRole + 5) {
        switch (col) {
        case COL_NAME:   return c.key.protocol;
        case COL_RX:     return static_cast<qulonglong>(c.rxSpeed);
        case COL_TX:     return static_cast<qulonglong>(c.txSpeed);
        case COL_BYTES:  return static_cast<qulonglong>(c.rxBytes + c.txBytes);
        case COL_PKTS:   return static_cast<qulonglong>(c.packets);
        default:         return QVariant();
        }
    }

    if (role == Qt::DisplayRole) {
        switch (col) {
        case COL_NAME:    return c.key.protocol;
        case COL_SRC:     return QString("%1 : %2").arg(c.key.srcIP).arg(c.key.srcPort);
        case COL_DST:     return QString("%1 : %2").arg(c.key.dstIP).arg(c.key.dstPort);
        case COL_SERVICE: return c.service;
        case COL_RX:      return "↓ " + formatSpeed(c.rxSpeed);
        case COL_TX:      return "↑ " + formatSpeed(c.txSpeed);
        case COL_BYTES:   return formatBytes(c.rxBytes + c.txBytes);
        case COL_PKTS:    return static_cast<qulonglong>(c.packets);
        }
    }

    if (role == Qt::ForegroundRole) {
        if (col == COL_NAME) {
            if (c.key.protocol.startsWith("TCP"))  return QColor("#3B82F6");
            if (c.key.protocol.startsWith("UDP"))  return QColor("#10B981");
            if (c.key.protocol.startsWith("ICMP")) return QColor("#F59E0B");
        }
        if (col == COL_RX) return QColor("#3B82F6");
        if (col == COL_TX) return QColor("#10B981");
    }
    return {};
}

QVariant NetworkTreeModel::headerData(int s, Qt::Orientation o, int role) const
{
    if (o != Qt::Horizontal || role != Qt::DisplayRole) return {};
    static const char *H[COL_COUNT] = {
        "Application / Protocol", "Source", "Destination",
        "Service", "Download", "Upload", "Total Bytes", "Packets"
    };
    return QLatin1String(H[s]);
}

void NetworkTreeModel::addPackets(const QList<CapturedPacketInfo> &batch)
{
    for (const auto& info : batch) {
        const ParsedPacket &pkt = info.pkt;
        quint32 pid = info.pid;
        QString proc = info.proc;

        const QString name = proc.isEmpty() ? (pid ? QString("PID %1").arg(pid) : "Unknown") : proc;
        const int pRow = findOrAddProcess(name, pid);
        ProcessNode &p = m_procs[pRow];

        const quint64 bytes  = pkt.wireLen ? pkt.wireLen : pkt.captureLen;
        const bool    isDown = pkt.isDownload;

        ConnectionKey key;
        key.srcIP = pkt.srcIP; key.srcPort = pkt.srcPort;
        key.dstIP = pkt.dstIP; key.dstPort = pkt.dstPort;
        key.protocol = pkt.protocol;

        auto it = p.connIdx.find(key);
        if (it == p.connIdx.end()) {
            const int cRow = p.conns.size();
            const QModelIndex pIdx = createIndex(pRow, 0, nullptr);
            beginInsertRows(pIdx, cRow, cRow);
            ConnectionStat c;
            c.key = key;
            if (isDown) c.rxBytes = bytes; else c.txBytes = bytes;
            c.packets = 1;
            c.service = pkt.service;
            p.conns.append(c);
            p.connIdx.insert(key, cRow);
            endInsertRows();
        } else {
            const int cRow = it.value();
            if (isDown) p.conns[cRow].rxBytes += bytes;
            else        p.conns[cRow].txBytes += bytes;
            p.conns[cRow].packets += 1;
        }

        if (isDown) p.totalRxBytes += bytes;
        else        p.totalTxBytes += bytes;
        p.totalPackets += 1;
    }
}

void NetworkTreeModel::onTick()
{
    for (int pRow = 0; pRow < m_procs.size(); ++pRow) {
        ProcessNode &p = m_procs[pRow];
        quint64 sumRx = 0, sumTx = 0;

        for (auto &c : p.conns) {
            c.rxSpeed = (c.rxBytes > c.prevRx) ? (c.rxBytes - c.prevRx) : 0;
            c.txSpeed = (c.txBytes > c.prevTx) ? (c.txBytes - c.prevTx) : 0;
            c.prevRx  = c.rxBytes;
            c.prevTx  = c.txBytes;
            sumRx    += c.rxSpeed;
            sumTx    += c.txSpeed;
        }
        p.totalRxSpeed = sumRx;
        p.totalTxSpeed = sumTx;

        // ── FIX: Emit dataChanged starting from COL_NAME to update parent UI correctly ──
        // ── FIX: Include Qt::UserRole + 5 to keep sorting updated dynamically ──
        if (!p.conns.isEmpty()) {
            const QModelIndex pIdx = createIndex(pRow, 0, nullptr);
            emit dataChanged(index(0, COL_NAME, pIdx), index(p.conns.size()-1, COL_PKTS, pIdx), {Qt::DisplayRole, Qt::UserRole + 5});
        }
        emit dataChanged(createIndex(pRow, COL_NAME, nullptr), createIndex(pRow, COL_PKTS, nullptr), {Qt::DisplayRole, Qt::UserRole + 5});
    }
}

void NetworkTreeModel::setProcessIcon(const QString &procName, const QIcon &icon)
{
    auto it = m_procIdx.find(procName);
    if (it == m_procIdx.end()) return;
    const int pRow = it.value();
    m_procs[pRow].icon = icon;
    const QModelIndex idx = createIndex(pRow, COL_NAME, nullptr);
    emit dataChanged(idx, idx, {Qt::DecorationRole});
}

void NetworkTreeModel::clear()
{
    if (m_procs.isEmpty()) return;
    beginResetModel();
    m_procs.clear();
    m_procIdx.clear();
    endResetModel();
}

int NetworkTreeModel::findOrAddProcess(const QString &name, quint32 pid)
{
    auto it = m_procIdx.find(name);
    if (it != m_procIdx.end()) {
        if (pid && !m_procs[it.value()].pid) m_procs[it.value()].pid = pid;
        return it.value();
    }
    const int pRow = m_procs.size();
    beginInsertRows(QModelIndex(), pRow, pRow);
    ProcessNode p; p.name = name; p.pid = pid;
    m_procs.append(p);
    m_procIdx.insert(name, pRow);
    endInsertRows();
    return pRow;
}

QString NetworkTreeModel::formatSpeed(quint64 bps)
{
    if (bps == 0)            return "0 B/s";
    if (bps < 1024)          return QString("%1 B/s").arg(bps);
    if (bps < 1024*1024)     return QString("%1 KB/s").arg(bps/1024.0, 0, 'f', 1);
    if (bps < 1024*1024*1024ULL) return QString("%1 MB/s").arg(bps/(1024.0*1024), 0, 'f', 2);
    return                         QString("%1 GB/s").arg(bps/(1024.0*1024*1024), 0, 'f', 2);
}

QString NetworkTreeModel::formatBytes(quint64 bytes)
{
    if (bytes < 1024)             return QString("%1 B").arg(bytes);
    if (bytes < 1024*1024)        return QString("%1 KB").arg(bytes/1024.0, 0, 'f', 1);
    if (bytes < 1024*1024*1024ULL)return QString("%1 MB").arg(bytes/(1024.0*1024), 0, 'f', 2);
    return                               QString("%1 GB").arg(bytes/(1024.0*1024*1024), 0, 'f', 2);
}

