#include "network-queue-fastack.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NetworkQueueFastAck");

TypeId NetworkQueueFastAck::GetTypeId()
{
    static TypeId tid = TypeId("ns3::NetworkQueueFastAck")
                            .SetParent<NetworkQueue>()
                            .SetGroupName("zhuge")
                            .AddConstructor<NetworkQueueFastAck>();
    return tid;
};

NetworkQueueFastAck::NetworkQueueFastAck ()
: receiver {NULL} 
{};

NetworkQueueFastAck::~NetworkQueueFastAck () {};

void NetworkQueueFastAck::Setup (Ipv4Address sendIP, uint16_t sendPort, Ipv4Address recvIP, uint16_t recvPort, 
    bool tcpEnabled, int mode)
{
    m_sendIP = sendIP;
    m_sendPort = sendPort;
    m_recvIP = recvIP;
    m_recvPort = recvPort;
    this->cc_create = cc_create;
    this->wifi_same_ap = wifi_same_ap;
    m_RxWinSize = 65535;
    this->CollectAckInterval = MicroSeconds(200);

    m_mode = mode;
};

void NetworkQueueFastAck::DoDispose(){

};

void NetworkQueueFastAck::StartApplication(void)
{
    InitSocket();
};

void NetworkQueueFastAck::StopApplication(void)
{
    m_checkack.Cancel();
};

void NetworkQueueFastAck::InitSocket()
{
    if (m_mode == 0) {m_rcvDev = StaticCast<PointToPointNetDevice, NetDevice>(GetNode()->GetDevice(0));}
    else {m_rcvDev = StaticCast<PointToPointNetDevice, NetDevice>(GetNode()->GetDevice(1));}
    if (m_mode == 0){
        m_sndDev = GetNode()->GetDevice(1);
        Ptr<WifiNetDevice> WifisndDev = StaticCast<WifiNetDevice, NetDevice>(m_sndDev);
        PointerValue ptr;
        BooleanValue qosSupported;
        Ptr<RegularWifiMac> rmac = DynamicCast<RegularWifiMac>(WifisndDev->GetMac());
        rmac->GetAttributeFailSafe("QosSupported", qosSupported);
        std::cout << qosSupported << std::endl;
        rmac->GetAttributeFailSafe("BE_Txop", ptr);
        m_edca = ptr.Get<QosTxop>();
        if (this->cc_create) {
            WifisndDev->SetPromiscReceiveCallback(MakeCallback(&NetworkQueueFastAck::OnRecv_snddev, this));
        }
    }
    else {
        m_sndDev = GetNode()->GetDevice(2);
        Ptr<PointToPointNetDevice> P2psndDev = StaticCast<PointToPointNetDevice, NetDevice>(m_sndDev);
        if (this->cc_create) {
            P2psndDev->SetPromiscReceiveCallback(MakeCallback(&NetworkQueueFastAck::OnRecv_snddev, this));
        }
        Ptr<DropTailQueue<Packet>> rcvqueue =  StaticCast<DropTailQueue<Packet>, Queue<Packet>>(P2psndDev->GetQueue());
        rcvqueue->SetMaxSize(QueueSize ("1000p"));
    }
    m_rcvDev->SetPromiscReceiveCallback(MakeCallback(&NetworkQueueFastAck::OnRecv_rcvdev, this));

    m_checkack = Simulator::Schedule(CollectAckInterval, &NetworkQueueFastAck::CheckAck, this);
    m_ExpAckNumber = SequenceNumber32(uint32_t(1));
    m_HighestSeq = SequenceNumber32(uint32_t(1));
};

void NetworkQueueFastAck::CheckAck(){
    if(m_mode == 0){
        while(m_edca->GetFem()->m_PacketList.size()>0){
            Ptr<Packet> PktToAck = m_edca->GetFem()->m_PacketList.front()->Copy();
            m_Qseq.push_back(PktToAck);
            m_edca->GetFem()->m_PacketList.pop_front();
        }
    }
    else{
        Ptr<PointToPointNetDevice> P2psndDev = StaticCast<PointToPointNetDevice, NetDevice>(m_sndDev);
        while(P2psndDev->m_PacketList.size()>0){
            Ptr<Packet> PktToAck = P2psndDev->m_PacketList.front()->Copy();
            m_Qseq.push_back(PktToAck);
            P2psndDev->m_PacketList.pop_front();
        }
        if(m_Qseq.size()>0){CreateAckPkt();}
        m_checkack = Simulator::Schedule(CollectAckInterval, &NetworkQueueFastAck::CheckAck, this);        
    }
    if(m_Qseq.size()>0){CreateAckPkt();}
    m_checkack = Simulator::Schedule(CollectAckInterval, &NetworkQueueFastAck::CheckAck, this);   
}

