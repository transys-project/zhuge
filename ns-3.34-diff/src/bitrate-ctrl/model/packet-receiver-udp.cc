#include "packet-receiver-udp.h"
#include "game-client.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("PacketReceiverUdp");

TypeId PacketReceiverUdp::GetTypeId() {
    static TypeId tid = TypeId ("ns3::PacketReceiverUdp")
        .SetParent<PacketReceiver> ()
        .SetGroupName("bitrate-ctrl")
    ;
    return tid;
};

PacketReceiverUdp::PacketReceiverUdp (uint32_t wndsize, uint16_t interval, Ipv4Address m_peerIP, uint16_t m_peerPort, uint16_t m_localPort)
: PacketReceiver{}
, m_record {}
, m_feedback_interval {MilliSeconds(interval)}
, m_feedbackTimer {Timer::CANCEL_ON_DESTROY}
, m_last_feedback {65535}
, wnd_size {1000}
, time_wnd_size{wndsize}
, m_credible {1000}
, bytes_per_packet {1500}
, last_id {0}
, pkts_in_wnd {0}
, bytes_in_wnd {0}
, time_in_wnd {0}
, losses_in_wnd {0}
, rcvd_pkt_cnt {0}
, rcvd_data_pkt_cnt {0}
, rcvd_fec_pkt_cnt {0}
, incomplete_groups {}
, complete_groups {}
, timeout_groups {}
, loss_rate {0.}
, throughput_kbps {30000.}
, one_way_dispersion {MicroSeconds(180)}
, m_loss_seq {1000}
, m_recv_sample {}
{   
    this->m_localPort = m_localPort;
    this->m_peerIP = m_peerIP;
    this->m_peerPort = m_peerPort;
};

PacketReceiverUdp::~PacketReceiverUdp () {};

void PacketReceiverUdp::StartApplication(GameClient* client, Ptr<VideoDecoder> decoder, Time delay_ddl, Ptr<Node> node){
    PacketReceiver::StartApplication(client, decoder, delay_ddl, node);
    if (this->m_socket == NULL) {
        this->m_socket = Socket::CreateSocket(node, UdpSocketFactory::GetTypeId());
        auto res = this->m_socket->Bind(InetSocketAddress{Ipv4Address::GetAny(),this->m_localPort});
        NS_ASSERT (res == 0);
        this->m_socket->Connect(InetSocketAddress{this->m_peerIP, this->m_peerPort});
        this->m_socket->SetRecvCallback(MakeCallback(&PacketReceiverUdp::OnSocketRecv_receiver,this));
    }
    this->m_feedbackTimer.SetFunction(&PacketReceiverUdp::Feedback_NetState,this);
    this->m_feedbackTimer.SetDelay(m_feedback_interval);
}

void PacketReceiverUdp::OnSocketRecv_receiver(Ptr<Socket> socket)
{
    Time time_now = Simulator::Now();
    Ptr<Packet> pkt = socket->Recv();
    Ptr<NetworkPacket> net_pkt = NetworkPacket::ToInstance(pkt);
    net_pkt->SetRcvTime(time_now);
    PacketType pkt_type = net_pkt->GetPacketType();

    if(pkt_type != DATA_PKT && pkt_type != FEC_PKT && pkt_type != DUP_FEC_PKT)
        // not data or FEC packets
        return;

    Ptr<VideoPacket> video_pkt = DynamicCast<VideoPacket, NetworkPacket> (net_pkt);
    ReceiveUdpPacket(video_pkt);

    /* update network statistics */
    uint32_t id = video_pkt->GetGlobalId();
    uint32_t RxTime = time_now.GetMicroSeconds();
    Ptr<RcvTime> rt = Create<RcvTime>(id, RxTime, pkt->GetSize());

    if (m_record.empty()) {
        m_record.push_back(rt);
        this->last_id = id;
        if (m_feedbackTimer.IsExpired()){
            m_feedbackTimer.Schedule();
        }
    }
    else {
        // slide on the window
        for (auto it = m_record.begin(); it < m_record.end(); it++) {
            if ((*it)->rt_us < RxTime - this->time_wnd_size) {
                this->bytes_in_wnd -= (*it)->pkt_size;
                this->m_record.erase(it);
            }
        }
        // while(this->m_record.front()->rt_us < RxTime - this->time_wnd_size) {
        //     this->bytes_in_wnd -= this->m_record.front()->pkt_size;
        //     this->m_record.pop_front();
        //     if(this->m_record.empty()) break;
        // }

        // insert new packet
        if (m_record.empty()) {
            m_record.push_back(rt);
        }
        else {
            if (this->lessThan_simple(id, m_record.front()->pkt_id)) {
                m_record.push_front(rt);
            }
            else {
                auto it = m_record.end();
                while (it > m_record.begin()) {
                    if (this->lessThan_simple((*(it-1))->pkt_id, id)){
                        m_record.insert(it, rt);
                        break;
                    }
                    it--;
                }
            }
            
        }
    }
    // std::cout << "[Rcver] throughput (kbps):" << (float)this->bytes_in_wnd / (float)this->time_wnd_size * 8. * 1000. << ", time wnd size:" <<this->time_wnd_size<<" ms\n";
    this->bytes_in_wnd += pkt->GetSize();
    // std::cout << "append pkt size:" << pkt->GetSize() << ", bytes in wnd:" << this->bytes_in_wnd << ", pkts in wnd:" << this->m_record.size() - 1 << "\n";

};

