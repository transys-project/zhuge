#ifndef PACKET_RECEIVER_H
#define PACKET_RECEIVER_H

#include "ns3/fec-policy.h"
#include "video-decoder.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/nstime.h"
#include "ns3/timer.h"
#include <vector>
#include <queue>

namespace ns3 {

class GameClient;

class PacketReceiver : public Object {
public:
    static TypeId GetTypeId (void);
    PacketReceiver();
    ~PacketReceiver();

    virtual void SendPacket(Ptr<NetworkPacket> pkt) = 0;
    virtual void Set_FECgroup_delay(Time& fec_group_delay_) = 0;
    virtual Time Get_FECgroup_delay() = 0;
    virtual void StartApplication(GameClient* client, Ptr<VideoDecoder> decoder, Time delay_ddl, Ptr<Node> node);
    virtual void StopRunning() = 0;
    virtual void ReplyACK(std::vector<Ptr<DataPacket>> data_pkts, uint16_t last_pkt_id) = 0;


protected:
    GameClient * game_client;
    Ptr<Socket> m_socket;
    Ptr<VideoDecoder> decoder;
    Time delay_ddl;     /* delay ddl */

};  // class PacketReceiver

};  // namespace ns3

#endif  /* PACKET_RECEIVER_H */