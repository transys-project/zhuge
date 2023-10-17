#include "network-queue-zhuge.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NetworkQueueZhuge");

TypeId NetworkQueueZhuge::GetTypeId()
{
    static TypeId tid = TypeId("ns3::NetworkQueueZhuge")
                            .SetParent<NetworkQueue>()
                            .SetGroupName("zhuge")
                            .AddConstructor<NetworkQueueZhuge>();
    return tid;
};

NetworkQueueZhuge::NetworkQueueZhuge ()
: receiver {NULL} 
, m_lastQDelayMs {0}
, m_lastTotalDelayMs {0}
, kAmpduMaxWaitMs {5}
, m_deltaAckDelay {DeltaDelay()}
, kMaxActualDelay {MilliSeconds(10)}    // should be measured as minRtt in practice
, kMinTxRateBps {500 << 10}
, m_curTxRateBps {kMinTxRateBps}
, m_curTxDelayUs {0}
{};

NetworkQueueZhuge::~NetworkQueueZhuge () {};

void NetworkQueueZhuge::Setup (Ipv4Address sendIP, uint16_t sendPort, Ipv4Address recvIP, uint16_t recvPort, 
    bool wirtc, uint16_t interval, bool tcpEnabled, int mode, int qsize, 
    Ptr<NetDevice> sndDev, Ptr<NetDevice> rcvDev, bool fastAckRtx)
{
    m_wirtcEnable = wirtc;
    m_sendIP = sendIP;
    m_sendPort = sendPort;
    m_recvIP = recvIP;
    m_recvPort = recvPort;
    this->cc_create = tcpEnabled;
    this->kTxBytesListInterval = MilliSeconds(1);
    this->last_id = 0;
    m_lastTotalTxBytes = 0;
    this->sendback_ip_hdr.SetSource(recvIP);
    this->sendback_ip_hdr.SetDestination(sendIP);
    this->sendback_ip_hdr.SetProtocol(17);
    this->sendback_ip_hdr.SetTtl(63);
    this->sendback_udp_hdr.SetSourcePort(recvPort);
    this->sendback_udp_hdr.SetDestinationPort(sendPort);
    this->sendback_ppp_hdr.SetProtocol(0x0021);
    this->schedule_flag = false;
    this->last_update = MilliSeconds(0);
    this->last_time = -1;

    kTxBytesListWindow = 5 * interval / kTxBytesListInterval.GetMilliSeconds();
    m_mode = mode;
    m_qsize = qsize;

    GivenSndDev = sndDev;
    GivenRcvDev = rcvDev;
    m_fastAckRtx = fastAckRtx;
    m_LastRcvAck = SequenceNumber32 (1);
};

void NetworkQueueZhuge::DoDispose(){

};

void NetworkQueueZhuge::StartApplication(void)
{
    InitSocket();
};

void NetworkQueueZhuge::StopApplication(void)
{
    m_eventUpdateTxRate.Cancel();
    m_eventSendback.Cancel();
};

