#ifndef PACKET_TCP_RECEIVER_H
#define PACKET_TCP_RECEIVER_H

#include "packet-receiver.h"
#include "ns3/fec-policy.h"
#include "ns3/socket.h"
#include "ns3/tcp-socket.h"
#include "ns3/simulator.h"
#include "ns3/nstime.h"
#include "ns3/timer.h"
#include <vector>
#include <queue>

namespace ns3 {

class GameClient;

class PacketReceiverTcp : public PacketReceiver {
public:
    static TypeId GetTypeId (void);
    PacketReceiverTcp(GameClient *, Ptr<VideoDecoder>, uint16_t, uint16_t);
    ~PacketReceiverTcp();
    
    void StartApplication(GameClient* client, Ptr<VideoDecoder> decoder, Time delay_ddl, Ptr<Node> node);
    
    void OnSocketRecv_receiver(Ptr<Socket>);

    void HandleAccept (Ptr<Socket> s, const Address& from);

    void StopRunning();

    void Set_FECgroup_delay(Time& fec_group_delay_);
    Time Get_FECgroup_delay();
    void ReplyACK(std::vector<Ptr<DataPacket>> data_pkts, uint16_t last_pkt_id);
    void SendPacket(Ptr<NetworkPacket> pkt);

private:
    uint32_t BytesToRecv;
    uint32_t frame_id;
    uint32_t frame_size;
    Ptr<Packet> halfpkt;
    Ptr<DataPacketHeader> curHdr;
    bool RecvPktStart;
    uint16_t m_localPort;
    GameClient * game_client;
    Ptr<Socket> m_socket;
    Ptr<VideoDecoder> decoder;
};  // class PacketReceiverTcp

};  // namespace ns3

#endif  /* PACKET_TCP_RECEIVER_H */