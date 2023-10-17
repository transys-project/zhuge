#ifndef HAIRPIN_POLICY_H
#define HAIRPIN_POLICY_H

#include "fec-policy.h"
#include "ns3/fec-array.h"
#include "ns3/object.h"
#include "ns3/nstime.h"
#include <vector>

namespace ns3{

class HairpinPolicy : public FECPolicy {
public:
    HairpinPolicy(uint16_t delay_ddl, double_t qoe_coeff);
    HairpinPolicy();
    ~HairpinPolicy();
    static TypeId GetTypeId (void);
private:
    static const std::vector<uint8_t> group_size_list;
    uint16_t delay_ddl;
    double_t qoe_coeff;
    static const int GROUP_SIZE_ITVL = 5;
private:
    /**
     * @brief Apply group size to historic packet lost pattern, return the distribution of packet losses per group
     *
     * @param statistic Loss, rtt, bandwidth, loss rate and FEC group delay per packet
     * @param group_size FEC group size
     * @param beta_0 FEC rate when packets are transmitted the first time
     * @return vector<double_t> distribution of packet losses per group, with length of group_size
     */
    std::vector<double_t> GetAverageLostPackets(std::vector<int> loss_seq, uint16_t group_size, double_t beta_0);
    /**
     * @brief Calculate QoE given deadline miss rate and bandwidth loss rate
     *
     * @param ddl_miss_rate The probability of a group of packets arrving at the client after deadline
     * @param bandwidth_loss_rate The ratio of FEC and retransmission packets to original data packets
     * @return double_t QoE
     */
    double_t QoE(double_t ddl_miss_rate, double_t bandwidth_loss_rate);
    /**
     * @brief Calculate combination number, choose k from n
     *
     * @param n Total number
     * @param k The number we take
     * @return uint32_t combination number
     */
    uint32_t Comb(uint8_t n, uint8_t k);
    /**
     * @brief Calculate translation probability from state<n0, r> to state<n1, r+1>
     *
     * @param n0 Number of data packets sent
     * @param n1 Number of data packets lost
     * @param loss_rate Network packet loss rate
     * @param fec_rate FEC rate for retransmission, must be integer
     * @return double_t Translation probability from state<n0, r> to state<n1, r+1>
     */
    double_t TransPbl(uint8_t n0, uint8_t n1, double_t loss_rate, uint8_t fec_rate);
    /**
     * @brief Calculate the probability of state<n, r> given all states in column r - 1
     *
     * @param column_vector Probability of every state in the last column
     * @param n State<n, r>
     * @param loss_rate Network packet loss rate
     * @param fec_rate FEC rate for retransmission, must be integer
     * @return double_t Probability of state<n, r>
     */
    double_t StatePbl(std::vector<double_t> column_vector, uint8_t n, double_t loss_rate, uint8_t fec_rate);
    /**
     * @brief Let the probability loss_count state in the Markov chain to 1 and calculate QoE
     *
     * @param group_size Data packet count in a FEC group
     * @param beta_1 FEC rate for retransmissions
     * @param loss_dis Distribution of packet losses per group, with length of group_size
     * @param loss_rate Measured packet loss rate in the network
     * @param rtt Measured round trip time in ms
     * @param ddl Deadline left after the first-round transmission
     * @param ddl_miss_rate Stores deadline miss rate
     * @param bw_loss_rate Stores bandwidth loss rate
     * @return double_t The expected QoE
     */
    void GetResult(uint8_t group_size, uint8_t beta_1, std::vector<double_t> loss_dis, double_t loss_rate, double_t rtt, double_t ddl, double_t & ddl_miss_rate, double_t & bw_loss_rate);
public:
    FECParam GetFECParam(
        Ptr<NetStat> statistic, uint32_t bitrate,
        uint16_t ddl, uint16_t ddl_left,
        bool is_rtx, uint8_t frame_size,
        uint16_t max_group_size, bool fix_group_size
    );
};  // class HairpinPolicy

}; // namespace ns3

#endif /* HAIRPIN_POLICY_H */