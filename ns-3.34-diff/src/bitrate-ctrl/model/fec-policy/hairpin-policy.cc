#include "hairpin-policy.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE("Hairpin");

/* class HairpinPolicy */
const std::vector<uint8_t> HairpinPolicy::group_size_list = {5, 15, 20, 25, 30, 35, 40, 45, 50, 55};

HairpinPolicy::HairpinPolicy(uint16_t delay_ddl, double_t qoe_coeff) : FECPolicy(MilliSeconds(1)) {
    this->qoe_coeff = qoe_coeff;
    this->delay_ddl = delay_ddl;
    this->pacing_flag = false;
};

HairpinPolicy::HairpinPolicy() : FECPolicy(MilliSeconds(1)) {};
HairpinPolicy::~HairpinPolicy() {};

TypeId HairpinPolicy::GetTypeId() {
    static TypeId tid = TypeId ("ns3::HairpinPolicy")
        .SetParent<FECPolicy> ()
        .SetGroupName("bitrate-ctrl")
        .AddConstructor<HairpinPolicy> ()
    ;
    return tid;
};

std::vector<double_t> HairpinPolicy::GetAverageLostPackets(std::vector<int> loss_seq, uint16_t group_size, double_t beta_0) {
    std::vector<double_t> loss_dis (group_size + 1, 0);

    int next_seq = 0;
    uint16_t group_count = 0;
    uint8_t fec_count = round(group_size * beta_0);
    uint16_t total_group_size = group_size + fec_count;
    uint16_t pkt_count, data_pkt_loss_in_group, fec_pkt_loss_in_group;
    // std::cout << "loss_seq_size: " << loss_seq.size() << "\t" << loss_seq[0] << "group size: " << group_size << '\n';
    while(loss_seq.size() != 0) {
        // fill a group
        pkt_count = 0;
        data_pkt_loss_in_group = 0;
        fec_pkt_loss_in_group = 0;
        while(pkt_count < total_group_size) {
            // fill packets from the sequence into a group
            if(next_seq == 0) {
                if(loss_seq.size() == 0) break;
                next_seq = loss_seq.back();
                loss_seq.pop_back();
            }
            if(next_seq > 0) {
                // consecutively receiving
                next_seq --;    /* remove by 1 */
                pkt_count ++;
            } else if(next_seq < 0) {
                // consecutive loss
                next_seq ++;    /* remove by 1 */
                if(pkt_count < group_size) data_pkt_loss_in_group ++;
                else fec_pkt_loss_in_group ++;
                pkt_count ++;
            }
        }
        if(pkt_count < total_group_size) {
            NS_ASSERT(loss_seq.size() == 0);
            break;
        }
        if(data_pkt_loss_in_group + fec_pkt_loss_in_group > fec_count)
            // cannot retrieve lost packets
            loss_dis[data_pkt_loss_in_group] ++;
        else
            loss_dis[0] ++;
        group_count ++;
    }
    if(group_count != 0) 
        for(uint16_t i = 0;i <= group_size;i++) {
            loss_dis[i] /= group_count;
        }
    else loss_dis[0] = 1;
    return loss_dis;
};

double_t HairpinPolicy::QoE(double_t ddl_miss_rate, double_t bandwidth_loss_rate) {
    return - ddl_miss_rate + this->qoe_coeff * (1 - bandwidth_loss_rate);
};

uint32_t HairpinPolicy::Comb(uint8_t n, uint8_t k)
{
    if (k > n) return 0;
    if (k * 2 > n) k = n-k;
    if (k == 0) return 1;

    int result = n;
    for( int i = 2; i <= k; ++i ) {
        result *= (n-i+1);
        result /= i;
    }
    return result;
}

double_t HairpinPolicy::TransPbl(uint8_t n0, uint8_t n1, double_t loss_rate, uint8_t fec_rate) {
    // dupllicate data packets as FEC packets
    double_t p01 = 0;
    if(fec_rate == 1) {
        if(n1 == 0) {
            for(uint8_t i = 0;i < n0;i++) {
                p01 += this->Comb(n0 * 2, i) * pow(loss_rate, i) * pow(1 - loss_rate, n0 * 2 - i);
            }
        } else {
            uint8_t min_loss_count = MAX(n0 + 1, n1);
            for(uint8_t i = min_loss_count;i < n1 + n0 + 1;i++) {
                p01 += pow(loss_rate, i) * pow(1 - loss_rate, n0 * 2 - i)
                    * this->Comb(n0, i - n1) * this->Comb(n0, n1);
            }
        }
    }
    else {
        p01 = this->Comb(n0, n1) * pow(pow(loss_rate, fec_rate + 1), n1) * pow(1 - pow(loss_rate, fec_rate + 1), n0 - n1);
    }
    return p01;
};