void NetworkQueueZhuge::InitSocket()
{
    if (m_mode == 0) {m_rcvDev = StaticCast<PointToPointNetDevice, NetDevice>(GivenRcvDev);}
    else {m_rcvDev = StaticCast<PointToPointNetDevice, NetDevice>(GivenRcvDev);}
    if (m_mode == 0){
        m_sndDev = GivenSndDev;
        Ptr<WifiNetDevice> WifisndDev = StaticCast<WifiNetDevice, NetDevice>(m_sndDev);
        PointerValue ptr;
        BooleanValue qosSupported;
        Ptr<RegularWifiMac> rmac = DynamicCast<RegularWifiMac>(WifisndDev->GetMac());
        rmac->GetAttributeFailSafe("QosSupported", qosSupported);
        std::cout << qosSupported << std::endl;
        rmac->GetAttributeFailSafe("BE_Txop", ptr);
        m_wmq = ptr.Get<QosTxop>()->GetWifiMacQueue();
        m_edca = ptr.Get<QosTxop>();
        m_wmq->SetAttribute("DropPolicy", EnumValue(WifiMacQueue::DROP_OLDEST));
        if (this->cc_create) {
            WifisndDev->SetPromiscReceiveCallback(MakeCallback(&NetworkQueueZhuge::OnRecv_snddev, this));
        }
    }
    else{
        m_sndDev = GivenSndDev;
        Ptr<PointToPointNetDevice> P2psndDev = StaticCast<PointToPointNetDevice, NetDevice>(m_sndDev);
        m_dlQueue =  StaticCast<DropTailQueue<Packet>, Queue<Packet>>(P2psndDev->GetQueue());
        if (this->cc_create) {
            P2psndDev->SetPromiscReceiveCallback(MakeCallback(&NetworkQueueZhuge::OnRecv_snddev, this));
        }
        m_dlQueue->SetMaxSize(QueueSize ("1p")); 
        // m_dlQueue->SetAttribute("MaxSize", QueueSizeValue(QueueSize ("1p")));
    }
    m_rcvDev->SetPromiscReceiveCallback(MakeCallback(&NetworkQueueZhuge::OnRecv_rcvdev, this));
    Ptr<DropTailQueue<Packet>> rcvqueue =  StaticCast<DropTailQueue<Packet>, Queue<Packet>>(m_rcvDev->GetQueue());
    if(m_qsize == 1){rcvqueue->SetMaxSize(QueueSize ("1p"));}
    m_isFirstAck = true;
    m_isFirstPkt = true;
    Ptr<TrafficControlLayer> tc = m_sndDev->GetNode()->GetObject<TrafficControlLayer>();
    m_qdisc = tc->GetRootQueueDiscOnDevice(m_sndDev);
    m_lastHeadId = 0;
    m_curQHeadTimeMs = 0;
    m_eventUpdateTxRate = Simulator::Schedule(kTxBytesListInterval, &NetworkQueueZhuge::UpdateTxRate, this);
};

