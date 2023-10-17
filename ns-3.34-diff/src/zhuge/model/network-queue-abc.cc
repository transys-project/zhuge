#include "network-queue-abc.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NetworkQueueAbc");

TypeId NetworkQueueAbc::GetTypeId()
{
    static TypeId tid = TypeId("ns3::NetworkQueueAbc")
                            .SetParent<NetworkQueue>()
                            .SetGroupName("zhuge")
                            .AddConstructor<NetworkQueueAbc>();
    return tid;
};

NetworkQueueAbc::NetworkQueueAbc ()
: receiver {NULL} 
, kMinTxRateBps {500 << 10}
, m_curTxRateBps {kMinTxRateBps}
, m_curTxDelayUs {0}
{};

NetworkQueueAbc::~NetworkQueueAbc () {};

void NetworkQueueAbc::Setup (Ipv4Address sendIP, uint16_t sendPort, Ipv4Address recvIP, uint16_t recvPort, 
    uint16_t interval, bool cc_create, int mode)
{
    m_wirtcEnable = false;
    m_sendIP = sendIP;
    m_sendPort = sendPort;
    m_recvIP = recvIP;
    m_recvPort = recvPort;
    this->cc_create = cc_create;
    this->kTxBytesListInterval = MilliSeconds(1);
    m_lastTotalTxBytes = 0;
    kTxBytesListWindow = interval / kTxBytesListInterval.GetMilliSeconds();
    m_mode = mode;
};

void NetworkQueueAbc::DoDispose(){

};

void NetworkQueueAbc::StartApplication(void)
{
    InitSocket();
};

void NetworkQueueAbc::StopApplication(void)
{
    m_eventUpdateTxRate.Cancel();
};

void NetworkQueueAbc::InitSocket()
{
    if (m_mode == 0) {m_rcvDev = StaticCast<PointToPointNetDevice, NetDevice>(GetNode()->GetDevice(0));}
    else {m_rcvDev = StaticCast<PointToPointNetDevice, NetDevice>(GetNode()->GetDevice(1));}
    if (m_mode == 0){
        m_sndDev = GetNode()->GetDevice(1);
        int32_t index = m_sndDev->GetNode()->GetObject<Ipv4L3Protocol>()->GetInterfaceForDevice(GetNode()->GetDevice(1));
        m_interface = m_sndDev->GetNode()->GetObject<Ipv4L3Protocol>()->GetInterface(index);
        m_interface->SetABCMode(true, m_recvIP);
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
            WifisndDev->SetPromiscReceiveCallback(MakeCallback(&NetworkQueueAbc::OnRecv_snddev, this));
        }
    }
    else{
        m_sndDev = GetNode()->GetDevice(2);
        Ptr<PointToPointNetDevice> P2psndDev = StaticCast<PointToPointNetDevice, NetDevice>(m_sndDev);
        m_dlQueue =  StaticCast<DropTailQueue<Packet>, Queue<Packet>>(P2psndDev->GetQueue());
        if (this->cc_create) {
            P2psndDev->SetPromiscReceiveCallback(MakeCallback(&NetworkQueueAbc::OnRecv_snddev, this));
        }
        int32_t index = m_sndDev->GetNode()->GetObject<Ipv4L3Protocol>()->GetInterfaceForDevice(GetNode()->GetDevice(2));
        m_interface = m_sndDev->GetNode()->GetObject<Ipv4L3Protocol>()->GetInterface(index);
        m_interface->SetABCMode(true, m_recvIP);
    }
    m_rcvDev->SetPromiscReceiveCallback(MakeCallback(&NetworkQueueAbc::OnRecv_rcvdev, this));
    Ptr<TrafficControlLayer> tc = m_sndDev->GetNode()->GetObject<TrafficControlLayer>();
    m_qdisc = tc->GetRootQueueDiscOnDevice(m_sndDev);
    m_lastHeadId = 0;
    m_curQHeadTimeMs = 0;
    m_eventUpdateTxRate = Simulator::Schedule(kTxBytesListInterval, &NetworkQueueAbc::UpdateTxRate, this);
};

