#include "packet-sender.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE("PacketSender");

TypeId PacketSender::GetTypeId() {
    static TypeId tid = TypeId ("ns3::PacketSender")
        .SetParent<Object> ()
        .SetGroupName("bitrate-ctrl")
    ;
    return tid;
};

PacketSender::PacketSender (
    GameServer * gameServer, void (GameServer::*EncodeFunc)(uint32_t encode_bitrate), Time cc_interval,
    Ipv4Address srcIP, uint16_t srcPort, Ipv4Address destIP, uint16_t destPort, Ipv4Header::DscpType dscp
)
: m_gameServer {gameServer}
, EncodeFunc {EncodeFunc}
, m_socket {NULL}
, m_srcIP {srcIP}
, m_srcPort {srcPort}
, m_destIP {destIP}
, m_destPort {destPort}
, m_dscp {dscp}
, m_cc_enable {true}
, m_cc_interval {cc_interval}
{
    std::cout << "[PacketSender] Init " << srcIP << " " << destIP << " " << m_srcIP << " " << m_destIP << '\n';
    m_cc_timer = Timer::CANCEL_ON_DESTROY;
};

PacketSender::~PacketSender() {};

uint32_t PacketSender::GetSendingRateBps() {
    return m_bitrate;
};

void PacketSender::UpdateBitrate() {
    if (!m_cc_enable) return;
    UpdateSendingRate();
    uint32_t bitrateBps = GetSendingRateBps();
    ((m_gameServer)->*EncodeFunc) (bitrateBps);
    m_cc_timer.Schedule();
};

void PacketSender::StartApplication(Ptr<Node> node) {
    m_cc_timer.SetFunction(&PacketSender::UpdateBitrate, this);
    m_cc_timer.SetDelay(m_cc_interval);
    m_cc_timer.Schedule();
}

void PacketSender::StopRunning(){
    if (m_cc_timer.IsRunning()) {
        m_cc_timer.Cancel();
    }
}

TypeId PacketFrame::GetTypeId()
{
    static TypeId tid = TypeId ("ns3::PacketFrame")
        .SetParent<Object> ()
        .SetGroupName("bitrate-ctrl")
    ;
    return tid;
};

PacketFrame::PacketFrame(std::vector<Ptr<VideoPacket>> packets, bool retransmission)
{
    this->packets_in_Frame.assign(packets.begin(), packets.end());
    this->retransmission = retransmission;
};

PacketFrame::~PacketFrame() {};

uint32_t PacketFrame::Frame_size_in_byte()
{
    return this->packets_in_Frame.size() * this->per_packet_size;
};

uint32_t PacketFrame::Frame_size_in_packet()
{
    return this->packets_in_Frame.size();
};

SentPacketInfo::SentPacketInfo(uint16_t id, uint16_t batch_id, Time sendtime, PacketType type, bool isgoodput, uint16_t size){
    this->pkt_id = id;
    this->batch_id = batch_id;
    this->pkt_send_time = sendtime;
    this->pkt_ack_time = MicroSeconds(0);
    this->pkt_type = type;
    this->is_goodput = isgoodput;
    this->pkt_size = size;
};

SentPacketInfo::~SentPacketInfo() {};

}; // namespace ns3