void NetworkQueueZhuge::UpdateTxRate()
{
    Packet pktinfo;
    uint32_t idx = 0;
    bool haspkt = true;
    if (m_mode == 1){
        if(m_dlQueue->GetNPackets() > 0){
            idx = 0;
            pktinfo = *(m_dlQueue->MyPeek(0));
        }
        else{
            haspkt = false;
        }
    }
    else if (m_mode == 0) {
        if (m_wmq->GetNPackets() > 0) {
            idx = 0;
            while (idx < m_wmq->GetNPackets() && m_wmq->MyPeek(idx)->GetHeader().IsAction()) {
                idx += 1;
            }
            if(idx >= m_wmq->GetNPackets()){
                haspkt = false;
            }
            else{
                pktinfo = *(m_wmq->MyPeek(idx)->GetPacket());
            }
        }
        else{
            haspkt = false;
        }
    }
    else
        NS_FATAL_ERROR ("Unknown mode");
    if (haspkt) {
        Ptr<Packet> pkt_copyinfo = pktinfo.Copy();
        if (m_mode == 1){
            PppHeader ppphdr = PppHeader();
            pkt_copyinfo->RemoveHeader(ppphdr);
        }
        else if (m_mode == 1) {
            LlcSnapHeader snaphdr = LlcSnapHeader();
            pkt_copyinfo->RemoveHeader(snaphdr);
        }
        else
            NS_FATAL_ERROR ("Unknown mode");

        Ipv4Header ipv4Hdr = Ipv4Header ();
        pkt_copyinfo->RemoveHeader (ipv4Hdr);
        uint16_t id = ipv4Hdr.GetIdentification ();
        Ipv4Address ipaddr = ipv4Hdr.GetDestination ();
        if (id != m_lastHeadId || ipaddr != m_lastHeadDstAddr) {
            m_lastHeadId = id;
            m_lastHeadDstAddr = ipaddr;
            m_curQHeadTimeMs = 0;
        }
        else {
            m_curQHeadTimeMs += (uint32_t) kTxBytesListInterval.GetMilliSeconds();
        }       
    }
    else{
        m_curQHeadTimeMs = 0;
    }
    uint32_t curTotalTxBytes;
    bool emptyqueue = false;
    if(m_mode == 0) {
        curTotalTxBytes = m_wmq->GetTotalReceivedBytes() - m_wmq->GetNBytes();
        if(m_wmq->GetNBytes() > 0){emptyqueue = true;}
    }
    else{
        curTotalTxBytes = m_dlQueue->GetTotalReceivedBytes() - m_dlQueue->GetNBytes();
        if(m_dlQueue->GetNBytes() > 0){emptyqueue = true;}
    }
    if (curTotalTxBytes > m_lastTotalTxBytes) {
        m_txBytesList.push_back((int32_t) curTotalTxBytes - m_lastTotalTxBytes);
    }
    else if (emptyqueue) {
        m_txBytesList.push_back(0);
    }
    else {
        m_txBytesList.push_back(-1);
    }
    m_lastTotalTxBytes = curTotalTxBytes;

    while (m_txBytesList.size() > kTxBytesListWindow) {
        m_txBytesList.pop_front();
    }

    uint32_t txBytesSum = 0;
    uint32_t txIdleSlotSum = 0; // idle slot refers to scenarios with an empty queue
    uint32_t txBusySlotSum = 0; // busy slot refers to scenarios where no packets could be sent out (channel busy)
    if (!m_txBytesList.empty()) {
        for (auto it = m_txBytesList.begin(); it != m_txBytesList.end(); it++) {
            if (*it > 0) { txBytesSum += *it; }
            else if (*it == 0) { txBusySlotSum += 1; }
            else { txIdleSlotSum += 1; }
        }
        if (txIdleSlotSum < m_txBytesList.size()) {
            // there are some packets sent out during the last window
            // let's update the dequeue rate
            m_curTxRateBps = 1000 * ((txBytesSum * 8) / 
                (kTxBytesListInterval.GetMilliSeconds() * (kTxBytesListWindow - txIdleSlotSum)));
            if (txBusySlotSum < m_txBytesList.size() - txIdleSlotSum) {
                m_curTxDelayUs = 1000 * (m_txBytesList.size() - txIdleSlotSum) / 
                    (m_txBytesList.size() - txIdleSlotSum - txBusySlotSum);
            }
            else {
                m_curTxDelayUs = kTxBytesListWindow * kTxBytesListInterval.GetMicroSeconds();
            }
        }
        NS_LOG_INFO ("[NetworkQueueZhuge] UpdateTxRate"
            << " Now " << Simulator::Now()
            << " txBytesSum " << txBytesSum 
            << " txBusySlotSum " << txBusySlotSum 
            << " txIdleSlotSum " << txIdleSlotSum 
            << " curTxRateBps " << m_curTxRateBps
            << " kTxBytesListWindow " << kTxBytesListWindow
            << " kTxBytesListInterval " << kTxBytesListInterval
            << " port " <<m_sendPort);
    }
    m_curTxRateBps = std::max(m_curTxRateBps, kMinTxRateBps);
    m_eventUpdateTxRate = Simulator::Schedule(kTxBytesListInterval, &NetworkQueueZhuge::UpdateTxRate, this);
}

