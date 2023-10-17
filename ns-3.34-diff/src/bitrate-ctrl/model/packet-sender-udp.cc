#include "packet-sender-udp.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE("PacketSenderUdp");

TypeId PacketSenderUdp::GetTypeId() {
    static TypeId tid = TypeId ("ns3::PacketSenderUdp")
        .SetParent<Object> ()
        .SetGroupName("bitrate-ctrl")
    ;
    return tid;
};

PacketSenderUdp::PacketSenderUdp (
    GameServer * gameServer, void (GameServer::*EncodeFunc)(uint32_t encodeBitrate),
    Ptr<FECPolicy> fecPolicy, Ipv4Address srcIP, uint16_t srcPort, Ipv4Address destIP, uint16_t destPort, 
    Ipv4Header::DscpType dscp, uint8_t fps, Time delayDdl, uint32_t bitrate, uint16_t interval, 
    Ptr<OutputStreamWrapper> bwTrStream
)
: PacketSender (gameServer, EncodeFunc, MilliSeconds(40), srcIP, srcPort, destIP, destPort, dscp)
, m_netStat {NULL}
, m_queue {}
, pktsHistory {}
, m_send_wnd {}
, m_send_wnd_size {10000}
, m_goodput_wnd {}
, m_goodput_wnd_size {50000} // 32 ms
, goodput_pkts_inwnd {0}
, total_pkts_inwnd {0}
, m_goodput_ratio {1.}
, m_group_id {0}
, m_groupstart_TxTime {0}
, m_group_size {0}
, m_prev_id {0}
, m_prev_RxTime {0}
, m_prev_groupend_id {0}
, m_prev_groupend_RxTime {0}
, m_prev_group_size {0}
, m_prev_pkts_in_frame{0}
, m_curr_pkts_in_frame{0}
, m_prev_frame_TxTime{0}
, m_curr_frame_TxTime{0}
, m_prev_frame_RxTime{0}
, m_curr_frame_RxTime{0}
, m_interval{interval}
, inter_arrival{0}
, inter_departure{0}
, inter_delay_var{0}
, inter_group_size{0}
, m_firstFeedback {true}
, m_controller {NULL}
, m_pacing {false}
, m_pacing_interval {MilliSeconds(0)}
, m_pacingTimer {Timer::CANCEL_ON_DESTROY}
, m_send_time_limit {MilliSeconds(interval)}
, m_eventSend {}
, IPIDCounter {0}
, m_last_acked_global_id {0}
, m_delay_ddl {delay_ddl}
, m_finished_frame_cnt {0}
, m_timeout_frame_cnt {0}
, m_exclude_ten_finished_frame_cnt {0}
{
    NS_LOG_ERROR("[Sender] Delay DDL is: " << m_delay_ddl.GetMilliSeconds() << " ms");
    // init frame index
    this->frame_id = 0;
    this->group_id = 0;
    this->batch_id = 0;
    this->fps = fps;
    this->bitrate = bitrate;
    this->delay_ddl = delayDdl;
    this->send_group_cnt = 0;


    this->init_data_pkt_count = 0;
    this->other_pkt_count = 0;
    this->init_data_pkt_size = 0;
    this->other_pkt_size = 0;
    this->exclude_head_init_data_pkt_count = 0;
    this->exclude_head_other_pkt_count = 0;
    this->exclude_head_init_data_pkt_size = 0;
    this->exclude_head_other_pkt_size = 0;

    m_fecPolicy = fecPolicy;
        
    this->check_rtx_interval = MicroSeconds(1e3); // check for retransmission every 1ms

    double defaultRtt = 50;
    double defaultBw  = 2;
    SetNetworkStatistics(MilliSeconds(defaultRtt), defaultBw, 0, 0);
    m_bwTrStream = bwTrStream;
};

PacketSenderUdp::~PacketSenderUdp() {};

void PacketSenderUdp::SetNetworkStatistics(
    Time default_rtt, double_t bw, double_t loss, double_t delay
) {
    if (m_netStat == NULL)
        m_netStat = Create<FECPolicy::NetStat>();
    // initialize netstat
    m_netStat->current_rtt          = default_rtt;
    m_netStat->srtt                 = default_rtt;
    m_netStat->min_rtt              = default_rtt;
    m_netStat->rtt_sd               = default_rtt.GetMilliSeconds() / 4;
    m_netStat->stat_cnt             = 0;
    m_netStat->rtt_sum_avg_ms       = 0;
    m_netStat->rtt_square_sum_avg_ms = 0;
    m_netStat->current_bw           = bw;
    m_netStat->current_loss_rate    = loss;
    m_netStat->one_way_dispersion   = MicroSeconds((uint64_t)(delay * 1e3));
    // randomly generate loss seq
    m_netStat->loss_seq.clear();
    int next_seq = 0;
    for (uint16_t i = 0;i < bw * 1e6 / 8 * 0.1 / 1300/* packet count in 100ms */;i++) {
        bool lost = bool(std::rand() % 2);
        if (lost) {
            if(next_seq <= 0) {
                next_seq --;
            } else {
                m_netStat->loss_seq.push_back(next_seq);
                next_seq = 0;
            }
        } else {    // not lost
            if (next_seq >= 0) {
                next_seq ++;
            } else {
                m_netStat->loss_seq.push_back(next_seq);
                next_seq = 0;
            }
        }
    }
};

void PacketSenderUdp::UpdateRTT(Time rtt) {
    if(m_netStat->srtt == MicroSeconds(0))
        m_netStat->srtt = rtt;
    else
        m_netStat->srtt = 0.2 * rtt + 0.8 * m_netStat->srtt;
    m_netStat->current_rtt = rtt;
    m_netStat->rtt_sum_avg_ms =
        (m_netStat->rtt_sum_avg_ms * m_netStat->stat_cnt
        + (rtt.GetMicroSeconds() / 1000.0)) / (m_netStat->stat_cnt + 1);
    m_netStat->rtt_square_sum_avg_ms =
        (m_netStat->rtt_square_sum_avg_ms * m_netStat->stat_cnt
        + pow(rtt.GetMicroSeconds() / 1000.0, 2)) / (m_netStat->stat_cnt + 1);
    m_netStat->stat_cnt ++;
    m_netStat->rtt_sd = sqrt(m_netStat->rtt_square_sum_avg_ms - pow(m_netStat->rtt_sum_avg_ms, 2));
}

