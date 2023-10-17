#include "packet-receiver-tcp.h"
#include "game-client.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("PacketReceiverTcp");

TypeId PacketReceiverTcp::GetTypeId() {
    static TypeId tid = TypeId ("ns3::PacketReceiverTcp")
        .SetParent<PacketReceiver> ()
        .SetGroupName("bitrate-ctrl")
    ;
    return tid;
};

PacketReceiverTcp::PacketReceiverTcp (GameClient * game_client, Ptr<VideoDecoder> decoder, uint16_t interval, uint16_t local_port)
: PacketReceiver{}
, game_client{game_client}
{
    this->RecvPktStart = true;
    this->m_localPort = local_port;
    this->decoder = decoder;
    this->curHdr = Create<DataPacketHeader> ();
};

PacketReceiverTcp::~PacketReceiverTcp () {};

void PacketReceiverTcp::StartApplication(GameClient* client, Ptr<VideoDecoder> decoder, Time delay_ddl, Ptr<Node> node) {
    PacketReceiver::StartApplication(client, decoder, delay_ddl, node);
    if (this->m_socket == NULL) {
        this->m_socket = Socket::CreateSocket(node, TcpSocketFactory::GetTypeId());
        auto res = this->m_socket->Bind(InetSocketAddress{Ipv4Address::GetAny(),this->m_localPort});
        Ptr<TcpSocket> tcpsocket = DynamicCast<TcpSocket>(this->m_socket);
        NS_ASSERT (res == 0);
        this->m_socket->Listen();
        this->m_socket->ShutdownSend();
        this->m_socket->SetRecvCallback(MakeCallback(&PacketReceiverTcp::OnSocketRecv_receiver,this));
        this->m_socket->SetAcceptCallback (MakeNullCallback<bool, Ptr<Socket>, const Address &> (),MakeCallback (&PacketReceiverTcp::HandleAccept, this));
    }
}
void PacketReceiverTcp::StopRunning(){
    this->m_socket->Close();
}

void PacketReceiverTcp::HandleAccept (Ptr<Socket> s, const Address& from)
{
  NS_LOG_FUNCTION (this << s << from);
  s->SetRecvCallback (MakeCallback (&PacketReceiverTcp::OnSocketRecv_receiver, this));
}

void PacketReceiverTcp::OnSocketRecv_receiver(Ptr<Socket> socket)
{
    Ptr<Packet> pktPayload;
    Time time_now = Simulator::Now();
    std::vector<Ptr<DataPacket>> pktsReady;
    if (this->RecvPktStart) {
        Ptr<Packet> pkt = socket->Recv(12, false);
        this->halfpkt = pkt->Copy();
        pkt->RemoveHeader(*curHdr);
        this->frame_id = curHdr->GetFrameId();
        uint32_t frame_size = curHdr->GetFrameSize();
        this->frame_size = frame_size;
        pktPayload = socket->Recv(frame_size, false);
        this->halfpkt->AddAtEnd(pktPayload);
        if (pktPayload->GetSize() < frame_size) {
            // Not ready for decoder
            this->BytesToRecv = frame_size - pktPayload->GetSize();
            this->RecvPktStart = false;
            // std::cout<<"[Packet Receiver] "<<"BytesToRecv: "<<this->BytesToRecv<<std::endl;
        }
    }
    else {
        pktPayload = socket->Recv(this->BytesToRecv, false);
        this->halfpkt->AddAtEnd(pktPayload);
        if (pktPayload->GetSize() < this->BytesToRecv) {
            this->BytesToRecv = this->BytesToRecv - pktPayload->GetSize();
            // std::cout<<"[Packet Receiver] "<<"BytesToRecv: "<<this->BytesToRecv<<std::endl;
        }
        else {
            this->RecvPktStart = true;
            this->BytesToRecv = 0;
            // Ready for decoder
        }
    }

    if (this->RecvPktStart) {
        this->halfpkt->AddHeader(*curHdr);
        pktsReady.push_back(Create<DataPacket> (curHdr->GetFrameId(), curHdr->GetFrameSize(), 
            curHdr->GetFramePktNum(), curHdr->GetPktIdInFrame()));
        this->decoder->DecodeDataPacket(pktsReady);
    }
};

void PacketReceiverTcp::Set_FECgroup_delay(Time& fec_group_delay_){};
Time PacketReceiverTcp::Get_FECgroup_delay(){return Seconds(0);};
void PacketReceiverTcp::ReplyACK(std::vector<Ptr<DataPacket>> data_pkts, uint16_t last_pkt_id){};
void PacketReceiverTcp::SendPacket(Ptr<NetworkPacket> pkt){};
}; // namespace ns3