#ifndef PACKET_SENDER_H
#define PACKET_SENDER_H

#include "utils.h"
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

namespace ns3 {

class GameServer;

class SentPacketInfo : public Object {
public:
    SentPacketInfo(uint16_t,uint16_t,Time,PacketType,bool,uint16_t);
    ~SentPacketInfo();
    uint16_t pkt_id;
    uint16_t batch_id;
    Time pkt_send_time;
    Time pkt_ack_time;
    PacketType pkt_type;
    bool is_goodput;
    uint16_t pkt_size; //in bytes
}; //class SentPacketInfo

class PacketFrame : public Object{
public:
    static TypeId GetTypeId (void);
    PacketFrame(std::vector<Ptr<VideoPacket>> packets, bool retransmission);
    ~PacketFrame();

    std::vector<Ptr<VideoPacket>> packets_in_Frame;

    Time Frame_encode_time_; /*the encodde time of the packets in this frame */

    uint32_t per_packet_size; /* bytes in each packet */

    bool retransmission; /* whether the packets are retransmission packets */

    /**
     * \brief calculate the total bytes in this frame
     */
    uint32_t Frame_size_in_byte();

    /**
     * \brief calculate the total number of packets in this frame
     */
    uint32_t Frame_size_in_packet();

}; // class PacketFrame


class PacketSender : public Object{
public:
    static TypeId GetTypeId (void);
    PacketSender (GameServer * server, void (GameServer::*)(uint32_t), Time cc_interval,
        Ipv4Address srcIP, uint16_t srcPort, Ipv4Address destIP, uint16_t destPort, Ipv4Header::DscpType dscp);
    ~PacketSender ();

    virtual void StartApplication(Ptr<Node> node) = 0;

    virtual void SetController(std::string cca) = 0;
    uint32_t GetSendingRateBps();
    virtual void UpdateSendingRate() = 0; // apply cc algorithm to calculate the sending bitrate

    void UpdateBitrate();

    virtual void CreateFrame(uint32_t frame_id, uint16_t pkt_id, uint16_t data_pkt_max_payload, uint16_t data_pkt_num, uint32_t data_size) = 0;
    
    virtual void StopRunning()=0;
    
protected:
    GameServer * m_gameServer;
    void (GameServer::*EncodeFunc)(uint32_t encodeBitrateBps);
    Ptr<Socket> m_socket; 

    Ipv4Address m_srcIP;
    uint16_t m_srcPort;
    Ipv4Address m_destIP;
    uint16_t m_destPort;
    Ipv4Header::DscpType m_dscp;

    uint32_t m_bitrate;
    bool m_cc_enable;
    Timer m_cc_timer;
    Time m_cc_interval;
};  // class PacketSender


};  // namespace ns3

#endif  /* PACKET_SENDER_H */