void PacketReceiverUdp::ReceiveUdpPacket(Ptr<VideoPacket> pkt) {
    auto group_id = pkt->GetGroupId();
    Time rcv_time = pkt->GetRcvTime();

    /* DEBUG */
    // if(pkt->GetTXCount() > 0)
        // std::cout << "[Client] At " << Simulator::Now().GetMilliSeconds() <<
        //     " ms pkt rcvd: Type: " << pkt->GetPacketType() <<
        //     ", Global ID: " << unsigned(pkt->GetGlobalId()) <<
        //     ", TX cnt: " << unsigned(pkt->GetTXCount()) <<
        //     ", Group id: " << pkt->GetGroupId() <<
        //     ", Pkt id group: " << pkt->GetPktIdGroup() <<
        //     ", Batch id: " << pkt->GetBatchId() <<
        //     ", Pkt id batch: " << pkt->GetPktIdBatch() <<
        //     ", Batch data num: " << pkt->GetBatchDataNum() <<
        //     ", Batch fec num: " << pkt->GetBatchFECNum() <<
        //     ", Encode Time: " << pkt->GetEncodeTime().GetMilliSeconds() <<
        //     ", Rcv Time: " << pkt->GetRcvTime().GetMilliSeconds() << "\n";
        
    if(pkt->GetTXCount() == 0) {
        if(pkt->GetPacketType() == PacketType::FEC_PKT)
            NS_LOG_FUNCTION("FEC packet (0) packet received!");
        // else if(pkt->GetPacketType() == PacketType::DATA_PKT)
        //     NS_LOG_FUNCTION("Data packet (0) received!");
    } else {
        NS_LOG_FUNCTION("RTX packet received!");
        if(pkt->GetPacketType() == PacketType::FEC_PKT)
            NS_LOG_FUNCTION("RTX FEC packet received!");
        else if(pkt->GetPacketType() == PacketType::DUP_FEC_PKT)
            NS_LOG_FUNCTION("RTX Dup FEC packet received!");
        else if(pkt->GetPacketType() == PacketType::DATA_PKT)
            NS_LOG_FUNCTION("RTX Data packet received!");
    }
    /* DEBUG end */

    /* STATISTICS */
    // statistic: rtx count
    if(this->rcvd_pkt_rtx_count.find(pkt->GetTXCount()) == this->rcvd_pkt_rtx_count.end())
        this->rcvd_pkt_rtx_count[pkt->GetTXCount()] = 0;
    this->rcvd_pkt_rtx_count[pkt->GetTXCount()] ++;

    this->rcvd_pkt_cnt ++;
    // categorize different types of rtx count
    if(pkt->GetPacketType() == PacketType::DATA_PKT){
        this->rcvd_data_pkt_cnt ++;
        if(this->rcvd_datapkt_rtx_count.find(pkt->GetTXCount()) == this->rcvd_datapkt_rtx_count.end())
            this->rcvd_datapkt_rtx_count[pkt->GetTXCount()] = 0;
        this->rcvd_datapkt_rtx_count[pkt->GetTXCount()] ++;
    }

    if(pkt->GetPacketType() == PacketType::FEC_PKT || pkt->GetPacketType() == PacketType::DUP_FEC_PKT){
        this->rcvd_fec_pkt_cnt ++;
        if(this->rcvd_fecpkt_rtx_count.find(pkt->GetTXCount()) == this->rcvd_fecpkt_rtx_count.end())
            this->rcvd_fecpkt_rtx_count[pkt->GetTXCount()] = 0;
        this->rcvd_fecpkt_rtx_count[pkt->GetTXCount()] ++;
    }
    /* STATISTICS end */

    /* 2) Insert/Create packet group */
    if(this->incomplete_groups.find(group_id) == this->incomplete_groups.end()) {
        // if it's the first packet of the packet group
        this->incomplete_groups[group_id] = Create<PacketGroup> (
            group_id, pkt->GetGroupDataNum(), pkt->GetEncodeTime()
        );
    }
    // std::cout << group_id << '\n';
    NS_ASSERT(this->incomplete_groups.find(group_id) != this->incomplete_groups.end());
    this->incomplete_groups[group_id]->AddPacket(pkt, this->Get_FECgroup_delay());
    /* End of 2) Insert/Create packet group */

    auto group = this->incomplete_groups[group_id];

    /* 3) Get decoded packets */
    // get decoded packets and send them to decoder
    auto decode_pkts = group->GetUndecodedPackets();
    this->decoder->DecodeDataPacket(decode_pkts);
    /* End of 3) Get decoded packets */

    /* 4) replay ACK packet */
    this->ReplyACK(decode_pkts, pkt->GetGlobalId());
    /* End of 4) replay ACK packet */

    /* 1) necessity check */
    // do not proceed if the group is timed out or complete
    if(this->complete_groups.find(group_id) != this->complete_groups.end())
        return;
    if(this->timeout_groups.find(group_id) != this->timeout_groups.end())
        return;
    /* End of 1) necessity check */

    /* 5) Check whether a group a complete */
    // check if all data packets in this group have been retrieved
    if (group->CheckComplete()) {
        // debug
        // std::cout << "[Complete Group] " <<
        //     "Group ID: " << group->GetGroupId() <<
        //     ", Total DATA: " << group->GetDataNum() <<
        //     ", Max TX: " << unsigned(group->GetMaxTxCount()) <<
        //     ", First TX data: " << group->GetFirstTxDataCount() <<
        //     ", First TX FEC: " << group->GetFirstTxFECCount() <<
        //     ", RTX data: " << group->GetRtxDataCount() <<
        //     ", RTX FEC: " << group->GetRtxFECCount() <<
        //     ", Frame id: " << decode_pkts[0]->GetFrameId() <<
        //     ", encode: " << pkt->GetEncodeTime().GetMilliSeconds() <<
        //     ", now" << Simulator::Now().GetMilliSeconds() << "\n";
        // rtx count
        uint8_t max_tx = group->GetMaxTxCount();
        if(this->rcvd_group_rtx_count.find(max_tx) == this->rcvd_group_rtx_count.end())
            this->rcvd_group_rtx_count[max_tx] = 0;
        this->rcvd_group_rtx_count[max_tx] ++;
        // group delay (packet interval)
        Time avg_pkt_itvl = group->GetAvgPktInterval();
        if(avg_pkt_itvl > MicroSeconds(0))
            this->Set_FECgroup_delay(avg_pkt_itvl);
        this->complete_groups[group_id] = group;
        this->incomplete_groups.erase(group_id);
    }
    /* End of 5) Check whether a group a complete */

    /* 6) Check all incomplete groups for timeouts */
    for(auto it = this->incomplete_groups.begin();it != this->incomplete_groups.end();) {
        auto i_group_id = it->second->GetGroupId();
        // Assume all packets comes from the same frame
        // std::cout << "Group ID: " << it->second->GetGroupId() <<
        //     "group rcv size: " << it->second->decoded_pkts.size() << "\n";
        if (rcv_time > it->second->GetEncodeTime() + this->delay_ddl) {
            // debug
            // std::cout << "[Timeout Group] " <<
            //     "At " << Simulator::Now().GetMilliSeconds() <<
            //     "ms, Group ID: " << it->second->GetGroupId() <<
            //     ", Total DATA: " << it->second->GetDataNum() <<
            //     ", Max TX: " << unsigned(it->second->GetMaxTxCount()) <<
            //     ", First TX data: " << it->second->GetFirstTxDataCount() <<
            //     ", First TX FEC: " << it->second->GetFirstTxFECCount() <<
            //     ", RTX data: " << it->second->GetRtxDataCount() <<
            //     ", RTX FEC: " << it->second->GetRtxFECCount() <<
            //     ", Last Rcv Time: " << it->second->GetLastRcvTimeTx0().GetMilliSeconds() <<
            //     ", Rcv time: " << rcv_time.GetMilliSeconds() <<
            //     ", Encode Time: " << it->second->GetEncodeTime().GetMilliSeconds() <<
            //     ", Delay ddl: " << this->delay_ddl.GetMilliSeconds() << "\n";
            // the whole group is timed out
            this->timeout_groups[i_group_id] = it->second;
            it = this->incomplete_groups.erase(it);
            continue;
        }
        it ++;
    }
    /* End of 6) check all incomplete groups for timeouts */
};