void NetworkQueueFastAck::CreateAckPkt(){
    std::cout<<"[start pkt]"<<" "<<"time now: "<<Simulator::Now().GetMilliSeconds()<<std::endl;
    for(uint32_t index = 0; index < m_Qseq.size(); index ++){
        Ptr<Packet> pktToAck = m_Qseq.front()->Copy();
        SequenceNumber32 SeqHead = SequenceNumber32(0);
        bool shouldack = true;
        if(m_mode == 0){
            LlcSnapHeader llchdr;
            pktToAck->Print(std::cout);
            std::cout<<"\n";
            pktToAck->RemoveHeader(llchdr);
            Ipv4Header ipv4hdr = Ipv4Header();
            pktToAck->RemoveHeader(ipv4hdr);
            if(ipv4hdr.GetDestination() == m_recvIP && ipv4hdr.GetProtocol() == 6){
                TcpHeader tcphdr;
                pktToAck->PeekHeader(tcphdr);
                SeqHead = tcphdr.GetSequenceNumber();
            }
            else{
                shouldack = false;
                m_Qseq.pop_front();
            }
        }
        else{
            PppHeader ppphdr;
            pktToAck->Print(std::cout);
            std::cout<<"\n";
            pktToAck->RemoveHeader(ppphdr);
            Ipv4Header ipv4hdr = Ipv4Header();
            pktToAck->RemoveHeader(ipv4hdr);
            if(ipv4hdr.GetDestination() == m_recvIP && ipv4hdr.GetProtocol() == 6){
                TcpHeader tcphdr;
                pktToAck->PeekHeader(tcphdr);
                SeqHead = tcphdr.GetSequenceNumber();
            }  
            else{
                shouldack = false;
                m_Qseq.pop_front();
            }          
        }
        if(shouldack && SeqHead == m_LastAckNumber){
            m_Qseq.pop_front();
            for(uint32_t i = 0; i < SndTimeHistory.size(); i++){
                if(SndTimeHistory[i].first == SeqHead && SndLengthHistory[i].first == SeqHead){
                    SndTimeHistory.erase(SndTimeHistory.begin()+i);
                    SndLengthHistory.erase(SndLengthHistory.begin()+i);
                    break;
                }
            }
        }
        else if(shouldack){
            for(uint32_t i = 0; i < SndTimeHistory.size(); i++){
                if(SndTimeHistory[i].first == SeqHead && SndLengthHistory[i].first == SeqHead){
                    // uint32_t echotime = SndTimeHistory[i].second;
                    uint32_t length = SndLengthHistory[i].second;
                    m_AckNumber = SeqHead + SequenceNumber32(length);
                    Ptr<Packet> p = Create<Packet> ();
                    TcpHeader Tcphdr;
                    Tcphdr.SetFlags (TcpHeader::ACK);
                    Tcphdr.SetSequenceNumber (SequenceNumber32(uint32_t(1)));
                    Tcphdr.SetAckNumber (m_AckNumber);
                    Tcphdr.SetSourcePort (m_recvPort);
                    Tcphdr.SetDestinationPort (m_sendPort);
                    // Ptr<TcpOptionTS> option = CreateObject<TcpOptionTS> ();
                    // option->SetTimestamp (TcpOptionTS::NowToTsValue ());
                    // option->SetEcho (echotime);
                    // Tcphdr.AppendOption(option);
                    uint16_t SeqOut = uint16_t(m_HighestSeq - m_LastRcvAck);
                    SeqOut = 0;
                    Tcphdr.SetWindowSize(m_RxWinSize - SeqOut);
                    Ipv4Header Iphdr;
                    Iphdr.SetSource(m_recvIP);
                    Iphdr.SetDestination(m_sendIP);
                    Iphdr.SetIdentification(m_LastAckId + 1);
                    Iphdr.SetProtocol(uint8_t(6));
                    Iphdr.SetPayloadSize(uint16_t(20));
                    Iphdr.SetTtl(uint8_t(64));
                    std::cout<<"[create pkt]"<<" time now: "<<Simulator::Now().GetMilliSeconds()<<" ";
                    p->AddHeader(Tcphdr);
                    p->AddHeader(Iphdr);
                    p->Print(std::cout);
                    std::cout<<std::endl;
                    m_LastAckId += 1;
                    SndTimeHistory.erase(SndTimeHistory.begin()+i);
                    SndLengthHistory.erase(SndLengthHistory.begin()+i);
                    if(m_mode == 0){Simulator::ScheduleNow(&NetworkQueueFastAck::SendPkt, this, p);}
                    else{Simulator::Schedule(MilliSeconds(16),&NetworkQueueFastAck::SendPkt, this, p);}
                    m_LastAckNumber = m_AckNumber;
                    break;
                }
            }
            m_Qseq.pop_front();
        }
    }
}