double_t HairpinPolicy::StatePbl(std::vector<double_t> column_vector, uint8_t n, double_t loss_rate, uint8_t fec_rate) {
    double_t state_pbl = 0;
    for(uint8_t i = MAX(n, 1);i < column_vector.size();i++) {
        state_pbl += column_vector[i] * this->TransPbl(i, n, loss_rate, fec_rate);
    }
    return state_pbl;
};

void HairpinPolicy::GetResult(uint8_t group_size, uint8_t beta_1, std::vector<double_t> loss_dis, double_t loss_rate, double_t rtt, double_t ddl, double_t & ddl_miss_rate, double_t & bw_loss_rate) {
    double_t on_time_rate = 0, extra_pkts = 0;
    ddl_miss_rate = 0;
    // Since state<n1, r> only needs results of state<n0, r - 1> (n0 <= n1),
    // we can use a single column vector to calculate the whole markov chain matrix
    NS_ASSERT(loss_dis.size() == group_size + 1u);
    double_t pbl_sum = 0;
    for(double_t pbl : loss_dis) pbl_sum += pbl;
    NS_ASSERT_MSG(abs(pbl_sum - 1) < 1e-10/* equal */, "Sum of Probability: " << pbl_sum); 
    // markov layers
    uint8_t layer = floor(ddl / rtt) + 1;
    layer += (ddl - rtt * (layer - 1)) > (rtt / 2) ? 1 : 0;
    // calculate markov chain
    for(uint8_t i = 0;i < layer;i++) {
        pbl_sum = 0;
        for(uint8_t j = 0;j <= group_size;j++) {
            if(i > 0)
                loss_dis[j] = this->StatePbl(loss_dis, j, loss_rate, beta_1);
            // on_time_rate
            if(j == 0) on_time_rate += loss_dis[j];
            // ddl_miss_rate
            else {
                pbl_sum += loss_dis[j];
                if(i == layer - 1) ddl_miss_rate += loss_dis[j];
                // extra_pkts, packets at the last layer will not be retrasmitted
                else if(i != layer - 1) extra_pkts += floor(j + j * beta_1) * loss_dis[j];
            }
        }
        // normalization: make sure miss_rate + on_time_rate == 1
        on_time_rate /= on_time_rate + pbl_sum;
        for(uint8_t j = 1;j <= group_size;j++) loss_dis[j] /= on_time_rate + pbl_sum;
        pbl_sum = 0;
    }
    bw_loss_rate = extra_pkts / (group_size + extra_pkts);
};

FECPolicy::FECParam HairpinPolicy::GetFECParam(
        Ptr<NetStat> statistic, uint32_t bitrate,
        uint16_t ddl, uint16_t ddl_left,
        bool is_rtx, uint8_t frame_size,
        uint16_t max_group_size, bool fix_group_size
    ) {
    uint8_t fec_count;
    uint8_t block_size;

    // Block size optimization
    bool block_size_opt = false;
    uint16_t srtt = (uint16_t) ceil((double_t) statistic->srtt.GetMicroSeconds() / 1e3);
    if(block_size_opt && (!fix_group_size && frame_size == max_group_size)) {
        // group size is not fixed
        get_block_size(
            statistic->current_loss_rate, frame_size,
            ddl, srtt,
            statistic->one_way_dispersion.GetMicroSeconds() / 1e3, &block_size
        );
    } else {
        // group size is fixed
        // if frame_size != max_group_size,
        // it means that it's not a first-time-transmitted data packet
        block_size = max_group_size;
    }
    
    get_fec_count(
        statistic->current_loss_rate, frame_size,
        ddl_left, srtt,
        block_size, &fec_count
    );
    if(fec_count == 255) 
        NS_ASSERT_MSG(false, "fec_count 255: loss " << statistic->current_loss_rate << ", frame size: " << frame_size << ", ddl_left: " << ddl_left << ", rtt: " << srtt << ", maxgroupsize: " << max_group_size);
    double_t fec_rate = ((double_t) fec_count) / block_size;

    // std::cout << "[Hairpin] loss " << statistic->current_loss_rate << ", frame size: " << (int) frame_size << ", ddl_left: " << ddl_left << ", rtt: " << srtt << ", block_size: " << max_group_size << ", fec_count: " << (int) fec_count << ", fec_rate: " << fec_rate << "\n";

    return FECParam(block_size, fec_rate);
};
}; // namespace ns3