void PacketSenderUdp::SetNetworkStatisticsBytrace(uint16_t rtt /* in ms */, 
                                double_t bw/* in Mbps */,
                                double_t loss_rate)
{
    this->UpdateRTT(MilliSeconds(rtt));
    m_netStat->current_bw = bw;
    m_netStat->current_loss_rate = loss_rate;
    if (m_controller)
        m_controller->UpdateLossRate(uint8_t (loss_rate * 256));
};

void PacketSenderUdp::StartApplication(Ptr<Node> node) {
    if (m_socket == NULL) {
        m_socket = Socket::CreateSocket(node, UdpSocketFactory::GetTypeId());

        InetSocketAddress local = InetSocketAddress(m_srcIP, m_srcPort);
        InetSocketAddress remote = InetSocketAddress(m_destIP, m_destPort);
        local.SetTos (m_dscp << 2);
        remote.SetTos (m_dscp << 2);
        m_socket->Bind(local);
        m_socket->Connect(remote);
        m_socket->SetRecvCallback(MakeCallback(&PacketSenderUdp::OnSocketRecv_sender,this));
    }
    PacketSender::StartApplication(node);

    m_pacingTimer.SetFunction(&PacketSenderUdp::SendPacket,this);
    m_pacingTimer.SetDelay(m_pacing_interval);
    if(this->trace_set){
        this->UpdateNetstateByTrace();
    }
    this->check_rtx_event = Simulator::Schedule(this->check_rtx_interval, &PacketSenderUdp::CheckRetransmission, this);
};

void PacketSenderUdp::StopRunning(){
    GetBandwidthLossRate();
    this->check_rtx_event.Cancel();
    PacketSender::StopRunning();
    m_socket->Close();
}

void PacketSenderUdp::SetController (std::string cca) {
    if (cca == "Gcc") {
        m_controller = std::make_shared<rmcat::GccController> (m_srcPort);
    }
    else if (cca == "Nada") {
        m_controller = std::make_shared<rmcat::NadaController> ();
    }
    else if (cca == "Fixed") {

    }
    else {
        NS_FATAL_ERROR ("Congestion control algorithm " + cca + " not implemented.");
    }
}

uint32_t PacketSenderUdp::GetNextGroupId() { return this->group_id ++; };
uint32_t PacketSenderUdp::GetNextBatchId() { return this->batch_id ++; };

TypeId PacketSenderUdp::UnFECedPackets::GetTypeId() {
    static TypeId tid = TypeId ("ns3::PacketSenderUdp::UnFECedPackets")
        .SetParent<Object> ()
        .SetGroupName("bitrate-ctrl")
        .AddConstructor<PacketSenderUdp::UnFECedPackets> ()
    ;
    return tid;
};

PacketSenderUdp::UnFECedPackets::UnFECedPackets()
: param {}
, next_pkt_id_in_batch {0}
, next_pkt_id_in_group {0}
, pkts {}
{};

PacketSenderUdp::UnFECedPackets::~UnFECedPackets() {};

void PacketSenderUdp::UnFECedPackets::SetUnFECedPackets(
    FECPolicy::FECParam param,
    uint16_t next_pkt_id_in_batch, uint16_t next_pkt_id_in_group,
    std::vector<Ptr<DataPacket>> pkts
) {
    this->param = param;
    this->next_pkt_id_in_batch = next_pkt_id_in_batch;
    this->next_pkt_id_in_group = next_pkt_id_in_group;
    this->pkts = pkts;
};

void PacketSenderUdp::CreateFrame(uint32_t frame_id, uint16_t pkt_id, uint16_t data_pkt_max_payload, uint16_t data_pkt_num, uint32_t data_size){
    std::deque<Ptr<DataPacket>> data_pkt_queue;

    // create packets
    for(uint32_t data_ptr = 0; data_ptr < data_size; data_ptr += data_pkt_max_payload) {
        Ptr<DataPacket> data_pkt = Create<DataPacket>(frame_id, 0, data_pkt_num, pkt_id);
        data_pkt->SetEncodeTime(Simulator::Now());
        data_pkt->SetPayload(nullptr, MIN(data_pkt_max_payload, data_size - data_ptr));
        data_pkt_queue.push_back(data_pkt);
        pkt_id = pkt_id + 1;
    }
    // record the number of data packets in a single frame
    this->frame_data_pkt_cnt[frame_id] = data_pkt_num;

    // std::cout << "[GameServer] frame pkt num: " << data_pkt_queue.size() << "\n";
    NS_ASSERT(data_pkt_queue.size() > 0);
    /* End of 1. Create data packets of a frame */

    auto statistics = this->GetNetworkStatistics();
    this->SendPackets(
        data_pkt_queue,
        this->delay_ddl -
            data_pkt_queue.size() * statistics->one_way_dispersion,
        frame_id,
        false
    );
}

