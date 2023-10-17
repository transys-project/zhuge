#ifndef NETWORK_QUEUE_H
#define NETWORK_QUEUE_H

#include "network-queue.h"
#include "delta-delay.h"

#include "ns3/fec-policy.h"
#include "ns3/packet-receiver-udp.h"
#include "ns3/packet-receiver-tcp.h"
#include "ns3/video-decoder.h"

#include "ns3/application.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/simulator.h"
#include "ns3/assert.h"
#include "ns3/socket.h"
#include "ns3/wifi-module.h"
#include "ns3/wifi-net-device.h"
#include "ns3/pointer.h"
#include "ns3/wifi-net-device.h"
#include "ns3/minstrel-wifi-manager.h"
#include "ns3/minstrel-ht-wifi-manager.h"
#include "ns3/ap-wifi-mac.h"
#include "ns3/ampdu-subframe-header.h"
#include "ns3/mobility-model.h"
#include "ns3/log.h"
#include "ns3/radiotap-header.h"
#include "ns3/config.h"
#include "ns3/names.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/wifi-mac-queue.h"
#include "ns3/qos-utils.h"
#include "ns3/ht-configuration.h"
#include "ns3/vht-configuration.h"
#include "ns3/he-configuration.h"
#include "ns3/obss-pd-algorithm.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/llc-snap-header.h"
#include "ns3/queue-disc.h"
#include "ns3/traffic-control-layer.h"
#include "ns3/ipv4-interface.h"
#include <unordered_map>
#include <numeric>

namespace ns3 {

class NetworkQueueZhuge : public NetworkQueue {

public:
    static TypeId GetTypeId (void);
    NetworkQueueZhuge();
    ~NetworkQueueZhuge();
    void Setup (Ipv4Address sendIP, uint16_t sendPort, Ipv4Address recvIP, uint16_t recvPort, 
        bool wirtc, uint16_t interval, bool tcpEnabled, int mode, int qsize, 
        Ptr<NetDevice> sndDev, Ptr<NetDevice> rcvDev, bool fastAckRtx);
    void Sendback();
    bool OnRecv_rcvdev(Ptr<NetDevice>, Ptr<const Packet>, uint16_t, const Address &, const Address &, NetDevice::PacketType);
    bool OnRecv_snddev(Ptr<NetDevice>, Ptr<const Packet>, uint16_t, const Address &, const Address &, NetDevice::PacketType);
    void SendPkt(Ptr<Packet> pkt_to_send);
    uint32_t CalculateTotalDelayMs(uint32_t extraByte);
    void SetAckDelay(Ipv4Header ipHdr);

protected:
    void DoDispose(void);
    Ptr<NetDevice> m_sndDev;
    Ptr<PointToPointNetDevice> m_rcvDev;

    Ptr<NetDevice> GivenSndDev;
    Ptr<NetDevice> GivenRcvDev;

    Ptr<WifiMacQueue> m_wmq;
    Ptr<DropTailQueue<Packet>> m_dlQueue;
    Ptr<NetworkPacket> sndback_pkt;
    uint32_t pkt_size;
    bool schedule_flag;

private:
    void StartApplication(void);
    void StopApplication(void);

private:
    bool m_wirtcEnable;
    Ptr<PacketReceiverUdp> receiver;

    Ipv4Address m_sendIP;
    uint16_t m_sendPort;
    Ipv4Address m_recvIP;
    uint16_t m_recvPort;
    uint16_t c_sendPort;
    bool cc_create;
    uint32_t m_lastTotalTxBytes;
    uint16_t last_id;
    uint16_t m_lastHeadId;
    Ipv4Address m_lastHeadDstAddr;
    Ipv4Address sendaddr;
    uint16_t send_id;
    uint32_t m_curQHeadTimeMs;
    uint32_t m_curQSizeByte;
    Ptr<QosTxop> m_edca;
    int64_t last_time;
    bool startSend;
    Time kTxBytesListInterval;
    uint32_t extra_bytes;
    int wifi_same_ap;
    int node_number;
    int m_mode;
    int m_qsize;
    Time last_update;
    PppHeader sendback_ppp_hdr;
    Ipv4Header sendback_ip_hdr;
    UdpHeader sendback_udp_hdr;
    Ptr<QueueDisc> m_qdisc;
    Ptr<Ipv4Interface> m_interface;
    std::deque<int32_t> m_txBytesList;
    uint32_t m_lastQSizeByte;
    uint32_t m_lastQDelayMs;
    uint32_t m_lastTotalDelayMs;
    uint32_t kAmpduMaxWaitMs;

    // fast Rtx from FastAck
    bool m_fastAckRtx;
    uint32_t m_DupNumber;
    SequenceNumber32 m_LastRcvAck;
    std::deque<Ptr<Packet>> m_cache;

    Time m_lastAckArrv;
    Time m_lastAckSent;
    DeltaDelay m_deltaAckDelay;
    Time kMaxActualDelay;
    uint32_t AckToSend;
    bool m_isFirstPkt;
    bool m_isFirstAck;
    uint16_t kTxBytesListWindow;
    uint32_t kMinTxRateBps;
    uint32_t m_curTxRateBps;
    uint32_t m_curTxDelayUs;
    uint32_t m_curTxTransRateBps;

    EventId m_eventUpdateTxRate;
    EventId m_eventSendback;

    void InitSocket();
    void UpdateTxRate();

};  // class NetworkQueueZhuge

};  // namespace ns3

#endif  /* NETWORK_QUEUE_H */