bool NetworkQueueZhuge::OnRecv_snddev(Ptr<NetDevice> m_rcvDev, Ptr<const Packet> packet, uint16_t protocol,
                                    const Address &sender, const Address &receiver, NetDevice::PacketType packetType)
{
    Ipv4Header ipHdr;
    packet->PeekHeader(ipHdr);
    if (ipHdr.GetDestination() == m_sendIP && 
        ipHdr.GetProtocol() == 6) {
        Ptr<Packet> parsedPkt = packet->Copy();
        Ptr<Packet> dynamicPkt = ConstCast<Packet> (packet);
        parsedPkt->RemoveHeader(ipHdr);
        TcpHeader tcpHdr;
        parsedPkt->PeekHeader(tcpHdr);

        // the fast Rtx strategy from ap to client
        // if (m_fastAckRtx) {
        //     if (int(tcpHdr.GetFlags()) == 16) {
        //         SequenceNumber32 seq = tcpHdr.GetAckNumber();
        //         if(seq == m_LastRcvAck){
        //             m_DupNumber += 1;
        //             if (m_DupNumber == 3){
        //                 for (uint32_t i = 0; i < m_cache.size(); i++){
        //                     Ptr<Packet> pkt = m_cache[i]->Copy();
        //                     Ipv4Header ipv4header;
        //                     pkt->RemoveHeader(ipv4header);
        //                     TcpHeader tcpheader;
        //                     pkt->PeekHeader(tcpheader);
        //                     SequenceNumber32 seqtemp = tcpheader.GetSequenceNumber();
        //                     if (seqtemp == seq) {
        //                         m_cache[i]->Print(std::cout);
        //                             std::cout << std::endl;
        //                         if (m_mode == 0) {
        //                             Mac48Address sendMac = ("00:00:00:00:00:15");
        //                             m_sndDev->Send(m_cache[i], Address(sendMac), 0x0800);
        //                         }
        //                         else {
        //                             Address sendMac = m_sndDev->GetChannel()->GetDevice(0)->GetAddress();
        //                             Ptr<PointToPointNetDevice> P2psndDev = StaticCast<PointToPointNetDevice, NetDevice>(m_sndDev);
        //                             P2psndDev->SendWithEmergency(m_cache[i], Address(sendMac), 0x0800);                   
        //                         }
        //                         break;
        //                     }
        //                 }                  
        //             }
        //         }
        //         else if (seq > m_LastRcvAck) {
        //             m_DupNumber = 1;
        //             for (uint32_t i = 0; i < m_cache.size(); i++){
        //                 Ptr<Packet> pkt = m_cache[i]->Copy();
        //                 Ipv4Header ipv4header;
        //                 pkt->RemoveHeader(ipv4header);
        //                 TcpHeader tcpheader;
        //                 pkt->PeekHeader(tcpheader);
        //                 SequenceNumber32 seqtemp = tcpheader.GetSequenceNumber() + 
        //                     SequenceNumber32(uint32_t(ipv4header.GetPayloadSize() - 20 - 12));
        //                 if (seqtemp == seq) {
        //                     uint32_t del = 0;
        //                     while (del <= i) {
        //                         m_cache.pop_front();
        //                         del += 1;
        //                     }
        //                     break;
        //                 }
        //             }
        //             if (m_mode == 0) {
        //                 NS_FATAL_ERROR ("Not Implemented!");
        //             }
        //             else {
        //                 Ptr<PointToPointNetDevice> P2psndDev = StaticCast<PointToPointNetDevice, NetDevice>(m_sndDev);
        //                 P2psndDev->SetTcpDropSeq (seq); 
        //             }
        //             m_LastRcvAck = seq;
        //         }
        //     }
        // }

        if (!m_isFirstAck) {
            Time curAckArrv = Simulator::Now();
            Time curAckSent = curAckArrv;
            if (curAckArrv > m_lastAckSent) {
            //     A straightforward understanding: it belongs to a new burst, and now these packets have already
            //     experienced the increased delay. We should not add additional (incorrect) delays.
            //     A better solution would be matching the delay of each packet (marking the sequence number)
            //     yet this is quite computational-expensive and may not achieve the distribution-like goal.
                curAckSent += m_deltaAckDelay.SampleDeltaDelay(Seconds(0));
            }
            else {
                curAckSent += m_lastAckSent - m_lastAckArrv;
                curAckSent += m_deltaAckDelay.SampleDeltaDelay(m_lastAckSent - curAckSent);
            }
            curAckSent = std::min(curAckArrv + kMaxActualDelay, curAckSent);
            NS_LOG_INFO ("[NetworkQueueZhuge] curAckArrv " << curAckArrv << " lastAckSent " << m_lastAckSent <<
                " lastAckArrv " << m_lastAckArrv << " actualDelay " << curAckSent - curAckArrv <<
                " curAckSent " << curAckSent << " port " << m_sendPort);
            m_lastAckArrv = curAckArrv;
            m_lastAckSent = curAckSent;
            if (m_wirtcEnable) {
                if (curAckSent > curAckArrv) {
                    Simulator::Schedule(curAckSent - curAckArrv, &NetworkQueueZhuge::SendPkt, this, dynamicPkt);
                    return true;
                }
            }
        }
        else {
            m_isFirstAck = false;
            m_lastAckArrv = Simulator::Now();
            m_lastAckSent = Simulator::Now();
        }
        Simulator::ScheduleNow(&NetworkQueueZhuge::SendPkt, this, dynamicPkt);
    }
    return true;
}

