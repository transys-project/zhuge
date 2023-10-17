#include "packet-sender-tcp.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE("PacketSenderTcp");

TypeId PacketSenderTcp::GetTypeId() {
    static TypeId tid = TypeId ("ns3::PacketSenderTcp")
        .SetParent<Object> ()
        .SetGroupName("bitrate-ctrl")
    ;
    return tid;
};

PacketSenderTcp::PacketSenderTcp (
    GameServer * gameServer, void (GameServer::*EncodeFunc)(uint32_t encodeBitrate), uint16_t interval,
    Time delayDdl, Ipv4Address srcIP, uint16_t srcPort, Ipv4Address dstIP, uint16_t dstPort, Ipv4Header::DscpType dscp, 
    uint32_t minEncodeBps, uint32_t maxEncodeBps, Ptr<OutputStreamWrapper> bwTrStream
)
: PacketSender (gameServer, EncodeFunc, MilliSeconds(40), srcIP, srcPort, dstIP, dstPort, dscp)
, kMinEncodeBps{minEncodeBps}
, kMaxEncodeBps{maxEncodeBps}
{
    m_firstUpdate = true;
    m_lastPendingBuf = 0;
    m_bwTrStream = bwTrStream;
};

PacketSenderTcp::~PacketSenderTcp() {};

void PacketSenderTcp::StartApplication(Ptr<Node> node) {
    if (m_socket == NULL) {
        m_socket = Socket::CreateSocket(node, TcpSocketFactory::GetTypeId());
        Ptr<TcpSocket> tcpSocket = DynamicCast<TcpSocket>(m_socket);
        UintegerValue val;
        tcpSocket->GetAttribute ("SndBufSize", val);
        m_txBufSize = (uint32_t) val.Get();

        InetSocketAddress local = InetSocketAddress(m_srcIP, m_srcPort);
        InetSocketAddress remote = InetSocketAddress(m_destIP, m_destPort);
        local.SetTos (m_dscp << 2);
        remote.SetTos (m_dscp << 2);
        m_socket->Bind(local);
        m_socket->Connect(remote);
        m_socket->ShutdownRecv ();
        m_socket->SetConnectCallback (
            MakeCallback (&PacketSenderTcp::ConnectionSucceeded, this),
            MakeCallback (&PacketSenderTcp::ConnectionFailed, this)
        );
    }
    PacketSender::StartApplication(node);
    m_eventUpdateDuty = Simulator::Schedule(MilliSeconds(1), &PacketSenderTcp::UpdateDutyRatio, this);
};

void PacketSenderTcp::StopRunning(){
    PacketSender::StopRunning();
    m_socket->Close();
    m_eventUpdateDuty.Cancel();
}

void PacketSenderTcp::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC ("Tcp Connection succeeded");
  m_connected = true;
  Address from, to;
  m_socket->GetSockName (from);
  m_socket->GetPeerName (to);
}

void PacketSenderTcp::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC ("Tcp Connection Failed");
}

void PacketSenderTcp::SetController(std::string cca) {
    m_cc_enable = true;
    m_cc_create = false;
    kTxRateUpdateWindowMs = (uint32_t) m_cc_interval.GetMilliSeconds();
    m_dutyState = new bool[kTxRateUpdateWindowMs]();
    ObjectFactory congestionAlgorithmFactory;
    TcpSocketBase *base = static_cast<TcpSocketBase*> (PeekPointer(m_socket));
    congestionAlgorithmFactory.SetTypeId (TypeId::LookupByName ("ns3::Tcp" + cca));
    std::cout << "[PacketSenderTcp] using " << congestionAlgorithmFactory.GetTypeId().GetName() << '\n';
    Ptr<TcpCongestionOps> algo = congestionAlgorithmFactory.Create<TcpCongestionOps> ();
    base->SetCongestionControlAlgorithm (algo);
}

void PacketSenderTcp::CreateFrame(uint32_t frame_id, uint16_t pkt_id, uint16_t data_pkt_max_payload, 
    uint16_t data_pkt_num, uint32_t data_size) {
    uint32_t frame_payload = data_size;
    data_pkt_num = 1;
    Ptr<DataPacket> data_pkt = Create<DataPacket>(frame_id, frame_payload, data_pkt_num, pkt_id);
    data_pkt->SetEncodeTime(Simulator::Now());
    data_pkt->SetPayload(nullptr, frame_payload);
    m_queue.push_back(data_pkt);
    m_eventSend = Simulator::ScheduleNow(&PacketSenderTcp::SendPacket,this);
}

void PacketSenderTcp::SendPacket()
{
    Time time_now = Simulator::Now();
    if (num_frame_in_queue() > 0) {
        std::vector<Ptr<DataPacket>>::iterator first_frame = m_queue.begin();
        Ptr<Packet> pkt_to_send = (*first_frame)->ToTcpSendPacket();
        if (m_connected) {
            int sendif = m_socket->Send(pkt_to_send);
            if (sendif > 0) {
                m_queue.erase(m_queue.begin());
                m_time_history.push_back(Simulator::Now().GetMilliSeconds());
                m_write_history.push_back(sendif);
                m_eventSend = Simulator::ScheduleNow(&PacketSenderTcp::SendPacket,this);
            }
        }
    }
};