void PacketSenderUdp::SendPackets(std::deque<Ptr<DataPacket>> pkts, Time ddl_left, uint32_t frame_id, bool is_rtx) {
    std::vector<Ptr<VideoPacket>> pkt_to_send_list;
    std::vector<Ptr<VideoPacket>> tmp_list;
    FECPolicy::FECParam fec_param;

    // Not retransmission packets:
    // Group the packets as fec_param's optimal
    if(!is_rtx) {
        // Get a FEC parameter in advance to divide packets into groups
        // Default
        fec_param = this->GetFECParam(pkts.size(), ddl_left, false, is_rtx, frame_id);

        // divide packets into groups
        while(pkts.size() >= fec_param.fec_group_size) {
            tmp_list.clear();

            // get fec_param.group_size data packets from pkts
            for(uint16_t i = 0;i < fec_param.fec_group_size;i++) {
                tmp_list.push_back(pkts.front());
                pkts.pop_front();
            }

            // group them into a pkt_batch
            this->CreateFirstPacketBatch(tmp_list, fec_param);

            // insert them into sending queue
            pkt_to_send_list.insert(pkt_to_send_list.end(), tmp_list.begin(), tmp_list.end());
        }
    }

    // Retransmission packets and other packets:
    // Group the remaininng packets as a single group
    if(pkts.size() > 0) {
        tmp_list.clear();

        // not pacing
        // send out all packets
        // get all data packets left from data_pkt_queue
        fec_param = this->GetFECParam(pkts.size(), ddl_left, true, is_rtx, frame_id);
        // std::cout << "[GameServer::SendFrameFew] FEC param: group size ="<<fec_param.fec_group_size
        // <<", beta0 = " <<fec_param.fec_rate_0<<"\n";
        while(!pkts.empty()) {
            tmp_list.push_back(pkts.front());
            pkts.pop_front();
        }
        // group them into a pkt_batch!
        if(is_rtx) this->CreateRTXPacketBatch(tmp_list, fec_param);
        else this->CreateFirstPacketBatch(tmp_list, fec_param);

        pkt_to_send_list.insert(pkt_to_send_list.end(), tmp_list.begin(), tmp_list.end());
    }

    if(pkt_to_send_list.size() == 0) return;

    // Set enqueue time
    for(auto pkt : pkt_to_send_list)
        pkt->SetEnqueueTime(Simulator::Now());
    // Send out packets
    if(is_rtx) this->SendRtx(pkt_to_send_list);
    else this->SendFrame(pkt_to_send_list);
    // store data packets in case of retransmissions
    if(!is_rtx) this->StorePackets(pkt_to_send_list);
}

void PacketSenderUdp::StorePackets(std::vector<Ptr<VideoPacket>> pkts) {
    // store data packets in case of retransmission
    for (auto pkt : pkts) {
        if(pkt->GetPacketType() != PacketType::DATA_PKT)
            continue;
        if(pkt->GetTXCount() != 0)
            continue;
        // only store first-time data packets
        Ptr<DataPacket> data_pkt = DynamicCast<DataPacket, VideoPacket> (pkt);
        Ptr<GroupPacketInfo> info = Create<GroupPacketInfo> (data_pkt->GetGroupId(), data_pkt->GetPktIdGroup());
        this->data_pkt_history_key.push_back(info);
        this->data_pkt_history[info->group_id][info->pkt_id_in_group] = data_pkt;
    }
};

bool PacketSenderUdp::IsRtxTimeout(Ptr<DataPacket> pkt) {
    Time current_time = Simulator::Now();
    Time enqueue_time = pkt->GetEnqueueTime();
    Time last_send_time = pkt->GetSendTime();
    uint32_t batch_size = pkt->GetBatchDataNum() + pkt->GetBatchFECNum();
    // Decide if the packet needs to be retransmitted
    auto statistic = GetNetworkStatistics();
    // smoothed RTT + 4 * standard deviation
    return (current_time > enqueue_time
        && (current_time - last_send_time > statistic->srtt + MicroSeconds((uint64_t)(statistic->rtt_sd * 1000)) 
            * 4 + batch_size * statistic->one_way_dispersion));
};


bool PacketSenderUdp::MissesDdl(Ptr<DataPacket> pkt) {
    Time current_time = Simulator::Now();
    Time encode_time = pkt->GetEncodeTime();
    // Decide if it's gonna miss ddl. Could be more strict
    if (current_time - encode_time > this->delay_ddl)
        return true;
    else
        return false;
};

void PacketSenderUdp::CreatePacketBatch(
    std::vector<Ptr<VideoPacket>>& pkt_batch,
    FECPolicy::FECParam fec_param,
    bool new_group, uint32_t group_id, bool is_rtx) {

    // data packets
    std::vector<Ptr<DataPacket>> data_pkts;
    for(auto pkt : pkt_batch)
        data_pkts.push_back(DynamicCast<DataPacket, VideoPacket> (pkt));

    // FEC paramters
    double_t fec_rate =fec_param.fec_rate;

    // new_group cannot be true when new_batch is false
    NS_ASSERT(data_pkts.size() > 0);

    // if force_create_fec -> create FEC packets even though data packet num is less than fec_param
    uint16_t batch_data_num = data_pkts.size();
    /* since fec_rate is calculated by fec_count when tx_count == 0, use round to avoid float error. */
    uint16_t batch_fec_num = (uint16_t) round(batch_data_num * fec_rate);
    uint16_t group_data_num, group_fec_num;
    uint8_t tx_count = data_pkts.front()->GetTXCount();

    uint32_t batch_id = GetNextBatchId();
    uint16_t pkt_id_in_batch = 0, pkt_id_in_group = 0;

    if(new_group) {
        group_id = GetNextGroupId();
        pkt_id_in_group = 0;
        group_data_num = batch_data_num;
        group_fec_num = batch_fec_num;
    } else {
        group_data_num = data_pkts.front()->GetGroupDataNum();
        group_fec_num = data_pkts.front()->GetBatchFECNum();
    }
    // std::cout << "[Group INFO] gid " << group_id << ", group data num: " << group_data_num << ", group fec num: " << group_fec_num << ", batch id: " << batch_id << ", batch data num: " << batch_data_num << ", batch fec num: " << batch_fec_num << '\n';

    // to save some time
    pkt_batch.reserve(batch_data_num + batch_fec_num);

    // Assign batch & group ids for data packets
    for(auto data_pkt : data_pkts) {
        NS_ASSERT(data_pkt->GetPacketType() == PacketType::DATA_PKT);

        // Set batch info
        data_pkt->SetFECBatch(batch_id, batch_data_num, batch_fec_num, pkt_id_in_batch ++);

        // Set group info
        if (!new_group && data_pkt->GetGroupInfoSetFlag()) {
            // packets whose group info have already been set
            NS_ASSERT(data_pkt->GetGroupId() == group_id);
            continue;
        }
        // other packets
        data_pkt->SetFECGroup(group_id, group_data_num, group_fec_num, pkt_id_in_group ++);
    }

    // Generate FEC packets
    // create FEC packets and push into packet_group{
    for(uint16_t i = 0;i < batch_fec_num;i++) {
        Ptr<FECPacket> fec_pkt = Create<FECPacket> (tx_count, data_pkts);
        fec_pkt->SetFECBatch(batch_id, batch_data_num, batch_fec_num, pkt_id_in_batch ++);
        if(!is_rtx)
            fec_pkt->SetFECGroup(
                group_id, group_data_num, group_fec_num, pkt_id_in_group ++
            );
        else
            fec_pkt->SetFECGroup(
                group_id, group_data_num, group_fec_num, VideoPacket::RTX_FEC_GROUP_ID
            );
        fec_pkt->SetEncodeTime(Simulator::Now());
        pkt_batch.push_back(fec_pkt);
    }
    return ;
};