void NetworkQueueZhuge::SendPkt(Ptr<Packet> packet)
{
    Address sendMac = m_rcvDev->GetChannel()->GetDevice(0)->GetAddress();
    m_rcvDev->Send(packet, sendMac, 0x0800);
}

uint32_t NetworkQueueZhuge::CalculateTotalDelayMs(uint32_t extraByte)
{
    uint32_t curQDelayMs, curQHeadTimeMs, curTxTimeMs, curTotalDelayMs;
    if (extraByte > m_qdisc->GetNBytes()) {
        Packet tailpkt;
        if(m_mode == 0) {
            m_curQSizeByte = m_wmq->GetNBytes();
            for (int idx = m_wmq->GetNPackets() - 1; idx >= 0; idx--)
            {
                tailpkt = *(m_wmq->MyPeek(idx)->GetPacket());
                m_curQSizeByte -= tailpkt.GetSize();
                Ptr<Packet> tailpkt_copy = tailpkt.Copy();
                LlcSnapHeader snaphdr = LlcSnapHeader();
                tailpkt_copy->RemoveHeader(snaphdr);
                Ipv4Header ipv4_hdr = Ipv4Header();
                tailpkt_copy->RemoveHeader(ipv4_hdr);
                UdpHeader udp_hdr;
                tailpkt_copy->PeekHeader(udp_hdr);
                if (ipv4_hdr.GetProtocol() == 17 && udp_hdr.GetSourcePort() == m_sendPort)
                {
                    break;
                }
            }
            curQHeadTimeMs = m_curQHeadTimeMs;
        }
        else {
            m_curQSizeByte = m_dlQueue->GetNBytes();
            for (int idx = m_dlQueue->GetNPackets() - 1; idx >= 0; idx--)
            {
                tailpkt = *(m_dlQueue->MyPeek(idx));
                m_curQSizeByte -= tailpkt.GetSize();
                Ptr<Packet> tailpkt_copy = tailpkt.Copy();
                PppHeader ppphdr = PppHeader();
                tailpkt_copy->RemoveHeader(ppphdr);
                Ipv4Header ipv4_hdr = Ipv4Header();
                tailpkt_copy->RemoveHeader(ipv4_hdr);
                UdpHeader udp_hdr;
                tailpkt_copy->PeekHeader(udp_hdr);
                if (ipv4_hdr.GetProtocol() == 17 && udp_hdr.GetSourcePort() == m_sendPort)
                    break;
            }
            curQHeadTimeMs = 0;
        }
    }
    else {
        m_curQSizeByte = m_qdisc->GetNBytes();
        if (m_mode == 0) {
            m_curQSizeByte += m_wmq->GetNBytes();
            curQHeadTimeMs = m_curQHeadTimeMs;
        }
        else if (m_mode == 1) {
            m_curQSizeByte += m_dlQueue->GetNBytes();
            curQHeadTimeMs = 0;
        }
        else
            NS_FATAL_ERROR ("Not Implemented!");
        m_curQSizeByte = std::max(m_curQSizeByte - extraByte, 0u);
    }

    if (m_mode == 0 &&
        m_curQSizeByte <= (kAmpduMaxWaitMs * m_curTxRateBps / 1000 / 8) &&
        m_curQSizeByte >= m_lastQSizeByte) {
        // current packet might be aggregated within one AMPDU frame with the packet at the head
        // Any delay during the formation of AMPDU frame should be reflected on curTxTimeMs
        // The second condition depicts the scenario where queue is draining with its best efforts,
        // yet there are still remaining packets in the queue. Now queue is impossible to be burst out.
        curQDelayMs = 0;
    }
    else {
        curQDelayMs = 1000 * (double(m_curQSizeByte * 8 ) / double(m_curTxRateBps));
    }
    curTxTimeMs = m_curTxDelayUs / 1000;
    curTotalDelayMs = curQDelayMs + curQHeadTimeMs + curTxTimeMs;
    NS_LOG_DEBUG ("[NetworkQueueZhuge] Time " << (double) Simulator::Now ().GetMicroSeconds () / 1e6
        << " qdiscLen " << m_qdisc->GetNBytes () << " " << m_qdisc->GetNPackets()
        << " dequeueRate(bps) " << m_curTxRateBps
        << " qSize " << m_curQSizeByte
        << " lastTotalDelay(ms) "<< m_lastTotalDelayMs 
        << " curTotalDelay(ms) " << curTotalDelayMs 
        << " m_deltaAckDelay(ms) " << int32_t (curTotalDelayMs - m_lastTotalDelayMs)
        << " qDelay(ms) " << curQDelayMs
        << " headTime(ms) " << curQHeadTimeMs 
        << " txTime(ms) " << curTxTimeMs
        << " lastId " << m_lastHeadId
        << " lastIp " << m_lastHeadDstAddr);
    // std::cout << "[QueueStat] " << Simulator::Now().GetMilliSeconds() 
    //     << " qsize (Byte) " << m_curQSizeByte << '\n';
    return curTotalDelayMs;
}

