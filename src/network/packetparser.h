/**
 * @file    packetparser.h
 * @brief   Wire-format structs, ParsedPacket with Q_DECLARE_METATYPE,
 *          ConnectionKey (FNV hash), and PacketParser namespace.
 * @author  Ali Sakkaf
 */
#pragma once

#include <QString>
#include <QMetaType>
#include <cstdint>

// ─── Wire-format structs (packed) ───────────────────────────────────────────
#pragma pack(push,1)
struct EthernetHeader { uint8_t dst[6]; uint8_t src[6]; uint16_t type; };
struct IPv4Header { uint8_t verIhl; uint8_t tos; uint16_t totalLen;
                    uint16_t id; uint16_t flags; uint8_t ttl; uint8_t protocol;
                    uint16_t checksum; uint8_t src[4]; uint8_t dst[4]; };
struct IPv6Header { uint32_t vtcFlow; uint16_t payloadLen; uint8_t nextHeader;
                    uint8_t hopLimit; uint8_t src[16]; uint8_t dst[16]; };
struct TCPHeader  { uint16_t srcPort; uint16_t dstPort; uint32_t seqNum;
                    uint32_t ackNum; uint8_t dataOffset; uint8_t flags;
                    uint16_t window; uint16_t checksum; uint16_t urgPtr; };
struct UDPHeader  { uint16_t srcPort; uint16_t dstPort; uint16_t length; uint16_t checksum; };
#pragma pack(pop)

// ─── Protocol / EtherType constants ─────────────────────────────────────────
enum EtherType : uint16_t { ETHERTYPE_IPV4=0x0800, ETHERTYPE_IPV6=0x86DD,
                             ETHERTYPE_ARP=0x0806,  ETHERTYPE_VLAN=0x8100 };
enum IPProtocol : uint8_t  { IPPROTO_ICMP_=1, IPPROTO_TCP_=6, IPPROTO_UDP_=17,
                              IPPROTO_ICMPv6_=58 };

// ─── ParsedPacket ────────────────────────────────────────────────────────────
struct ParsedPacket
{
    bool     valid      = false;
    bool     isIPv6     = false;
    bool     isDownload = false; // true = dst is local IP (RX), false = TX
    uint16_t etherType  = 0;
    uint8_t  ipProto    = 0;
    QString  srcIP, dstIP;
    uint16_t srcPort    = 0;
    uint16_t dstPort    = 0;
    uint32_t captureLen = 0;
    uint32_t wireLen    = 0;    // IP total-length (header + payload)
    QString  protocol;
    QString  service;
};
Q_DECLARE_METATYPE(ParsedPacket)

// ─── ConnectionKey ───────────────────────────────────────────────────────────
struct ConnectionKey
{
    QString  srcIP, dstIP;
    uint16_t srcPort = 0, dstPort = 0;
    QString  protocol;

    bool operator==(const ConnectionKey &o) const {
        return srcIP==o.srcIP && dstIP==o.dstIP &&
               srcPort==o.srcPort && dstPort==o.dstPort &&
               protocol==o.protocol;
    }
};

// FNV-1a manual hash — avoids calling qHash() to prevent self-reference ambiguity
inline uint qHash(const ConnectionKey &k, uint seed = 0) noexcept
{
    uint h = seed ^ 2166136261u;
    auto mix = [&h](const QString &s) {
        for (int i = 0; i < s.size(); ++i)
            h = (h ^ static_cast<uint>(s.at(i).unicode())) * 16777619u;
    };
    mix(k.srcIP); mix(k.dstIP); mix(k.protocol);
    h = (h ^ static_cast<uint>(k.srcPort)) * 16777619u;
    h = (h ^ static_cast<uint>(k.dstPort)) * 16777619u;
    return h;
}

// ─── PacketParser namespace ──────────────────────────────────────────────────
namespace PacketParser
{
    ParsedPacket parse  (const uint8_t *data, uint32_t capLen, uint32_t wireLen);
    ParsedPacket parseIP(const uint8_t *data, uint32_t len);

    QString  ipv4ToString   (const uint8_t ip[4]);
    QString  ipv6ToString   (const uint8_t ip[16]);
    QString  serviceName    (uint16_t port, const QString &proto);
    QString  etherTypeString(uint16_t type);
    uint16_t ntohs_(uint16_t v);
    uint32_t ntohl_(uint32_t v);
}