void PacketSenderUdp::CreateFirstPacketBatch(
    std::vector<Ptr<VideoPacket>>& pkt_batch, FECPolicy::FECParam fec_param) {
    NS_LOG_FUNCTION(pkt_batch.size());

    NS_ASSERT(pkt_batch.size() > 0);

    this->CreatePacketBatch(
        pkt_batch, fec_param, true /* new_group */, 0, false /* not rtx */
    );

    this->send_group_cnt ++;
};

void PacketSenderUdp::CreateRTXPacketBatch(
    std::vector<Ptr<VideoPacket>>& pkt_batch, FECPolicy::FECParam fec_param) {
    NS_LOG_FUNCTION(pkt_batch.size());

    NS_ASSERT(pkt_batch.size() > 0);
    NS_ASSERT(pkt_batch.size() == fec_param.fec_group_size);

    uint32_t group_id = pkt_batch.front()->GetGroupId();

    this->CreatePacketBatch(
        pkt_batch, fec_param, false /* new_group */, group_id, true /* is rtx */
    );
};

FECPolicy::FECParam PacketSenderUdp::GetFECParam(
        uint16_t max_group_size, 
        Time ddl_left, bool fix_group_size,
        bool is_rtx, uint32_t frame_id
    ) {
    if(this->frame_data_pkt_cnt.find(frame_id) == this->frame_data_pkt_cnt.end()) {
        NS_ASSERT_MSG(false, "No frame size info");
    }
    uint8_t frame_size = this->frame_data_pkt_cnt[frame_id];
    auto fec_param = m_fecPolicy->GetFECParam(
        this->GetNetworkStatistics(),
        // this->encoder->GetBitrate(), /*can delete*/
        0,
        this->delay_ddl.GetMilliSeconds(),
        (uint16_t) floor(ddl_left.GetMicroSeconds() / 1e3), 
        is_rtx, frame_size,
        max_group_size, fix_group_size
    );
    
    if (!m_cc_enable) return fec_param;
    m_curr_fecparam = fec_param;
    if (m_cc_timer.IsRunning())
        m_cc_timer.Cancel();
    PacketSender::UpdateBitrate();
    return fec_param;
};

void PacketSenderUdp::RetransmitGroup(uint32_t group_id) {
    // cannot find the packets to send
    if(this->data_pkt_history.find(group_id) == this->data_pkt_history.end())
        return;

    int tx_count = -1;
    Time encode_time = MicroSeconds(0);
    Time now = Simulator::Now();

    std::deque<Ptr<DataPacket>> data_pkt_queue;

    auto group_data_pkt = &this->data_pkt_history[group_id];
    // find all packets that belong to the same group and retransmit them
    for(auto it = group_data_pkt->begin();it != group_data_pkt->end();it ++) {

        Ptr<DataPacket> data_pkt = it->second;

        if(tx_count == -1) tx_count = data_pkt->GetTXCount() + 1;
        if(encode_time == MicroSeconds(0)) encode_time = data_pkt->GetEncodeTime();

        data_pkt->SetTXCount(tx_count);
        data_pkt->SetEnqueueTime(now);
        data_pkt->ClearFECBatch();
        data_pkt_queue.push_back(data_pkt);
    }
    // std::cout << "[Server] At " << Simulator::Now().GetMilliSeconds() << " ms, retransmitting group " << group_id << " with " << data_pkt_queue.size() << " packets, tx_count = " << tx_count << '\n';

    if(data_pkt_queue.size() != 0) {
        // all packets in the same group belong to the same frame
        uint32_t frame_id = data_pkt_queue.front()->GetFrameId();
        this->SendPackets(data_pkt_queue, this->delay_ddl - (now - encode_time), frame_id, true);
    }
}