void PacketReceiverUdp::ReplyACK(std::vector<Ptr<DataPacket>> data_pkts, uint16_t last_pkt_id) {
    // std::cout << "[Client] At " << Simulator::Now().GetMilliSeconds() << " ms send ACK for packet " << last_pkt_id << '\n';
    std::vector<Ptr<GroupPacketInfo>> pkt_infos;
    for(auto data_pkt : data_pkts) {
        // std::cout << "[Client] At " << Simulator::Now().GetMilliSeconds() << " ms send ACK for group " << data_pkt->GetGroupId() << " pkt " << data_pkt->GetPktIdGroup() << '\n';
        pkt_infos.push_back(Create<GroupPacketInfo>(data_pkt->GetGroupId(), data_pkt->GetPktIdGroup()));
    }
    Ptr<AckPacket> pkt = Create<AckPacket>(pkt_infos, last_pkt_id);
    this->SendPacket(pkt);
};

void PacketReceiverUdp::Feedback_NetState ()
{
    this->pkts_in_wnd = 0;
    if(this->m_record.size() > 1) {
        Ptr<RcvTime> record_end = m_record.back();
        Ptr<RcvTime> record_front = m_record.front();
        this->pkts_in_wnd = (record_end->pkt_id - record_front->pkt_id + 65537) % 65536;
        //NS_ASSERT_MSG(this->pkts_in_wnd <= this->wnd_size + 1,"Packets in window "<<this->pkts_in_wnd<<" is larger than window size limit "<<(this->wnd_size + 1));
        //this->time_in_wnd = record_end->rt_us - record_front->rt_us;
    }
    //this->bytes_in_wnd = this->m_record.size() * this->bytes_per_packet;
    this->losses_in_wnd = 0;

    // if(this->time_in_wnd > 0) {
    //     this->throughput_kbps = (float)this->bytes_in_wnd / (float)this->time_in_wnd * 8. * 1000.;
    // }
    this->throughput_kbps = (float)this->bytes_in_wnd / (float)this->time_wnd_size * 8. * 1000.;
    // std::cout << "[Rcver] throughput (kbps):" << this->throughput_kbps<<", bytes in wnd:" <<this->bytes_in_wnd << ", pkts in wnd:" << this->pkts_in_wnd << ", time wnd size:" <<this->time_wnd_size<<" ms\n";

    // if(throughput_kbps > 100000){
    //     std::cout << "throughput(kbps):" << this->throughput_kbps<<", bytes in wnd:" <<this->bytes_in_wnd
    //               << ", pkts in wnd:" << this->pkts_in_wnd << ", time wnd size:" <<this->time_wnd_size<<"\n";
    // }

    //Gather m_loss_seq & m_recvtime_sample from m_record
    this->m_loss_seq.clear();
    this->m_recv_sample.clear();
    auto it = this->m_record.begin();
    int consecutive_recv = 0;
    uint32_t id;
    if(!this->m_record.empty()){
        uint32_t prev_id = (*it)->pkt_id - 1;
        bool first=true;
        while (it<this->m_record.end())
        {
            id = (*it)->pkt_id;

            if(!first){
                NS_ASSERT_MSG(this->lessThan_simple(prev_id,id),"history should be ordered. Previous ID="<<prev_id<<" while ID="<<id<<" with last_id="<<this->last_id);
            }
            else{
                first=false;
            }

            if ((id - prev_id + 65536)%65536 == 1) {
                consecutive_recv += 1;
                if(it == this->m_record.end()-1) {this->m_loss_seq.push_back(consecutive_recv);}
            }
            else {
                this->m_loss_seq.push_back(consecutive_recv);
                this->m_loss_seq.push_back(1 - (id - prev_id + 65536)%65536);
                consecutive_recv = 1;
                this->losses_in_wnd += id - prev_id - 1;
            }
            if(this->lessThan_simple(this->m_last_feedback, id)){
                this->m_recv_sample.push_back(*it);
                this->m_last_feedback = id;
            }

            it++;
            prev_id = id;
        }
    }

    if(this->pkts_in_wnd >= 1){
        this->loss_rate = (float)this->losses_in_wnd / (float)this->pkts_in_wnd;
    }

    NS_LOG_FUNCTION("LossRate" << this->loss_rate << "Throughput" << this->throughput_kbps << "FEC group delay" << this->one_way_dispersion.GetMicroSeconds());

    /* construct NetState*/
    Ptr<NetStates> netstate = Create<NetStates>(this->loss_rate, (uint32_t)this->throughput_kbps, (uint16_t)this->one_way_dispersion.GetMicroSeconds());
    netstate->loss_seq = m_loss_seq;
    netstate->recvtime_hist = m_recv_sample;

    Ptr<NetStatePacket> nstpacket = Create<NetStatePacket>();
    nstpacket->SetNetStates(netstate);
    if (!(this->game_client)->wirtc) {
        this->m_socket->Send(nstpacket->ToNetPacket());
    }
    // this->m_recv_sample.clear();
    this->m_feedbackTimer.Schedule();
};


