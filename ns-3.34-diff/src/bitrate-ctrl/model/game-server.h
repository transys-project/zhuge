#ifndef GAME_SERVER_H
#define GAME_SERVER_H

#include "packet-sender.h"
#include "packet-sender-tcp.h"
#include "packet-sender-udp.h"
#include "video-encoder.h"
#include "ns3/fec-policy.h"
#include "ns3/application.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/simulator.h"
#include "ns3/assert.h"
#include "ns3/socket.h"
#include <vector>
#include <deque>
#include <unordered_map>

namespace ns3 {

class PacketSender;

enum CC_ALG { NOT_USE_CC, GCC, NADA, SCREAM, Copa, ABC, Bbr, Cubic};

class GameServer : public Application {

/* Override Application */
public:
    static TypeId GetTypeId (void);
    /**
     * \brief Construct a new GameServer object
     *
     * \param fps Default video output config: frames per second
     * \param delay_ddl
     * \param bitrate in Kbps
     * \param pacing_flag
     */
    GameServer();
    //GameServer(uint8_t, Time, uint32_t, bool);
    ~GameServer();
    void Setup(
        Ipv4Address srcIP, uint16_t srcPort, Ipv4Address destIP, uint16_t destPort,
        Time delay_ddl, uint16_t interval, Ptr<FECPolicy> fecPolicy, 
        bool tcpEnabled, Ipv4Header::DscpType dscp, Ptr<OutputStreamWrapper> appTrStream, 
        Ptr<OutputStreamWrapper> bwTrStream
    );
    void SetController(std::string cca);
    void StopEncoding();
protected:
    void DoDispose();
private:
    void StartApplication();
    void StopApplication();

/* Packet sending logic */
private:
    Ptr<PacketSender> m_sender;       /* Pacing, socket operations */
    Ptr<VideoEncoder> m_encoder;      /* Provides encoded video data */
    Ptr<FECPolicy> policy;          /* FEC Policy */

    uint32_t kMinEncodeBps;
    uint32_t kMaxEncodeBps;

    uint32_t frame_id;  /* accumulated */
    uint32_t group_id;  /* accumulated */
    uint32_t batch_id;  /* for simplicity, batch_id is also globally incremental */
    uint8_t fps;        /* video fps */
    uint32_t bitrate;   /* in Kbps */
    uint16_t interval;  /* ms, pass the info to packet sender */
    Time delay_ddl;     /* delay ddl */
    bool pacing_flag;   /* flag for pacing */

    Time check_rtx_interval;
    EventId check_rtx_event; /* Timer for retransmisstion */

    Ipv4Address m_srcIP;
    uint16_t m_srcPort;
    Ipv4Address m_destIP;
    uint16_t m_destPort;

    /* Congestion Control-related variables */
    std::string m_cca;
    
    Time m_cc_interval;

    FECPolicy::FECParam m_curr_fecparam; /* record the latest FEC parameters for encoding bitrate convertion */

    float m_goodput_ratio; /* (data pkt number / all pkts sent) in a time window */

    Ptr<OutputStreamWrapper> m_appTrStream;

    // statistics
    uint64_t send_group_cnt;
    uint64_t send_frame_cnt;
    double kCopaDelta;

    uint32_t GetNextGroupId();
    uint32_t GetNextBatchId();
    uint32_t GetNextFrameId();

    /**
     * \brief Create a packet batch from pkt_batch
     *
     * \param pkt_batch Data packets for creating the batch, store the whole batch afterwards
     * \param fec_param FEC parameters
     * \param new_group flag to create a new group
     * \param group_id if new_group==false, use group_id instead of creating a new id
     * \param is_rtx
     */
    void CreatePacketBatch(
        std::vector<Ptr<VideoPacket>>& pkt_batch,
        FECPolicy::FECParam fec_param,
        bool new_group, uint32_t group_id, bool is_rtx
    );

    void CreateFirstPacketBatch(
    std::vector<Ptr<VideoPacket>>& pkt_batch, FECPolicy::FECParam fec_param);
    void CreateRTXPacketBatch(
    std::vector<Ptr<VideoPacket>>& pkt_batch, FECPolicy::FECParam fec_param);

    /**
     * \brief For incomplete group/batch, complete it with new data packets
     *
     * \param new_data_pkts new data packets for creating the batch, store the whole batch afterwards
     * \param fec_pkts Store FEC packets
     */
    void CompletePacketBatch(
        std::vector<Ptr<VideoPacket>>& new_data_pkts, std::vector<Ptr<VideoPacket>>& fec_pkts
    );

    /**
     * \brief Determine whether the packet will definitely exceed delay_ddl
     *
     * \param pkt
     *
     * \return true it will exceed delay_ddl
     * \return false it probably will not
     */
    bool MissesDdl(Ptr<DataPacket> packet);


    bool IsRtxTimeout(Ptr<DataPacket> packet);


    /**
     * \brief Store data packets for retransmission
     *
     * \param pkts
     */
    void StorePackets(std::vector<Ptr<VideoPacket>> pkts);

    void SendPackets(std::deque<Ptr<DataPacket>> pkts, Time ddl_left, uint32_t frame_id, bool is_rtx);

    void RetransmitGroup(uint32_t);

    /**
     * \brief Check for packets to be retransmitted regularly
     *
     */
    void CheckRetransmission();

    FECPolicy::FECParam GetFECParam(
        uint16_t max_group_size, 
        Time ddl_left, bool fix_group_size,
        bool is_rtx, uint32_t frame_id
    );

public:
    /**
     * \brief A frame of encoded data is provided by encoder. Called every frame.
     *
     * \param buffer Pointer to the start of encoded data
     * \param size Length of the data in bytes
     */
    void SendFrame(uint8_t * buffer, uint32_t size);

    /**
     * \brief Process ACK packet
     *
     * \param pkt contains info of the packets that have been received
     */
    void RcvACKPacket(Ptr<AckPacket> pkt);

    /**
     * \brief For packet_sender to get the UDP socket
     *
     * \return Ptr<Socket> a UDP socket
     */
    Ptr<Socket> GetSocket();

    /**
     * @brief Update sending bitrate in CC controller and report to video encoder.
     *
     */
    void UpdateBitrate(uint32_t);

    /**
     * @brief Print group info
     */
    void OutputStatistics();

};  // class GameServer

};  // namespace ns3

#endif  /* GAME_SERVER_H */