void PacketSenderUdp::CheckRetransmission() {
    Time now = Simulator::Now();
    auto statistic = this->GetNetworkStatistics();

    /* 1) check for packets that will definitely miss ddl */
   for(auto it = data_pkt_history_key.begin(); it != data_pkt_history_key.end();) {
        Ptr<GroupPacketInfo> info = (*it);

        // if we cannot find it in data_pkt_history,
        if(this->data_pkt_history.find(info->group_id) == this->data_pkt_history.end()) {
            it = this->data_pkt_history_key.erase(it);
            continue;
        }
        auto group_data_pkt = &this->data_pkt_history[info->group_id];
        if(group_data_pkt->find(info->pkt_id_in_group) == group_data_pkt->end()) {
            it = this->data_pkt_history_key.erase(it);
            continue;
        }

        auto pkt = this->data_pkt_history[info->group_id][info->pkt_id_in_group];

        // we can find it in data_pkt_history, check if it's timed out
        if(this->MissesDdl(pkt)) {
            // std::cout << "[Server] Remove group " << pkt->GetGroupId() << " pkt " << pkt->GetPktIdGroup() << " due to missing ddl" << '\n';
            // remove it in this->data_pkt_history
            this->data_pkt_history[info->group_id].erase(info->pkt_id_in_group);
            if(this->data_pkt_history[info->group_id].size() == 0)
                this->data_pkt_history.erase(info->group_id);
            it = this->data_pkt_history_key.erase(it);
        } else
            // since it's a FIFO queue
            // none of the packets behind the first non-timeout packet will timeout
            break;
    }

    /* 2) check for packets that exceeds rtx timer, needs to be retransmitted */
    std::unordered_set<uint32_t> rtx_group_id;
    for(auto it = data_pkt_history_key.begin(); it != data_pkt_history_key.end();) {
        Ptr<GroupPacketInfo> info = (*it);

        // std::cout << "[Server] At " << Simulator::Now().GetMilliSeconds() << " ms Checking group " << info->group_id << " pkt " << info->pkt_id_in_group << '\n';
        // if we cannot find it in data_pkt_history
        if(this->data_pkt_history.find(info->group_id) ==this->data_pkt_history.end()) {
            it = this->data_pkt_history_key.erase(it);
            continue;
        }
        auto group_data_pkt = &this->data_pkt_history[info->group_id];
        if(group_data_pkt->find(info->pkt_id_in_group) == group_data_pkt->end()) {
            it = this->data_pkt_history_key.erase(it);
            continue;
        }

        auto pkt = this->data_pkt_history[info->group_id][info->pkt_id_in_group];

        // we can find it in data_pkt_history, check if it's timed out
        // if this packet has not arrived at client
        // do not retransmit it and all the packets behind
        if(now - pkt->GetEncodeTime() < statistic->min_rtt) {
            break;
        }

        // this group has just been retransmitted
        if(rtx_group_id.find(pkt->GetGroupId()) != rtx_group_id.end()) {
            it ++;
            continue;
        }

        if(this->IsRtxTimeout(pkt)) {
            // group all packets that are of the same group
            // into a batch and send out
            uint32_t group_id = pkt->GetGroupId();
            this->RetransmitGroup(group_id);
            rtx_group_id.insert(group_id);
        } else {
            // NS_LOG_ERROR("[Server] At " << Simulator::Now().GetMilliSeconds() << " ms Do not retransmit group " << pkt->GetGroupId() << " pkt " << pkt->GetPktIdGroup() << ", whose encode time is " << pkt->GetEncodeTime().GetMilliSeconds() << " ms and last send time is " << pkt->GetSendTime().GetMilliSeconds() << " ms");
        }

        it ++;
    }

    this->check_rtx_event = Simulator::Schedule(this->check_rtx_interval, &PacketSenderUdp::CheckRetransmission, this);
};

/* Remove packet history records when we receive an ACK packet */
void PacketSenderUdp::RcvACKPacket(Ptr<AckPacket> ack_pkt) {
    std::vector<Ptr<GroupPacketInfo>> pkt_infos = ack_pkt->GetAckedPktInfos();

    for(Ptr<GroupPacketInfo> pkt_info : pkt_infos) {
        // std::cout << "ACK: (" << pkt_info->group_id << ", " <<pkt_info->pkt_id_in_group << "), ";
        NS_LOG_FUNCTION(pkt_info->group_id << pkt_info->pkt_id_in_group);
        
        // std::cout << "[Server] At " << Simulator::Now().GetMilliSeconds() << " ms Received an ACK of group " << pkt_info->group_id << ", pkt " << pkt_info->pkt_id_in_group << '\n';

        // find the packet in this->data_pkt_history
        if(this->data_pkt_history.find(pkt_info->group_id) != this->data_pkt_history.end()) {
            auto group_data_pkt = &this->data_pkt_history[pkt_info->group_id];
            if(group_data_pkt->find(pkt_info->pkt_id_in_group) != group_data_pkt->end()) {
                // erase pkt_id_in_frame
                group_data_pkt->erase(pkt_info->pkt_id_in_group);
            }
            // erase frame_id if neccesary
            if(this->data_pkt_history[pkt_info->group_id].size() == 0) {
                this->data_pkt_history.erase(pkt_info->group_id);
            }
        }

        // we do not need to remove entry in data_pkt_history_key
        // send CheckRetransmission() will do this for us
    }
};

void PacketSenderUdp::SendFrame (std::vector<Ptr<VideoPacket>> packets)
{
    this->UpdateGoodputRatio();
    Ptr<PacketFrame> newFrame = Create<PacketFrame>(packets,false);
    newFrame->Frame_encode_time_ = packets[0]->GetEncodeTime();
    m_queue.push_back(newFrame);
    this->Calculate_pacing_rate();
    if (m_pacing) {
        if(m_pacingTimer.IsExpired()){
            m_pacingTimer.Schedule();
        }
        else {
            m_pacingTimer.Cancel();
            m_pacingTimer.Schedule();
        }
    }
    else {
        m_eventSend = Simulator::ScheduleNow(&PacketSenderUdp::SendPacket,this);
    }
};

void PacketSenderUdp::SendRtx (std::vector<Ptr<VideoPacket>> packets)
{
    NS_LOG_FUNCTION("At time " << Simulator::Now().GetMilliSeconds() << ", " << packets.size() << " RTX packets are enqueued");
    Ptr<PacketFrame> newFrame = Create<PacketFrame>(packets,true);
    newFrame->Frame_encode_time_ = packets[0]->GetEncodeTime();
    m_queue.insert(m_queue.begin(),newFrame);
    this->Calculate_pacing_rate();
    if(m_pacing){
        if(m_pacingTimer.IsExpired()){
           m_pacingTimer.Schedule();
        }
    }
    else {
        m_eventSend = Simulator::ScheduleNow(&PacketSenderUdp::SendPacket,this);
    }
};

void PacketSenderUdp::Calculate_pacing_rate()
{
    Time time_now = Simulator::Now();
    uint32_t num_packets_left = 0;
    for(uint32_t i=0;i<this->num_frame_in_queue();i++) {
        if(m_queue[i]->retransmission) {
            continue;
        }
        num_packets_left = num_packets_left + m_queue[i]->Frame_size_in_packet();
        if(num_packets_left < 1) {
            NS_LOG_ERROR("Number of packet should not be zero.");
        }
        Time time_before_ddl_left = m_send_time_limit + m_queue[i]->Frame_encode_time_ - time_now;
        Time interval_max = time_before_ddl_left / num_packets_left;
        if(time_before_ddl_left <= Time(0)) {
            // TODO: DDL miss appears certain, need to require new IDR Frame from GameServer?
            NS_LOG_ERROR("DDL miss appears certain.");
        }
        else {
            if(interval_max < m_pacing_interval) {
                m_pacing_interval = interval_max;
                m_pacingTimer.SetDelay(interval_max);
            }
        }
    }
};