void NetworkQueueZhuge::SetAckDelay(Ipv4Header ipHdr)
{    
    uint32_t curTotalDelayMs = CalculateTotalDelayMs(0);
    if (m_curQSizeByte == 0 && m_lastQSizeByte == 0) {
        // queue is empty at all, it's time to reset the reverse timer
        // m_deltaAckDelay.Reset();
    }
    else {
        m_deltaAckDelay.AddDeltaDelay(MilliSeconds(int32_t(curTotalDelayMs - m_lastTotalDelayMs)));
    }
    m_lastTotalDelayMs = curTotalDelayMs;
    m_lastQSizeByte = m_curQSizeByte;
}

bool NetworkQueueZhuge::OnRecv_rcvdev(Ptr<NetDevice> m_rcvDev, Ptr<const Packet> packet, uint16_t protocol,
                                    const Address &sender, const Address &receiver, NetDevice::PacketType packetType)
{
    Ipv4Header rcvIpHdr;
    packet->PeekHeader (rcvIpHdr);
    // TODO: support concurrent feedback of multiple flows
    if (!(rcvIpHdr.GetSource() == m_sendIP && rcvIpHdr.GetDestination() == m_recvIP)) {
        if (!this->cc_create) {
            if (this->schedule_flag) {
                this->extra_bytes += packet->GetSize() - 20;    // 20 is Ipv4HeaderSize
            }
            return false;
        }
        else {
            NS_LOG_INFO( "[NetworkQueueZhuge] discarded " << rcvIpHdr.GetSource() << " " << rcvIpHdr.GetDestination()
                << " but we need " << m_sendIP << " " << m_recvIP );
            return true;
        }
    }

    uint16_t id = rcvIpHdr.GetIdentification ();
    Ipv4Address ipaddr = rcvIpHdr.GetDestination ();
    m_lastHeadId = id;
    m_lastHeadDstAddr = ipaddr;
    
    if (this->cc_create) {
        if (rcvIpHdr.GetProtocol() == 6 && rcvIpHdr.GetDestination() == m_recvIP) {
            // TODO: need to justify whether the incoming packet is packet of interest
            Ptr<Packet> pkt_to_set = packet->Copy();
            pkt_to_set->RemoveHeader(rcvIpHdr);
            TcpHeader tcpheader;
            pkt_to_set->PeekHeader(tcpheader);
            if (Simulator::Now().GetSeconds() > 3 && int(tcpheader.GetFlags()) == 16) {
                SetAckDelay(rcvIpHdr);
            }
            if (int(tcpheader.GetFlags()) == 16 && rcvIpHdr.GetPayloadSize() > 20 && m_fastAckRtx) {
                m_cache.push_back(packet->Copy());
            }
            return false;
        }
    }
    else {
        if (rcvIpHdr.GetProtocol() == 17) {
            Ptr<Packet> packet_udp = packet->Copy();
            packet_udp->RemoveHeader(rcvIpHdr);
            UdpHeader rcvUdpHdr;
            packet_udp->PeekHeader(rcvUdpHdr);
            if (rcvUdpHdr.GetSourcePort() != this->m_sendPort) {
                if (this->schedule_flag) {this->extra_bytes += packet_udp->GetSize();}
                return false;
            }
            if (rcvUdpHdr.GetSourcePort() == this->m_sendPort && this->schedule_flag) {
                this->extra_bytes += packet_udp->GetSize();
            }
            if (rcvUdpHdr.GetSourcePort() == this->m_sendPort && !this->schedule_flag) {
                packet_udp->RemoveHeader(rcvUdpHdr);
                this->sndback_pkt = NetworkPacket::ToInstance(packet_udp);
                this->pkt_size = packet_udp->GetSize();
                this->send_id = rcvIpHdr.GetIdentification();
                this->sendaddr = rcvIpHdr.GetDestination();
                this->extra_bytes = 0;
                m_eventSendback = Simulator::Schedule(MilliSeconds(1), &NetworkQueueZhuge::Sendback, this);
                this->schedule_flag = true;
            }
        }
    }
    return true;
}

