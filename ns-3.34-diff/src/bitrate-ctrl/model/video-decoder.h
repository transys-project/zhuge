#ifndef VIDEO_DECODER_H
#define VIDEO_DECODER_H

#include "ns3/fec-policy.h"
#include "ns3/object.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <fstream>

namespace ns3 {

class GameClient;

class VideoFrame : public Object {
    uint16_t data_pkt_num;
    Time encode_time;
    Time first_pkt_rcv_time, last_pkt_rcv_time;
    std::unordered_set<uint16_t> group_ids;
    std::unordered_set<uint16_t> pkts;
public:
    static TypeId GetTypeId (void);
    VideoFrame(Ptr<DataPacket> pkt);
    ~VideoFrame();
    void AddPacket(Ptr<DataPacket> pkt);
    uint16_t GetDataPktNum();
    uint16_t GetDataPktRcvedNum();
    std::unordered_set<uint16_t> GetGroupIds();
    Time GetFrameDelay();
    Time GetEncodeTime();
    Time GetLastRcvTime();
};

class VideoDecoder : public Object {
public:
    static TypeId GetTypeId (void);
    VideoDecoder();
    VideoDecoder(Time, uint16_t, GameClient *, void (GameClient::*)(uint32_t, Time));
    ~VideoDecoder();

private:
    Time delay_ddl;
    std::unordered_map<uint32_t, Ptr<VideoFrame>> unplayed_frames;
    std::unordered_map<uint32_t, Ptr<VideoFrame>> played_frames;
    uint32_t min_frame_id;
    uint32_t max_frame_id;
    uint32_t m_lastPlayedFrameId;
    uint16_t m_frameInterval;

    std::set<uint32_t> exclude_ten_played_frames;

    GameClient * game_client;
    void (GameClient::*ReplyFrameAck)(uint32_t, Time);
public:
    void DecodeDataPacket(std::vector<Ptr<DataPacket>> pkts);
    double_t GetDDLMissRate();

}; // class VideoDecoder

}; // namespace ns3

#endif  /* VIDEO_DECODER_H */