#include "video-decoder.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE("VideoDecoder");

TypeId VideoFrame::GetTypeId() {
    static TypeId tid = TypeId ("ns3::VideoFrame")
        .SetParent<Object> ()
        .SetGroupName("bitrate-ctrl")
    ;
    return tid;
};

VideoFrame::VideoFrame(Ptr<DataPacket> pkt) {
    this->group_ids.insert(pkt->GetGroupId());
    this->data_pkt_num = pkt->GetFramePktNum();
    this->first_pkt_rcv_time = pkt->GetRcvTime();
    this->last_pkt_rcv_time = pkt->GetRcvTime();
    this->encode_time = pkt->GetEncodeTime();
    this->AddPacket(pkt);
};

VideoFrame::~VideoFrame() {};

void VideoFrame::AddPacket(Ptr<DataPacket> pkt) {
    if(pkt->GetRcvTime() > this->last_pkt_rcv_time)
        this->last_pkt_rcv_time = pkt->GetRcvTime();
    if(pkt->GetEncodeTime() < this->encode_time)
        this->encode_time = pkt->GetEncodeTime();
    this->pkts.insert(pkt->GetPktIdFrame());
};

uint16_t VideoFrame::GetDataPktNum() { return this->data_pkt_num; };

uint16_t VideoFrame::GetDataPktRcvedNum() { return this->pkts.size(); };

std::unordered_set<uint16_t> VideoFrame::GetGroupIds() { return this->group_ids; };

Time VideoFrame::GetFrameDelay() {
    return last_pkt_rcv_time - encode_time;
};

Time VideoFrame::GetEncodeTime() {
    return this->encode_time;
};

Time VideoFrame::GetLastRcvTime() {
    return this->last_pkt_rcv_time;
}


TypeId VideoDecoder::GetTypeId() {
    static TypeId tid = TypeId ("ns3::VideoDecoder")
        .SetParent<Object> ()
        .SetGroupName("bitrate-ctrl")
        .AddConstructor<VideoDecoder> ()
    ;
    return tid;
};

VideoDecoder::VideoDecoder(Time delay_ddl, uint16_t fps, GameClient * game_client, void (GameClient::*ReplyFrameAck)(uint32_t, Time)) {
    this->delay_ddl = delay_ddl;
    this->min_frame_id = 0;
    this->max_frame_id = 0;
    m_lastPlayedFrameId = -1;
    this->game_client = game_client;
    this->ReplyFrameAck = ReplyFrameAck;
    m_frameInterval = 1000 / fps;
};

VideoDecoder::VideoDecoder() {

};

VideoDecoder::~VideoDecoder() {

};

