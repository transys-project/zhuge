#include "game-server.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GameServer");

TypeId GameServer::GetTypeId () {
    static TypeId tid = TypeId ("ns3::GameServer")
        .SetParent<Application> ()
        .SetGroupName("bitrate-ctrl")
        .AddConstructor<GameServer> ()
    ;
    return tid;
};

GameServer::GameServer ()
: m_sender      {NULL}
, m_encoder     {NULL}
, kMinEncodeBps {(uint32_t) 100E3}
, kMaxEncodeBps {(uint32_t) 100E6}
, frame_id      {0}
, group_id      {0}
, batch_id      {0}
, fps           {0}
, bitrate       {0}
, interval      {0}
, delay_ddl     {MilliSeconds(0)}
, m_srcIP       {}
, m_srcPort     {0}
, m_destIP      {}
, m_destPort    {0}
, m_cca         {"Fixed"}
{};

GameServer::~GameServer () {};

void GameServer::Setup (Ipv4Address srcIP, uint16_t srcPort, Ipv4Address destIP, uint16_t destPort,
    Time delayDdl, uint16_t interval, Ptr<FECPolicy> fecPolicy, bool tcpEnabled, Ipv4Header::DscpType dscp, 
    Ptr<OutputStreamWrapper> appTrStream, Ptr<OutputStreamWrapper> bwTrStream) 
{
    // std::cout << "[GameServer] Server ip: " << srcIP << ", server port: " << srcPort << ", Client ip: " << destIP <<  ", dstPort: " << destPort << '\n';
    m_srcIP = srcIP;
    m_srcPort = srcPort;
    m_destIP = destIP;
    m_destPort = destPort;
    // init modules
    m_encoder = Create<DumbVideoEncoder> (1000 / interval, bitrate, this, &GameServer::SendFrame);
    if (tcpEnabled) {
        m_sender = Create<PacketSenderTcp> (this, &GameServer::UpdateBitrate, interval, delayDdl, 
            srcIP, srcPort, destIP, destPort, dscp, kMinEncodeBps, kMaxEncodeBps, bwTrStream); 
    }
    else {
        m_sender = Create<PacketSenderUdp> (this, &GameServer::UpdateBitrate, fecPolicy, srcIP, srcPort, 
            destIP, destPort, dscp, fps, delayDdl, kMinEncodeBps / 1000, interval, bwTrStream);
    }
    this->send_frame_cnt = 0;
    m_appTrStream = appTrStream;
};

void GameServer::SetController (std::string cca) {
    m_cca = cca;
}

void GameServer::DoDispose () {};

void GameServer::StartApplication () {
    m_sender->StartApplication (GetNode());
    m_sender->SetController (m_cca);
    m_encoder->StartEncoding ();
};


void GameServer::StopApplication () {
    NS_LOG_ERROR("\n\n[Server] Stopping GameServer...");
    m_sender->StopRunning ();
};


void GameServer::StopEncoding () {
    m_encoder->StopEncoding ();
};

uint32_t GameServer::GetNextFrameId () { return this->frame_id ++; };


void GameServer::SendFrame (uint8_t * buffer, uint32_t data_size) {
    if (data_size == 0)
        return;
    
    this->send_frame_cnt ++;

    /* 1. Create data packets of a frame */
    // calculate the num of data packets needed
    uint32_t frame_id = GetNextFrameId ();
    uint16_t pkt_id = 0;
    uint16_t data_pkt_max_payload = DataPacket::GetMaxPayloadSize();
    uint16_t data_pkt_num = data_size / data_pkt_max_payload
         + (data_size % data_pkt_max_payload != 0);    /* ceiling division */
    *m_appTrStream->GetStream () << Simulator::Now().GetMilliSeconds() << 
        " FlowId " << m_srcPort <<
        " Send frame " << frame_id << std::endl;
    m_sender->CreateFrame (frame_id, pkt_id, data_pkt_max_payload, data_pkt_num, data_size);
}

void GameServer::UpdateBitrate(uint32_t encodeRateBps){
    encodeRateBps = std::max (encodeRateBps, kMinEncodeBps);
    encodeRateBps = std::min (encodeRateBps, kMaxEncodeBps);
    m_encoder->SetBitrate (encodeRateBps >> 10);
};

void GameServer::OutputStatistics() {
    NS_LOG_ERROR("[Server] Total Frames: " << this->send_frame_cnt);
    NS_LOG_ERROR("[Server] [Result] Total Frames: " << this->send_frame_cnt - 10);
    NS_LOG_ERROR("[Server] Total Groups: " << this->send_group_cnt);
}

}; // namespace ns3