void PacketSenderUdp::OnSocketRecv_sender(Ptr<Socket> socket)
{
    Ptr<Packet> pkt = socket->Recv();
    Ptr<NetworkPacket> packet = NetworkPacket::ToInstance(pkt);
    auto pkt_type = packet->GetPacketType();
    Time now = Simulator::Now();

    if (pkt_type == ACK_PKT) {
        NS_LOG_FUNCTION("ACK packet received!");
        Ptr<AckPacket> ack_pkt = DynamicCast<AckPacket, NetworkPacket> (packet);
        // hand to GameServer for retransmission
        RcvACKPacket(ack_pkt);

        // Update RTT and inter-packet delay
        uint16_t pkt_id = ack_pkt->GetLastPktId();
        // std::cout << "[Sender] At " << Simulator::Now().GetMilliSeconds() << " ms rcvd ACK for packet " << pkt_id << '\n';
        if (this->pktsHistory.find(pkt_id) != this->pktsHistory.end()) {
            // RTT
            Ptr<SentPacketInfo> current_pkt = this->pktsHistory[pkt_id];
            current_pkt->pkt_ack_time = now;
            Time rtt = now - current_pkt->pkt_send_time;
            if (pkt_id >= m_last_acked_global_id) {
                if (!this->trace_set) UpdateRTT(rtt);
                // std::cout << "[PacketSenderUdp] RcvAck: now " << now 
                //     << " pkt send time " << current_pkt->pkt_send_time
                //     << " rtt " << rtt 
                //     << " pkt id " << pkt_id << '\n';
            }
            
            m_last_acked_global_id = pkt_id;

            // inter-packet delay
            if (this->pktsHistory.find(pkt_id - 1) != this->pktsHistory.end()) {
                Ptr<SentPacketInfo> last_pkt = this->pktsHistory[pkt_id - 1];
                if (last_pkt->batch_id == current_pkt->batch_id
                    && last_pkt->pkt_send_time == current_pkt->pkt_send_time
                    && last_pkt->pkt_ack_time != MicroSeconds(0)
                    && now >= last_pkt->pkt_ack_time) {
                    Time inter_pkt_delay = now - last_pkt->pkt_ack_time;
                    if (m_netStat->rt_dispersion == MicroSeconds(0))
                        m_netStat->rt_dispersion = inter_pkt_delay;
                    else m_netStat->rt_dispersion =
                        0.2 * inter_pkt_delay + 0.8 * m_netStat->rt_dispersion;
                }
            }
        }
        return;
    }

    if (pkt_type == FRAME_ACK_PKT) {
        NS_LOG_FUNCTION("Frame ACK packet received!");
        Ptr<FrameAckPacket> frame_ack_pkt = DynamicCast<FrameAckPacket, NetworkPacket> (packet);
        uint32_t frame_id = frame_ack_pkt->GetFrameId();
        Time frame_encode_time = frame_ack_pkt->GetFrameEncodeTime();
        if(now - frame_encode_time <= m_delay_ddl + MilliSeconds(1)) {
            m_finished_frame_cnt ++;
            if(frame_id >= 10) m_exclude_ten_finished_frame_cnt ++;
        } else {
            m_timeout_frame_cnt ++;
        }
        return;
    }

    // netstate packet
    NS_ASSERT_MSG(pkt_type == NETSTATE_PKT, "Sender should receive FRAME_ACK_PKT, ACK_PKT or NETSTATE_PKT");
    Ptr<NetStatePacket> netstate_pkt = DynamicCast<NetStatePacket, NetworkPacket> (packet);
    auto states = netstate_pkt->GetNetStates();
    if(!this->trace_set){
        m_netStat->current_bw = ((double_t)states->throughput_kbps) / 1000.;
        m_netStat->current_loss_rate = states->loss_rate;
        if(m_controller)
            m_controller->UpdateLossRate(uint8_t (states->loss_rate * 256));
    }
    m_netStat->loss_seq = states->loss_seq;
    m_netStat->one_way_dispersion = MicroSeconds(states->fec_group_delay_us);

    if (m_cc_enable) {
        uint64_t now_us = Simulator::Now().GetMicroSeconds();

        for (auto recvtime_item : states->recvtime_hist){
            uint16_t id = recvtime_item->pkt_id;
            uint64_t RxTime = recvtime_item->rt_us;
            uint64_t TxTime = m_controller->GetPacketTxTimestamp(id);
            // NS_ASSERT_MSG((RxTime <= now_us), "Receiving event and feedback event should be time-ordered.");
            if (m_firstFeedback) {
                m_prev_id = id;
                m_prev_RxTime = RxTime;
                m_group_id = 0;
                m_group_size = m_controller->GetPacketSize(id);
                m_groupstart_TxTime = TxTime;
                m_firstFeedback = false;
                m_curr_pkts_in_frame = 1;
                continue;
            }
            
            if ((lessThan_64(m_groupstart_TxTime, TxTime) && TxTime - m_groupstart_TxTime <= 20 * m_interval * 1000) || 
                m_groupstart_TxTime == TxTime) {
                if ((TxTime - m_groupstart_TxTime) > 10000) {
                    // Switching to another burst (named as group)
                    // update inter arrival and inter departure
                    if (m_group_id > 0) {
                        NS_ASSERT_MSG (m_prev_pkts_in_frame > 0, "Consecutive frame must have pkts!");
                        inter_arrival = m_curr_frame_RxTime / m_curr_pkts_in_frame - m_prev_frame_RxTime / m_prev_pkts_in_frame;
                        inter_departure = m_curr_frame_TxTime / m_curr_pkts_in_frame - m_prev_frame_TxTime / m_prev_pkts_in_frame;
                        inter_delay_var = inter_arrival - inter_departure;
                        inter_group_size = m_group_size - m_prev_group_size;
                        m_controller->processFeedback(now_us, id, RxTime, inter_arrival, inter_departure, inter_delay_var, inter_group_size, m_prev_RxTime);
                    }

                    // update group information
                    m_controller->PrunTransitHistory(m_prev_groupend_id);
                    m_prev_group_size = m_group_size;
                    m_prev_groupend_id = m_prev_id;
                    m_prev_groupend_RxTime = m_prev_RxTime;
                    m_group_id += 1;
                    m_group_size = 0;
                    m_groupstart_TxTime = TxTime;
                    m_prev_frame_TxTime = m_curr_frame_TxTime;
                    m_prev_frame_RxTime = m_curr_frame_RxTime;
                    m_prev_pkts_in_frame = m_curr_pkts_in_frame;
                    m_curr_frame_TxTime = 0;
                    m_curr_frame_RxTime = 0;
                    m_curr_pkts_in_frame = 0;
                }

                m_curr_pkts_in_frame += 1;
                m_curr_frame_TxTime += TxTime;
                m_curr_frame_RxTime += RxTime;       

                m_group_size += m_controller->GetPacketSize(id);
                m_prev_id = id;
                m_prev_RxTime = RxTime;
            }
            else {
                std::cout << "group id: " << m_group_id
                          << ", group start tx: " << m_groupstart_TxTime
                          << ", tx: " << TxTime << std::endl;
            }
            
        }
        
        std::ofstream f_t("results/Time_sender.log", std::ios::app);
        std::ofstream f_rtt("results/rtt_sender.log", std::ios::app);
        std::ofstream f_lr("results/lr_sender.log", std::ios::app);
        std::ofstream f_bw("results/bw_sender.log", std::ios::app);
        if(!f_t || !f_rtt || !f_lr || !f_bw){
            std::cout<<"File open error"<<std::endl;
        }
        else{
            float time_s = float(now_us / 1000000);
            float rtt =  float(m_netStat->current_rtt.GetMilliSeconds());
            float lossrate = float(m_netStat->current_loss_rate);
            float bw = m_netStat->current_bw;
            f_t << time_s << std::endl;
            f_rtt << rtt << std::endl;
            f_lr << lossrate << std::endl;
            f_bw << bw << std::endl;
            f_rtt.close();
            f_t.close();
            f_lr.close();
            f_bw.close();
        }
    }
};

