#ifndef PACKET_SENDER_TCP_H
#define PACKET_SENDER_TCP_H

#include "packet-sender.h"
#include "ns3/fec-policy.h"
#include "ns3/sender-based-controller.h"
#include "ns3/gcc-controller.h"
#include "ns3/nada-controller.h"

#include "ns3/core-module.h"
#include "ns3/socket.h"
#include "ns3/application.h"
#include "ns3/simulator.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/nstime.h"
#include "ns3/timer.h"
#include <vector>
#include <queue>
#include <memory>
#include <numeric>

namespace ns3 {

class GameServer;

class PacketSenderTcp  : public PacketSender{
public:
    static TypeId GetTypeId (void);
    PacketSenderTcp(GameServer * server, void (GameServer::*)(uint32_t), uint16_t interval, 
        Time delay_ddl, Ipv4Address srcIP, uint16_t srcPort, Ipv4Address destIP, uint16_t destPort, 
        Ipv4Header::DscpType dscp, uint32_t minEncodeBps, uint32_t maxEncodeBps, Ptr<OutputStreamWrapper> bwTrStream);
    ~PacketSenderTcp();

    void StartApplication(Ptr<Node> node);
    void StopRunning();
    void SetController(std::string cca);
    
    void ConnectionSucceeded (Ptr<Socket> socket);
    void ConnectionFailed (Ptr<Socket> socket);
    
    uint32_t kTxRateUpdateWindowMs;

    void CreateFrame(uint32_t frame_id, uint16_t pkt_id, uint16_t data_pkt_max_payload, uint16_t data_pkt_num, uint32_t data_size);

    void UpdateSendingRate(); // apply cc algorithm to calculate the sending bitrate

    uint32_t GetSendingRateBps();

    void UpdateDutyRatio();

private:
    /**
     * \brief Send a packet to the network
     */
    void SendPacket();

    /**
     * \brief calculate the number of frames in the queue
     * \return the number of frames in the queue
     */
    uint32_t num_frame_in_queue();

private:
    std::vector<Ptr<DataPacket>> m_queue; /* Queue storing packets by frames */

    std::deque<int64_t> m_time_history;
    std::deque<uint32_t> m_write_history;
    bool* m_dutyState;
    EventId m_eventUpdateDuty;
    uint32_t m_txBufSize;
    uint32_t m_lastPendingBuf;
    bool m_firstUpdate;

    EventId m_eventSend;
    bool m_cc_create;
    bool m_connected;

    Ptr<OutputStreamWrapper> m_bwTrStream;

    uint32_t kMinEncodeBps;
    uint32_t kMaxEncodeBps;
    double kTargetDutyRatio = 0.75f;
    double kDampingCoef = 0.5f;
    double kProtectDecreaseCoef = 0.1f;
};  // class PacketSenderTcp


};  // namespace ns3

#endif  /* PACKET_TCP_SENDER_H */