void PacketSenderTcp::UpdateDutyRatio () {
    Ptr<TcpSocketBase> tcpsocketbase = DynamicCast<TcpSocketBase>(m_socket);
    // uint32_t inflight = tcpsocketbase->GetTxBuffer()->BytesInFlight ();
    uint32_t sentsize = tcpsocketbase->GetTxBuffer()->GetSentSize();
    uint32_t txbuffer = m_socket->GetTxAvailable ();
    uint16_t nowIndex = Simulator::Now().GetMilliSeconds() % kTxRateUpdateWindowMs;
    m_dutyState[nowIndex] = ( sentsize + txbuffer < m_txBufSize);
    m_eventUpdateDuty = Simulator::Schedule(MilliSeconds(1), &PacketSenderTcp::UpdateDutyRatio, this);
}

void PacketSenderTcp::UpdateSendingRate () {
    Ptr<TcpSocketBase> tcpSocket = DynamicCast<TcpSocketBase>(m_socket);
    // uint32_t inflight = tcpsocketbase->GetTxBuffer()->BytesInFlight ();
    uint32_t sentsize = tcpSocket->GetTxBuffer()->GetSentSize();
    uint32_t txbufferavailble = m_socket->GetTxAvailable ();
    uint32_t curPendingBuf = m_txBufSize - sentsize - txbufferavailble;
    int32_t deltaPendingBuf = (int32_t) curPendingBuf - (int32_t) m_lastPendingBuf;
    int64_t time_now = Simulator::Now().GetMilliSeconds();
    uint32_t totalWriteBytes = 0;
    if (!m_time_history.empty()) {
        while ((!m_time_history.empty()) && ((time_now - m_time_history.front()) > kTxRateUpdateWindowMs)){
            m_time_history.pop_front();
            m_write_history.pop_front();
        }
    }
    if (!m_write_history.empty()) {
        totalWriteBytes = std::accumulate(m_write_history.begin(), m_write_history.end(), 0);
    }
    if (m_firstUpdate) {
        m_bitrate = kMinEncodeBps;
        m_firstUpdate = false;
        m_lastPendingBuf = curPendingBuf;
    }
    else {
        double dutyRatio = double(std::accumulate(m_dutyState, m_dutyState + kTxRateUpdateWindowMs, 0)) / 
            double(kTxRateUpdateWindowMs);
        uint32_t totalSendBytes = totalWriteBytes - deltaPendingBuf;
        uint32_t lastSendingRateBps = (totalWriteBytes - deltaPendingBuf) * 1000 * 8 / kTxRateUpdateWindowMs;
        if (curPendingBuf > 0) {
            m_bitrate = uint32_t(kTargetDutyRatio * std::min((double) m_bitrate, 
                std::max(0.1, 1 - (double) curPendingBuf / totalSendBytes) * lastSendingRateBps));
        }
        else {
            m_bitrate -= kDampingCoef * (dutyRatio - kTargetDutyRatio) * m_bitrate;
        }
        m_bitrate = std::min(m_bitrate, kMaxEncodeBps);
        m_bitrate = std::max(m_bitrate, kMinEncodeBps);

        // printf("[PacketSenderTcp] dutyRatio %f txSend %u txThreshold %u nowBuffer %u lastTxBuffer %u sentsize %u "
        //     "available %u bitrate %u\n", 
        //     dutyRatio, totalSendBytes, kTxRateUpdateWindowMs, curPendingBuf, m_lastPendingBuf, sentsize, 
        //     txbufferavailble, m_bitrate);
        // printf("[PacketSenderTcp] Time %u RttNet(ms) %u Bitrate(bps) %u Cwnd (byte) %u Rtt(ms) %u\n", 
        //     (uint32_t) Simulator::Now().GetMilliSeconds(), 
        //     (uint32_t) tcpsocketbase->GetRttNet()->GetEstimate().GetMilliSeconds(), 
        //     lastSendingRateBps, tcpsocketbase->GetCwndByte(),
        //     (uint32_t) tcpsocketbase->GetRtt()->GetEstimate().GetMilliSeconds());
        *m_bwTrStream->GetStream () << Simulator::Now ().GetMilliSeconds () << 
            " FlowId " << m_srcPort << 
            " Bitrate(bps) " << lastSendingRateBps << 
            " Rtt(ms) " << (uint32_t) tcpSocket->GetRtt ()->GetEstimate().GetMilliSeconds() <<
            " Cwnd(pkts) " << tcpSocket->GetCwndByte () << 
            " nowBuf " << curPendingBuf << std::endl;
        m_lastPendingBuf = curPendingBuf;
    }
};

uint32_t PacketSenderTcp::num_frame_in_queue()
{
    return m_queue.size();
};

}; // namespace ns3
