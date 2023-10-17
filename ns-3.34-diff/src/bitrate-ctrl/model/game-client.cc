#include "game-client.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("GameClient");

TypeId GameClient::GetTypeId() {
    static TypeId tid = TypeId ("ns3::GameClient")
        .SetParent<Application> ()
        .SetGroupName("bitrate-ctrl")
        .AddConstructor<GameClient>()
    ;
    return tid;
};

GameClient::GameClient()
: receiver{NULL}
, decoder{NULL}
, fps{0}
, m_peerIP{}
, m_peerPort{0}
, m_localPort{0}
, m_receiver_window{34000}
{};

GameClient::~GameClient() {

};

void GameClient::Setup(Ipv4Address srcIP, uint16_t srcPort, uint16_t destPort, uint16_t interval, Time delay_ddl, bool cc_create) {
    this->m_peerIP = srcIP;
    this->m_peerPort = srcPort;
    this->m_localPort = destPort;
    this->fps = 1000 / interval;
    this->decoder = Create<VideoDecoder> (delay_ddl, fps, this, &GameClient::ReplyFrameACK);
    this->delay_ddl = delay_ddl;
    this->interval = interval;
    this->m_cc_create = cc_create;
    if (cc_create) {
        this->receiver = Create<PacketReceiverTcp> (this, this->decoder, this->interval, this->m_localPort);
    }
    else {
        this->receiver = Create<PacketReceiverUdp> (m_receiver_window, this->interval, this->m_peerIP, this->m_peerPort, this->m_localPort);
    }
};

void GameClient::DoDispose() {

};

void GameClient::StartApplication(void) {
    std::cout<<"receiver start"<<std::endl;
    this->receiver->StartApplication(this, this->decoder, this->delay_ddl, GetNode());
};

void GameClient::StopApplication(void) {
    NS_LOG_ERROR("\n[Client] Stopping GameClient...");
    this->receiver->StopRunning();
};

void GameClient::ReplyFrameACK(uint32_t frame_id, Time frame_encode_time) {
    Ptr<FrameAckPacket> pkt = Create<FrameAckPacket>(frame_id, frame_encode_time);
    if(!this->m_cc_create){
        this->receiver->SendPacket(pkt);
    }
};

}; // namespace ns3