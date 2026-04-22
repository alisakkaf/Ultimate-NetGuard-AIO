/**
 * @file    packetparser.cpp
 * @author  Ali Sakkaf
 */
#include "packetparser.h"
#include <winsock2.h>   // for ntohs/ntohl
#include <QHash>

// ─── Byte-order helpers ──────────────────────────────────────────────────────
uint16_t PacketParser::ntohs_(uint16_t v) { return ntohs(v); }
uint32_t PacketParser::ntohl_(uint32_t v) { return ntohl(v); }

// ─── IP address to string ────────────────────────────────────────────────────
QString PacketParser::ipv4ToString(const uint8_t ip[4]) {
    return QString("%1.%2.%3.%4").arg(ip[0]).arg(ip[1]).arg(ip[2]).arg(ip[3]);
}

QString PacketParser::ipv6ToString(const uint8_t ip[16]) {
    QString s;
    for (int i=0;i<16;i+=2) {
        if (i) s += ':';
        s += QString("%1").arg(static_cast<uint>((ip[i]<<8)|ip[i+1]), 4, 16, QChar('0'));
    }
    return s;
}

// ─── Service name lookup ────────────────────────────────────────────────────
QString PacketParser::serviceName(uint16_t port, const QString &proto)
{
    static const QHash<uint16_t,QString> known = {
        {80,"HTTP"},{443,"HTTPS"},{53,"DNS"},{22,"SSH"},{21,"FTP"},
        {25,"SMTP"},{587,"SMTP"},{143,"IMAP"},{993,"IMAP"},{110,"POP3"},
        {995,"POP3"},{3306,"MySQL"},{5432,"Postgres"},{6379,"Redis"},
        {27017,"MongoDB"},{3389,"RDP"},{5900,"VNC"},{123,"NTP"},
        {137,"NetBIOS"},{138,"NetBIOS"},{139,"NetBIOS"},{445,"SMB"},
        {8080,"HTTP-Alt"},{8443,"HTTPS-Alt"},{67,"DHCP"},{68,"DHCP"},
        {500,"IKE"},{4500,"IKE-NAT"},{1194,"OpenVPN"}
    };
    Q_UNUSED(proto)
    auto it = known.constFind(port);
    return (it != known.constEnd()) ? it.value() : QString();
}

QString PacketParser::etherTypeString(uint16_t type)
{
    switch(type) {
    case ETHERTYPE_IPV4: return "IPv4";
    case ETHERTYPE_IPV6: return "IPv6";
    case ETHERTYPE_ARP:  return "ARP";
    default:             return QString("0x%1").arg(type, 4, 16, QChar('0'));
    }
}

// ─── Transport-layer parsing helper ─────────────────────────────────────────
static void parseTransport(ParsedPacket &pkt, const uint8_t *data, uint32_t len)
{
    if (pkt.ipProto == IPPROTO_TCP_ && len >= sizeof(TCPHeader)) {
        const TCPHeader *th = reinterpret_cast<const TCPHeader*>(data);
        pkt.srcPort  = ntohs(th->srcPort);
        pkt.dstPort  = ntohs(th->dstPort);
        pkt.protocol = pkt.isIPv6 ? "TCP6" : "TCP";
    } else if (pkt.ipProto == IPPROTO_UDP_ && len >= sizeof(UDPHeader)) {
        const UDPHeader *uh = reinterpret_cast<const UDPHeader*>(data);
        pkt.srcPort  = ntohs(uh->srcPort);
        pkt.dstPort  = ntohs(uh->dstPort);
        pkt.protocol = pkt.isIPv6 ? "UDP6" : "UDP";
    } else if (pkt.ipProto == IPPROTO_ICMP_) {
        pkt.protocol = "ICMP";
    } else if (pkt.ipProto == IPPROTO_ICMPv6_) {
        pkt.protocol = "ICMPv6";
    } else {
        pkt.protocol = QString("IP/%1").arg(pkt.ipProto);
    }
    pkt.service = PacketParser::serviceName(pkt.dstPort, pkt.protocol);
    pkt.valid   = true;
}

// ─── Raw IP packet (from SIO_RCVALL — no Ethernet header) ───────────────────
ParsedPacket PacketParser::parseIP(const uint8_t *data, uint32_t len)
{
    ParsedPacket pkt;
    if (len < 20) return pkt;

    const uint8_t ver = (data[0] >> 4) & 0x0F;
    if (ver == 4) {
        if (len < sizeof(IPv4Header)) return pkt;
        const IPv4Header *ip = reinterpret_cast<const IPv4Header*>(data);
        uint8_t ihl = (ip->verIhl & 0x0F) * 4;
        if (len < ihl) return pkt;
        pkt.srcIP    = ipv4ToString(ip->src);
        pkt.dstIP    = ipv4ToString(ip->dst);
        pkt.ipProto  = ip->protocol;
        pkt.wireLen  = ntohs(ip->totalLen);
        pkt.captureLen = len;
        parseTransport(pkt, data + ihl, len - ihl);
    } else if (ver == 6) {
        if (len < sizeof(IPv6Header)) return pkt;
        const IPv6Header *ip = reinterpret_cast<const IPv6Header*>(data);
        pkt.isIPv6  = true;
        pkt.srcIP   = ipv6ToString(ip->src);
        pkt.dstIP   = ipv6ToString(ip->dst);
        pkt.ipProto = ip->nextHeader;
        pkt.wireLen = 40 + ntohs(ip->payloadLen);
        pkt.captureLen = len;
        parseTransport(pkt, data + 40, len - 40);
    }
    return pkt;
}

// ─── Ethernet-framed packet (pcap / raw Ethernet) ────────────────────────────
ParsedPacket PacketParser::parse(const uint8_t *data, uint32_t capLen, uint32_t wireLen)
{
    ParsedPacket pkt;
    if (capLen < sizeof(EthernetHeader)) return pkt;
    const EthernetHeader *eth = reinterpret_cast<const EthernetHeader*>(data);
    uint16_t et = ntohs(eth->type);
    pkt.etherType = et;
    pkt.wireLen   = wireLen;
    pkt.captureLen= capLen;
    if (et == ETHERTYPE_VLAN && capLen >= sizeof(EthernetHeader)+4)
        et = ntohs(*reinterpret_cast<const uint16_t*>(data + sizeof(EthernetHeader) + 2));
    const uint8_t *payload = data + sizeof(EthernetHeader);
    uint32_t       payLen  = capLen - static_cast<uint32_t>(sizeof(EthernetHeader));
    if (et == ETHERTYPE_IPV4 || et == ETHERTYPE_IPV6)
        return parseIP(payload, payLen);
    return pkt;
}