Ptr<FECPolicy::NetStat> PacketSenderUdp::GetNetworkStatistics() { return m_netStat; };

double_t PacketSenderUdp::GetBandwidthLossRate() {
    double_t bandwidth_loss_rate_count, bandwidth_loss_rate_size;
    if(this->init_data_pkt_count + this->other_pkt_count == 0)
        return 0;
    bandwidth_loss_rate_count =
        ((double_t) this->other_pkt_count) /
        (this->init_data_pkt_count);
    bandwidth_loss_rate_size =
        ((double_t) this->other_pkt_size) /
        (this->init_data_pkt_size);
    NS_LOG_ERROR("[Sender] Initial data packets: " << this->init_data_pkt_count << ", other packets:" << this->other_pkt_count);
    NS_LOG_ERROR("[Sender] Bandwidth loss rate: " << bandwidth_loss_rate_count * 100 << "% (count), " << bandwidth_loss_rate_size * 100 << "% (size)");

    if(this->exclude_head_init_data_pkt_count > 0) {
        double_t exclude_head_bandwidth_loss_rate_count, exclude_head_bandwidth_loss_rate_size;
        exclude_head_bandwidth_loss_rate_count =
            ((double_t) this->exclude_head_other_pkt_count) /
            (this->exclude_head_init_data_pkt_count);
        exclude_head_bandwidth_loss_rate_size =
            ((double_t) this->exclude_head_other_pkt_size) /
            (this->exclude_head_init_data_pkt_size);
        NS_LOG_ERROR("[Sender] [Result] Initial data packets: " << this->exclude_head_init_data_pkt_count << ", other packets:" << this->exclude_head_other_pkt_count);
        NS_LOG_ERROR("[Sender] [Result] Bandwidth loss rate: " << exclude_head_bandwidth_loss_rate_count * 100 << "% (count), " << exclude_head_bandwidth_loss_rate_size * 100 << "% (size)");
    }
    NS_LOG_ERROR("[Sender] Played frames: " << m_finished_frame_cnt << ", timeout frames: " << m_timeout_frame_cnt);
    NS_LOG_ERROR("[Sender] [Result] Played frames: " << m_exclude_ten_finished_frame_cnt);

    return bandwidth_loss_rate_count;
};

void PacketSenderUdp::UpdateSendingRate(){
    if (m_cc_enable) {
        m_bitrate = 0.85 * m_controller->getSendBps();
        *m_bwTrStream->GetStream () << Simulator::Now ().GetMilliSeconds () << 
            " FlowId " << m_srcPort << 
            " Bitrate(bps) " << (uint32_t) GetNetworkStatistics()->srtt.GetMilliSeconds() << 
            " Rtt(ms) " << m_bitrate << std::endl;
    }
};

double PacketSenderUdp::GetGoodputRatio() {
    return m_goodput_ratio;
};

void PacketSenderUdp::UpdateGoodputRatio() {
    if(this->total_pkts_inwnd > 1 && this->goodput_pkts_inwnd > 0) {
        m_goodput_ratio = (double)(this->goodput_pkts_inwnd) / (double)(this->total_pkts_inwnd);
    }
};