void PacketReceiverUdp::SendPacket(Ptr<NetworkPacket> pkt) {
    this->m_socket->Send(pkt->ToNetPacket());
}

void PacketReceiverUdp::Set_FECgroup_delay(Time& fec_group_delay_)
{
    this->one_way_dispersion = fec_group_delay_;
};

Time PacketReceiverUdp::Get_FECgroup_delay()
{
    return this->one_way_dispersion;
};

void PacketReceiverUdp::OutputStatistics() {
    this->decoder->GetDDLMissRate();
    NS_LOG_ERROR("\n[Client] Max TX Count: " << rcvd_pkt_rtx_count.size() - 1);
    NS_LOG_ERROR("[Client] All Packets: " << this->rcvd_pkt_cnt);
    for(auto it = rcvd_pkt_rtx_count.begin();it != rcvd_pkt_rtx_count.end();it++)
        NS_LOG_ERROR("\tTX: " << unsigned(it->first) << ", packet count: " << it->second);
    NS_LOG_ERROR("[Client] Data Packet: " << this->rcvd_data_pkt_cnt);
    for(auto it = rcvd_datapkt_rtx_count.begin();it != rcvd_datapkt_rtx_count.end();it++)
        NS_LOG_ERROR("\tTX: " << unsigned(it->first) << ", packet count: " << it->second);
    NS_LOG_ERROR("[Client] FEC Packet: " << this->rcvd_fec_pkt_cnt);
    for(auto it = rcvd_fecpkt_rtx_count.begin();it != rcvd_fecpkt_rtx_count.end();it++)
        NS_LOG_ERROR("\tTX: " << unsigned(it->first) << ", packet count: " << it->second);

    NS_LOG_ERROR("\n[Client] Groups: Received: " <<  this->complete_groups.size() + this->incomplete_groups.size() + this->timeout_groups.size() << ", Complete: " << this->complete_groups.size() << ", Incomplete: " << this->incomplete_groups.size() << ", Timeout: " << this->timeout_groups.size());
    uint32_t group_count = this->complete_groups.size() + this->incomplete_groups.size() + this->timeout_groups.size();
    for(auto it = rcvd_group_rtx_count.begin();it != rcvd_group_rtx_count.end();it++) {
        NS_LOG_ERROR("\tTX: " << unsigned(it->first) << ", group count: " << it->second << ", ratio: " << ((double_t) it->second) / group_count * 100 << "%");
    }
};