bool NetworkQueueFastAck::OnRecv_snddev(Ptr<NetDevice> m_rcvDev, Ptr<const Packet> packet, uint16_t protocol,
                                    const Address &sender, const Address &receiver, NetDevice::PacketType packetType)
{
    Ipv4Header ipHdr;
    packet->PeekHeader(ipHdr);
    Ptr<Packet> parsedPkt = packet->Copy();
    Ptr<Packet> dynamicPkt = ConstCast<Packet> (packet);
    if (ipHdr.GetDestination() == m_sendIP && ipHdr.GetProtocol() == 6) {
        TcpHeader TcpHdr;
        parsedPkt->RemoveHeader(ipHdr);
        parsedPkt->PeekHeader(TcpHdr);
        if(int(TcpHdr.GetFlags()) == 16){
            m_RxWinSize = TcpHdr.GetWindowSize();
            SequenceNumber32 seq = TcpHdr.GetAckNumber();
            if(seq == m_LastRcvAck){
                m_DupNumber += 1;
                if(m_DupNumber == 3){
                    std::cout<<"[pkt print queue]"<<" need to retrans "<<seq<<std::endl;
                    for(uint32_t i = 0; i < m_cache.size(); i++){
                        Ptr<Packet> pkt = m_cache[i]->Copy();
                        Ipv4Header ipv4header;
                        pkt->RemoveHeader(ipv4header);
                        TcpHeader tcpheader;
                        pkt->PeekHeader(tcpheader);
                        SequenceNumber32 seqtemp = tcpheader.GetSequenceNumber();
                        std::cout<<"[pkt print queue]"<<" retrans find "<<tcpheader.GetSequenceNumber() <<" "<<seqtemp<<std::endl;
                        if(seqtemp == seq){
                            if(m_mode == 0){
                                std::cout<<"[send retrans pkt]"<<" "<<"time now: "<<Simulator::Now().GetMilliSeconds()<<" ";
                                Mac48Address sendMac ("00:00:00:00:00:01");
                                m_sndDev->Send(m_cache[i], Address(sendMac), 0x0800);
                                m_cache.erase(m_cache.begin()+i);
                            }
                            else{
                                std::cout<<"[send retrans pkt]"<<" "<<"time now: "<<Simulator::Now().GetMilliSeconds()<<" ";
                                Address sendMac = m_sndDev->GetChannel()->GetDevice(0)->GetAddress();
                                m_sndDev->Send(m_cache[i], Address(sendMac), 0x0800);
                                m_cache.erase(m_cache.begin()+i);                                
                            }
                            break;
                        }
                    }                  
                }
            }
            else if(seq > m_LastRcvAck){
                m_DupNumber = 1;
                for(uint32_t i = 0; i < m_cache.size(); i++){
                    Ptr<Packet> pkt = m_cache[i]->Copy();
                    Ipv4Header ipv4header;
                    pkt->RemoveHeader(ipv4header);
                    TcpHeader tcpheader;
                    pkt->PeekHeader(tcpheader);
                    SequenceNumber32 seqtemp = tcpheader.GetSequenceNumber() + SequenceNumber32(uint32_t(ipv4header.GetPayloadSize() - 20));
                    std::cout<<"[pkt print queue]"<<" normal "<<tcpheader.GetSequenceNumber() <<" "<<seqtemp<<std::endl;
                    if(seqtemp == m_LastRcvAck){
                        uint32_t del = 0;
                        while(del <= i){m_cache.pop_front();del+=1;}
                        break;
                    }
                }
                m_LastRcvAck = seq;
            }
        }
        else{Simulator::ScheduleNow(&NetworkQueueFastAck::SendPkt, this, dynamicPkt);}
    }
    return true;
}

bool NetworkQueueFastAck::OnRecv_rcvdev(Ptr<NetDevice> m_rcvDev, Ptr<const Packet> packet, uint16_t protocol,
                                     const Address &sender, const Address &receiver, NetDevice::PacketType packetType){
    
    Ipv4Header rcvIpHdr;
    packet->PeekHeader(rcvIpHdr);   
    if (this->cc_create) {
        if (rcvIpHdr.GetProtocol() == 6 && rcvIpHdr.GetDestination()==m_recvIP) {
            // TODO: need to justify whether the incoming packet is packet of interest
            Ptr<Packet> pkt_to_set = packet->Copy();
            pkt_to_set->RemoveHeader(rcvIpHdr);
            TcpHeader tcpheader;
            pkt_to_set->PeekHeader(tcpheader);
            if (int(tcpheader.GetFlags()) == 16 && rcvIpHdr.GetPayloadSize() > 20) {
                uint32_t SndTime = uint32_t(Simulator::Now().GetMilliSeconds()) - 25;
                uint16_t SndLength = rcvIpHdr.GetPayloadSize() - 20;
                SequenceNumber32 seq = tcpheader.GetSequenceNumber();
                SndTimeHistory.push_back(std::make_pair(seq,SndTime));
                SndLengthHistory.push_back(std::make_pair(seq,SndLength));
                if(seq == m_ExpAckNumber){m_HighestSeq = seq;}
                m_ExpAckNumber = seq + SequenceNumber32(uint32_t(SndLength));
                m_cache.push_back(packet->Copy());
                std::cout<<"[push back]"<<"time now: "<<Simulator::Now().GetMicroSeconds()<<" "<<seq<<" "<<SndTime<<" "<<SndLength<<std::endl;
            }
        }
    }
    return true;
}

void NetworkQueueFastAck::SendPkt(Ptr<Packet> packet)
{
    Address sendMac = m_rcvDev->GetChannel()->GetDevice(0)->GetAddress();
    m_rcvDev->Send(packet, sendMac, 0x0800);
}

}; // namespace ns3