void NetworkQueueAbc::UpdateTxRate()
{
    Packet pktinfo;
    uint32_t idx = 0;
    bool haspkt = true;
    if(m_mode == 1){
        if(m_dlQueue->GetNPackets() > 0){
            idx = 0;
            pktinfo = *(m_dlQueue->MyPeek(0));
        }
        else{
            haspkt = false;
        }
    }
    else{
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
    if(haspkt){
        Ptr<Packet> pkt_copyinfo = pktinfo.Copy();
        if(m_mode == 1){
            PppHeader ppphdr = PppHeader();
            pkt_copyinfo->RemoveHeader(ppphdr);
        }
        else{
            LlcSnapHeader snaphdr = LlcSnapHeader();
            pkt_copyinfo->RemoveHeader(snaphdr);
        }

        Ipv4Header ipv4_hdr = Ipv4Header();
        pkt_copyinfo->RemoveHeader(ipv4_hdr);
        uint16_t id = ipv4_hdr.GetIdentification();
        Ipv4Address ipaddr = ipv4_hdr.GetDestination();
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
            // std::cout << " " << *it;
            if (*it > 0) { txBytesSum += *it; }
            else if (*it == 0) { txBusySlotSum += 1; }
            else { txIdleSlotSum += 1; }
        }
        // std::cout << '\n';
        if (txIdleSlotSum < m_txBytesList.size()) {
            // there are some packets sent out during the last window
            // let's update the dequeue rate
            m_curTxRateBps = 1000 * ((txBytesSum * 8) / 
                (kTxBytesListInterval.GetMilliSeconds() * (kTxBytesListWindow - txIdleSlotSum)));
            m_curTxTransRateBps = 1000 * ((txBytesSum * 8) / 
                (kTxBytesListInterval.GetMilliSeconds() * kTxBytesListWindow ));
            if (txBusySlotSum < m_txBytesList.size() - txIdleSlotSum) {
                m_curTxDelayUs = 1000 * (m_txBytesList.size() - txIdleSlotSum) / 
                    (m_txBytesList.size() - txIdleSlotSum - txBusySlotSum);
            }
            else {
                m_curTxDelayUs = kTxBytesListWindow * kTxBytesListInterval.GetMicroSeconds();
            }
        }
        // std::cout << "[NetworkQueueAbc] UpdateTxRate"
        //     << " Now " << Simulator::Now()
        //     << " txBytesSum " << txBytesSum 
        //     << " txBusySlotSum " << txBusySlotSum 
        //     << " txIdleSlotSum " << txIdleSlotSum 
        //     << " curTxRateBps " << m_curTxRateBps
        //     << " kTxBytesListWindow " << kTxBytesListWindow
        //     << " kTxBytesListInterval " << kTxBytesListInterval
        //     << '\n';
    }
    m_curTxRateBps = std::max(m_curTxRateBps, kMinTxRateBps);
    m_curTxTransRateBps = std::max(m_curTxTransRateBps, kMinTxRateBps);
    uint32_t QsizeAp = 0;
    if(m_mode == 1){ QsizeAp = m_dlQueue->GetNBytes(); }
    else{ QsizeAp = m_wmq->GetNBytes(); }
    double delay = 1000 * 8 * ( this->m_qdisc->GetNBytes() + QsizeAp ) / float(m_curTxRateBps);
    m_interface->UpdateFromQueue(double(m_curTxRateBps),double(m_curTxTransRateBps),delay);
    m_eventUpdateTxRate = Simulator::Schedule(kTxBytesListInterval, &NetworkQueueAbc::UpdateTxRate, this);
}

bool NetworkQueueAbc::OnRecv_snddev(Ptr<NetDevice> m_rcvDev, Ptr<const Packet> packet, uint16_t protocol,
                                    const Address &sender, const Address &receiver, NetDevice::PacketType packetType)
{
    Ptr<Packet> dynamicPkt = ConstCast<Packet> (packet);
    Ipv4Header ipHdr;
    packet->PeekHeader(ipHdr);
    if(ipHdr.GetDestination() == m_sendIP){Simulator::ScheduleNow(&NetworkQueueAbc::SendPkt, this, dynamicPkt);}
    return true;
}

void NetworkQueueAbc::SendPkt(Ptr<Packet> packet)
{
    Address sendMac = m_rcvDev->GetChannel()->GetDevice(0)->GetAddress();
    m_rcvDev->Send(packet, sendMac, 0x0800);
}

bool NetworkQueueAbc::OnRecv_rcvdev(Ptr<NetDevice> m_rcvDev, Ptr<const Packet> packet, uint16_t protocol,
                                     const Address &sender, const Address &receiver, NetDevice::PacketType packetType){return true;}
}; // namespace ns3