void PacketReceiverUdp::StopRunning() {
    if(this->m_feedbackTimer.IsRunning()){
        this->m_feedbackTimer.Cancel();
    }
    this->m_socket->Close();
    this->OutputStatistics();
};

bool PacketReceiverUdp::lessThan(uint16_t id1, uint16_t id2) {
    //NS_ASSERT_MSG(((this->last_id - id1 + 65536) % 65536) <= this->wnd_size &&
    //              ((this->last_id - id2 + 65536) % 65536) <= this->wnd_size,
    //              "ID1 and ID2 must be in-window.");
    if(this->last_id >= this->wnd_size && id1 < id2)
        return true;

    if(this->last_id<this->wnd_size) {
        if(id2<=this->last_id && id1>=65536-this->wnd_size+this->last_id)
            return true;
        if(id1<id2) {
            if((id1 >= 65536 - this->wnd_size + this->last_id) &&
               (id2 >= 65536 - this->wnd_size + this->last_id))
                return true;
            if((id1 <= this->last_id) &&
               (id2 <= this->last_id))
                return true;
        }
    }
    return false;
};

bool PacketReceiverUdp::lessThan_simple(uint16_t id1, uint16_t id2){
    uint16_t noWarpSubtract = id2 - id1;
    uint16_t wrapSubtract = id1 - id2;
    return noWarpSubtract < wrapSubtract;
}

}; // namespace ns3