void PacketSenderUdp::UpdateNetstateByTrace()
{
    // Load trace file
    std::ifstream trace_file;
    trace_file.open(this->trace_filename);

    // Initialize storing variables
    std::string trace_line;
    std::vector<std::string> trace_data;
    std::vector<std::string> bw_value;
    std::vector<std::string> rtt_value;
    uint16_t rtt;
    double_t bw;
    double_t lr;

    uint64_t past_time = 0;

    // Set netstate for every tracefile line
    while (std::getline(trace_file,trace_line))
    {   
        rtt_value.clear();
        bw_value.clear();
        trace_data.clear();
        SplitString(trace_line, trace_data," ");
        SplitString(trace_data[0], bw_value, "Mbps");
        SplitString(trace_data[1], rtt_value, "ms");
        rtt = (uint16_t) std::atof(rtt_value[0].c_str());
        bw = std::atof(bw_value[0].c_str());
        lr = std::atof(trace_data[2].c_str());
        m_settrace_event = Simulator::Schedule(
            MilliSeconds(rtt + past_time),&PacketSenderUdp::SetNetworkStatisticsBytrace, 
            this, rtt, bw, lr);
        past_time += m_interval;
    }
    
};

void PacketSenderUdp::SetTrace(std::string tracefile)
{
    this->trace_set = true;
    this->trace_filename = tracefile;
};

void PacketSenderUdp::SetTimeLimit (const Time& limit)
{
    m_send_time_limit = limit;
};

void PacketSenderUdp::Pause()
{
    if (m_pacing) {
        m_pacingTimer.Cancel();
    }
    else {
        Simulator::Cancel(m_eventSend);
    }
};

void PacketSenderUdp::Resume()
{
    if (m_pacing) {
        m_pacingTimer.Schedule();
    }
    else {
        m_eventSend = Simulator::ScheduleNow(&PacketSenderUdp::SendPacket,this);
    }
};

void PacketSenderUdp::SendPacket()
{
    Time time_now = Simulator::Now();
    uint64_t NowUs = time_now.GetMicroSeconds();
    if(this->num_frame_in_queue() > 0){
        std::vector<Ptr<VideoPacket>>* current_frame = &m_queue[0]->packets_in_Frame;
        std::vector<Ptr<VideoPacket>>::iterator first_packet = current_frame->begin();
        Ptr<VideoPacket> network_packet_to_send = *first_packet;
        network_packet_to_send->SetSendTime(time_now);
        network_packet_to_send->SetGlobalId(this->IPIDCounter);
        Ptr<Packet> pkt_to_send = network_packet_to_send->ToNetPacket();

        PacketType pkt_type = network_packet_to_send->GetPacketType();
        bool is_goodput = (pkt_type == PacketType::DATA_PKT)
                            && (network_packet_to_send->GetTXCount() == 0);
        uint16_t pkt_size = pkt_to_send->GetSize();
        Ptr<SentPacketInfo> pkt_info = Create<SentPacketInfo>(this->IPIDCounter,
                    network_packet_to_send->GetBatchId(),
                    time_now, pkt_type, is_goodput, pkt_size);

        if (m_cc_enable) {
            // handle pkt information to cc controller
            m_controller->processSendPacket(NowUs, this->IPIDCounter, pkt_to_send->GetSize());
        }

        // statistics
        uint32_t group_id = network_packet_to_send->GetGroupId();
        if (pkt_type == PacketType::DATA_PKT) {
            Ptr<DataPacket> data_pkt = DynamicCast<DataPacket, VideoPacket> (network_packet_to_send);
            if(data_pkt->GetFrameId() < 10) {
                this->first_ten_frame_group_id.insert(group_id);
            }
        }
        if (is_goodput) {
            this->init_data_pkt_count ++;
            this->init_data_pkt_size += pkt_size;
            if(this->first_ten_frame_group_id.find(group_id) == this->first_ten_frame_group_id.end()) {
                this->exclude_head_init_data_pkt_count ++;
                this->exclude_head_init_data_pkt_size += pkt_size;
            }
            this->goodput_pkts_inwnd += pkt_size;
        } 
        else {
            this->other_pkt_count ++;
            this->other_pkt_size += pkt_size;
            if(this->first_ten_frame_group_id.find(group_id) == this->first_ten_frame_group_id.end()) {
                this->exclude_head_other_pkt_count ++;
                this->exclude_head_other_pkt_size += pkt_size;
            }
        }
        this->total_pkts_inwnd += pkt_size;
        m_socket->Send(pkt_to_send);
        current_frame->erase(first_packet);
        this->pktsHistory[this->IPIDCounter] = pkt_info;
        m_send_wnd.push_back(this->IPIDCounter);
        m_goodput_wnd.push_back(this->IPIDCounter);

        while (!m_goodput_wnd.empty()){
            uint16_t id_out_of_date = m_goodput_wnd.front();
            //NS_ASSERT_MSG(this->pktsHistory.find(id_out_of_date)!=this->pktsHistory.end(),"History should record every packet sent.");
            Ptr<SentPacketInfo> info_out_of_date = this->pktsHistory[id_out_of_date];
            if((uint64_t)(info_out_of_date->pkt_send_time.GetMicroSeconds()) < NowUs - m_goodput_wnd_size){
                this->total_pkts_inwnd -= info_out_of_date->pkt_size;
                if(info_out_of_date->is_goodput){
                    this->goodput_pkts_inwnd -= info_out_of_date->pkt_size;
                }
                m_goodput_wnd.pop_front();
            }
            else {
                break;
            }
        }

        while (m_send_wnd.size() > m_send_wnd_size) {
            uint16_t id_out_of_date = m_send_wnd.front();
            NS_ASSERT_MSG(this->pktsHistory.find(id_out_of_date)!=this->pktsHistory.end(), "History should record every packet sent.");
            this->pktsHistory.erase(id_out_of_date);
            m_send_wnd.pop_front();
        }

        this->IPIDCounter = (this->IPIDCounter + 1) % 65536;

        if(current_frame->empty()){
            m_queue.erase(m_queue.begin());
            this->Calculate_pacing_rate();
        }
        if(m_pacing){
            m_pacingTimer.Schedule();
        }
        else {
            m_eventSend = Simulator::ScheduleNow(&PacketSenderUdp::SendPacket,this);
        }

    }
};

uint32_t PacketSenderUdp::num_frame_in_queue()
{
    return m_queue.size();
};

}; // namespace ns3