void NetworkQueueZhuge::Sendback()
{
    this->schedule_flag = false;
    Time time_now = Simulator::Now();
    if (this->m_txBytesList.size() == 0)
        return;
    uint32_t curTotalDelayMs = CalculateTotalDelayMs(this->extra_bytes);

    PacketType pkt_type = this->sndback_pkt->GetPacketType();

    if (pkt_type != DATA_PKT)
        // not data or FEC packets
        return;

    Ptr<VideoPacket> video_pkt = DynamicCast<VideoPacket, NetworkPacket>(this->sndback_pkt);
    /* update network statistics */
    uint32_t id = video_pkt->GetGlobalId();
    uint32_t RxTime = time_now.GetMicroSeconds() + uint32_t(curTotalDelayMs * 1000);
    Ptr<RcvTime> rt = Create<RcvTime>(id, RxTime, this->pkt_size);

    if (id > this->last_id || this->last_id - id > 32768) {
        Ptr<NetStates> netstate = Create<NetStates>(0, m_curTxRateBps >> 10, 0);
        netstate->loss_seq = {};
        netstate->loss_seq.push_back((id + 65536 - this->last_id) % 65536);
        netstate->recvtime_hist = {};
        netstate->recvtime_hist.push_back(rt);

        Ptr<NetStatePacket> nstpacket = Create<NetStatePacket>();
        nstpacket->SetNetStates(netstate);

        Ptr<Packet> netpacket = nstpacket->ToNetPacket();
        netpacket->AddHeader(this->sendback_udp_hdr);
        this->sendback_ip_hdr.SetPayloadSize(netpacket->GetSize());
        netpacket->AddHeader(this->sendback_ip_hdr);

        Address sendMac = m_rcvDev->GetChannel()->GetDevice(0)->GetAddress();
        if (m_wirtcEnable) {
            auto ret = m_rcvDev->Send(netpacket, sendMac, 0x0800);
            NS_LOG_INFO(time_now.GetMilliSeconds() 
                        << ' ' << this->m_sendIP << ' ' 
                        << ret << ' ' << id  
                        << " LastRcvBytes: " << this->m_lastTotalTxBytes 
                        << " dequeue_rate: " << m_curTxRateBps 
                        << " port " << m_sendPort);
        }

        this->last_id = id;
    }
}

}; // namespace ns3