void VideoDecoder::DecodeDataPacket(std::vector<Ptr<DataPacket>> pkts) {
    for (auto pkt : pkts) {
        uint32_t frame_id = pkt->GetFrameId();
        if (this->played_frames.find(frame_id) != this->played_frames.end()) {
            continue;
        }

        /* Remove frames that are ddl away from the current frame.
        In practice, new key frame should have already been generated */
        for (uint32_t idx = m_lastPlayedFrameId + 1; 
             idx <= frame_id - delay_ddl.GetMilliSeconds() / m_frameInterval; 
             idx ++) {
            this->played_frames[idx] = this->unplayed_frames[idx];
            this->unplayed_frames.erase(idx);
            std::cout << "discard frame: " << idx << " time: " << Simulator::Now().GetMilliSeconds() << '\n';
            m_lastPlayedFrameId = idx;
            for (uint32_t ith = idx + 1; ith <= this->max_frame_id; ith++){
                if (this->unplayed_frames.find(ith) != this->unplayed_frames.end()){
                    if(this->unplayed_frames[ith]->GetDataPktNum() == this->unplayed_frames[ith]->GetDataPktRcvedNum()){
                        this->played_frames[ith] = this->unplayed_frames[ith];
                        this->unplayed_frames.erase(ith);     
                        m_lastPlayedFrameId = ith;
                        Time time_now = Simulator::Now();
                        std::cout<<"decode: frame: " << ith <<" time: " << time_now.GetMilliSeconds() << std::endl;                    
                    }
                    else{
                        break;
                    }
                }
                else{
                    break;
                }
            }
        }
        
        this->max_frame_id = MAX(this->max_frame_id, frame_id);
        this->min_frame_id = MIN(this->min_frame_id, frame_id);
        if (this->unplayed_frames.find(frame_id) == this->unplayed_frames.end()) {
            this->unplayed_frames[frame_id] = Create<VideoFrame> (pkt);
        }
        else
            this->unplayed_frames[frame_id]->AddPacket(pkt);
        if (this->unplayed_frames[frame_id]->GetDataPktNum()
            == this->unplayed_frames[frame_id]->GetDataPktRcvedNum()) {
                if (frame_id == 0) {
                    this->played_frames[frame_id] = this->unplayed_frames[frame_id];
                    this->unplayed_frames.erase(frame_id);   
                    m_lastPlayedFrameId = 0;    
                    Time time_now = Simulator::Now();
                    std::cout << "decode: frame: " << frame_id << " time: " << time_now.GetMilliSeconds() << std::endl;         
                }
                else if (frame_id == (m_lastPlayedFrameId + 1)) {
                  for (uint32_t idx = frame_id; idx <= this->max_frame_id; idx ++) {
                        if (this->unplayed_frames.find(idx) != this->unplayed_frames.end()){
                            if(this->unplayed_frames[idx]->GetDataPktNum() == this->unplayed_frames[idx]->GetDataPktRcvedNum()){
                                this->played_frames[idx] = this->unplayed_frames[idx];
                                this->unplayed_frames.erase(idx);     
                                m_lastPlayedFrameId = idx;
                                Time time_now = Simulator::Now();
                                std::cout<<"decode: frame: " << idx <<" time: " << time_now.GetMilliSeconds() << std::endl;                    
                                if (idx >= 10) this->exclude_ten_played_frames.insert(idx);
                                ((this->game_client)->*ReplyFrameAck)(idx, this->played_frames[idx]->GetEncodeTime());
                                if (idx > frame_id){
                                   std::cout << "rtx decode: " << idx << " " << frame_id << std::endl;
                                }
                            }
                            else {
                                break;
                            }
                        }
                        else {
                            break;
                        }
                  }           
                }
                else{
                    std::cout<<"rtx frame not filled: "<<frame_id<<std::endl;
                }
        }
        else {
            if(this->unplayed_frames[frame_id]->GetDataPktRcvedNum() > (this->unplayed_frames[frame_id]->GetDataPktNum() -5)){
            //   std::cout<<"not all pkts: "<<frame_id<<" receive: "<<this->unplayed_frames[frame_id]->GetDataPktRcvedNum()<<" need: "<<this->unplayed_frames[frame_id]->GetDataPktNum()<<std::endl;
            }    
        }
    }
};

double_t VideoDecoder::GetDDLMissRate() {
    /* ddl miss rate = missed_frames / all_frames */
    uint64_t frame_rcvd_cnt = played_frames.size(),
        frame_total_cnt = this->max_frame_id - this->min_frame_id + 1;
    if(frame_total_cnt == 0)  return 0;
    double_t ddl_miss_rate = ((double_t) frame_total_cnt - frame_rcvd_cnt) / frame_total_cnt;
    NS_LOG_ERROR("[Decoder] Total frames: " << frame_total_cnt << ", played frames: " << played_frames.size() << ", unplayed frames: " << unplayed_frames.size());
    NS_LOG_ERROR("[Decoder] [Result] Played frames: " << exclude_ten_played_frames.size());
    NS_LOG_ERROR("[Decoder] DDL Miss Rate: " << ddl_miss_rate * 100 << "%");

    /* avg latency */
    double_t avg_latency = 0;
    double_t exclude_ten_avg_latency = 0;
    for(auto it = played_frames.begin();it != played_frames.end();it ++) {
        avg_latency += it->second->GetFrameDelay().GetMilliSeconds();
        if(it->first >= 10)
            exclude_ten_avg_latency += it->second->GetFrameDelay().GetMilliSeconds();
    }
    avg_latency /= played_frames.size() == 0 ? 1 : played_frames.size();
    NS_LOG_ERROR("[Decoder] Average Latency: " << avg_latency <<  " ms\n");
    exclude_ten_avg_latency /= exclude_ten_played_frames.size() == 0 ? 1 : exclude_ten_played_frames.size();
    NS_LOG_ERROR("[Decoder] [Result] Average Latency: " << exclude_ten_avg_latency <<  " ms\n");

    return ddl_miss_rate;
};
};