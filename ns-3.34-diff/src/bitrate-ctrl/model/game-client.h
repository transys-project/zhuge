#ifndef GAME_CLIENT_H
#define GAME_CLIENT_H

#include "ns3/fec-policy.h"
#include "packet-receiver.h"
#include "packet-receiver-tcp.h"
#include "packet-receiver-udp.h"
#include "video-decoder.h"
#include "ns3/application.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/simulator.h"
#include "ns3/assert.h"
#include "ns3/socket.h"
#include <unordered_map>

namespace ns3 {

class GameClient : public Application {

public:
    static TypeId GetTypeId (void);
    GameClient();
    ~GameClient();
    void Setup(
        Ipv4Address srcIP, uint16_t srcPort,uint16_t destPort, uint16_t interval,
        Time delay_ddl, bool cc_create);
    bool wirtc;
    bool m_cc_create;
    uint16_t interval;

protected:
    void DoDispose(void);

private:
    void StartApplication(void);
    void StopApplication(void);

private:
    Ptr<PacketReceiver> receiver;
    Ptr<VideoDecoder> decoder;
    Ptr<FECPolicy> policy;

    uint8_t fps;        /* video fps */
    Time delay_ddl;     /* delay ddl */
    Time rtt;
    Time default_rtt;

    Ipv4Address m_peerIP;
    uint16_t m_peerPort;
    uint16_t m_localPort;

    uint32_t m_receiver_window; /* size of receiver's sliding window (in ms) */

    // measure rtt
    static const uint8_t RTT_WINDOW_SIZE = 10;
    std::deque<uint64_t> rtt_window;    /* in us */
    std::deque<Ptr<GroupPacketInfo>> rtx_history_key;     /* in the order of time */
    std::unordered_map<
        uint32_t, std::unordered_map<uint16_t, 
            std::unordered_map<uint8_t, Time>>> rtx_history;  /* {GroupId: {pkt_id: {tx_count : send_time, ...}, ...}, ...} */

    /**
     * @brief Reply Frame ACK for DMR calculation
     */
    void ReplyFrameACK(uint32_t, Time);

public:

};  // class GameClient

};  // namespace ns3

#endif  /* GAME_CLIENT_H */