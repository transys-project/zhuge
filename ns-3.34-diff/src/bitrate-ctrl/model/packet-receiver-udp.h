#ifndef PACKET_UDP_RECEIVER_H
#define PACKET_UDP_RECEIVER_H

#include "packet-receiver.h"
#include "ns3/fec-policy.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/nstime.h"
#include "ns3/timer.h"
#include <vector>
#include <queue>

namespace ns3 {

class GameClient;

class PacketReceiverUdp : public PacketReceiver {
public:
    static TypeId GetTypeId (void);
    PacketReceiverUdp(uint32_t, uint16_t, Ipv4Address ,uint16_t, uint16_t);
    ~PacketReceiverUdp();
    
    void StartApplication(GameClient* client, Ptr<VideoDecoder> decoder, Time delay_ddl, Ptr<Node> node);

    void OnSocketRecv_receiver(Ptr<Socket>);

    void Feedback_NetState();

    void ReplyACK(std::vector<Ptr<DataPacket>> data_pkts, uint16_t last_pkt_id);

    /**
     * Mainly for sending ACK packet
     */
    void SendPacket(Ptr<NetworkPacket>);
    void ReceiveUdpPacket(Ptr<VideoPacket> pkt);

    void Set_FECgroup_delay(Time&);
    Time Get_FECgroup_delay();
    // API for setting FEC group delay @kongxiao0532
    void OutputStatistics();
    void StopRunning();

    bool lessThan (uint16_t id1, uint16_t id2);

    bool lessThan_simple(uint16_t id1, uint16_t id2);

private:

    std::deque<Ptr<RcvTime>> m_record;

    Time     m_feedback_interval;
    Timer    m_feedbackTimer;
    uint16_t m_last_feedback;

    /* Calculate statistics within a sliding window */
    uint16_t wnd_size;
    uint32_t time_wnd_size; // in us
    uint16_t m_credible;
    uint32_t bytes_per_packet;
    uint16_t last_id;

    uint16_t pkts_in_wnd;
    uint32_t bytes_in_wnd;
    uint32_t time_in_wnd;
    uint32_t losses_in_wnd;

    /* statistics */
    uint64_t rcvd_pkt_cnt;
    uint64_t rcvd_data_pkt_cnt;
    uint64_t rcvd_fec_pkt_cnt;
    std::map<uint8_t, uint64_t> rcvd_pkt_rtx_count;
    std::map<uint8_t, uint64_t> rcvd_group_rtx_count;
    std::map<uint8_t, uint64_t> rcvd_datapkt_rtx_count;
    std::map<uint8_t, uint64_t> rcvd_fecpkt_rtx_count;
    std::map<uint8_t, uint64_t> rcvd_rtxpkt_rtx_count;
    std::unordered_map<uint32_t, Ptr<PacketGroup>> incomplete_groups;
    std::unordered_map<uint32_t, Ptr<PacketGroup>> complete_groups;
    std::unordered_map<uint32_t, Ptr<PacketGroup>> timeout_groups;

    float loss_rate;
    float throughput_kbps;
    Time  one_way_dispersion;
    std::vector<int> m_loss_seq;
    std::vector<Ptr<RcvTime>> m_recv_sample;


    Ipv4Address m_peerIP;
    uint16_t m_peerPort;
    uint16_t m_localPort;   
};  // class PacketReceiverUdp

};  // namespace ns3

#endif  /* PACKET_UDP_RECEIVER_H */