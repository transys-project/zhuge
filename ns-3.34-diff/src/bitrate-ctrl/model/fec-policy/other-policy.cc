#include "other-policy.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE("OtherPolicies");


/* class FECOnlyPolicy */
FECOnlyPolicy::FECOnlyPolicy() : FECPolicy(MilliSeconds(1) /* no rtx */)  {};
FECOnlyPolicy::~FECOnlyPolicy() {};

TypeId FECOnlyPolicy::GetTypeId() {
    static TypeId tid = TypeId ("ns3::FECOnlyPolicy")
        .SetParent<FECPolicy> ()
        .SetGroupName("bitrate-ctrl")
        .AddConstructor<FECOnlyPolicy> ()
    ;
    return tid;
};

FECPolicy::FECParam FECOnlyPolicy::GetFECParam(
        Ptr<NetStat> statistic, uint32_t bitrate,
        uint16_t ddl, uint16_t ddl_left,
        bool is_rtx, uint8_t frame_size,
        uint16_t max_group_size, bool fix_group_size
    ) {
    uint8_t fec_count = 0;

    if(!is_rtx) {
        // remaining_time == ddl_left, so that layer == 1
        get_fec_count(
            statistic->current_loss_rate, frame_size,
            ddl_left, ddl_left,
            max_group_size, &fec_count
        );
        if(fec_count == 255) 
            NS_ASSERT_MSG(false, "fec_count 255: loss " << statistic->current_loss_rate << ", frame size: " << frame_size << ", ddl_left: " << ddl_left << ", rtt: " << statistic->srtt.GetMilliSeconds() << ", maxgroupsize: " << max_group_size);
    }
    
    double_t fec_rate = ((double_t) fec_count) / max_group_size;
    // std::cout << "[FECOnly] loss " << statistic->current_loss_rate << ", frame size: " << (int) frame_size << ", ddl_left: " << ddl_left << ", rtt: " << statistic->srtt.GetMilliSeconds() << ", block_size: " << max_group_size << ", fec_count: " << (int) fec_count << ", fec_rate: " << fec_rate << "\n";
    
    return FECParam(max_group_size, fec_rate);
};


/* class RTXOnlyPolicy */
RTXOnlyPolicy::RTXOnlyPolicy() : FECPolicy(MilliSeconds(1)) {};
RTXOnlyPolicy::~RTXOnlyPolicy() {};

TypeId RTXOnlyPolicy::GetTypeId() {
    static TypeId tid = TypeId ("ns3::RTXOnlyPolicy")
        .SetParent<FECPolicy> ()
        .SetGroupName("bitrate-ctrl")
        .AddConstructor<RTXOnlyPolicy> ()
    ;
    return tid;
};

FECPolicy::FECParam RTXOnlyPolicy::GetFECParam(
        Ptr<NetStat> statistic, uint32_t bitrate,
        uint16_t ddl, uint16_t ddl_left,
        bool is_rtx, uint8_t frame_size,
        uint16_t max_group_size, bool fix_group_size
    ) {
    return FECParam(max_group_size, 0);
};

/* class WebRTCPolicy */
WebRTCPolicy::WebRTCPolicy() : FECPolicy(MilliSeconds(1)) {
    this->fixed = false;
};
WebRTCPolicy::WebRTCPolicy(bool fixed, double_t loss_rate) : FECPolicy(MilliSeconds(1)) {
    this->fixed = fixed;
    this->loss_rate = loss_rate;
};
WebRTCPolicy::~WebRTCPolicy() {};

TypeId WebRTCPolicy::GetTypeId() {
    static TypeId tid = TypeId ("ns3::WebRTCPolicy")
        .SetParent<FECPolicy> ()
        .SetGroupName("bitrate-ctrl")
        .AddConstructor<WebRTCPolicy> ()
    ;
    return tid;
};

FECPolicy::FECParam WebRTCPolicy::GetFECParam(
        Ptr<NetStat> statistic, uint32_t bitrate,
        uint16_t ddl, uint16_t ddl_left,
        bool is_rtx, uint8_t frame_size,
        uint16_t max_group_size, bool fix_group_size
    ) {
    double_t fec_rate = 0;

    if(!is_rtx) {
        double_t loss = statistic->current_loss_rate;
        if(this->fixed) loss = this->loss_rate;
        if(max_group_size > 48)
            max_group_size = 48;
        get_fec_rate_webrtc(loss, max_group_size, ((double_t) bitrate) / 1000, &fec_rate);
    }
    fec_rate = MIN(fec_rate, 1);

    // if(fec_rate != 1) {
        // std::cout
        //     << "[WebrtcPolicy] RTT: " << statistic->current_rtt.GetMilliSeconds()
        //     << "ms, Loss rate: " << statistic->current_loss_rate
        //     << ", group delay: " << statistic->one_way_dispersion.GetMicroSeconds()
        //     << ", max group size: " << max_group_size
        //     << ", fec_rate: " << fec_rate
        //     << ", is rtx: " << is_rtx
        //     << ", now: " << Simulator::Now().GetMilliSeconds()
        //     << '\n';

    // }

    return FECParam(max_group_size, fec_rate);
};

/* class WebRTCStarPolicy */
WebRTCStarPolicy::WebRTCStarPolicy() : FECPolicy(MilliSeconds(1)) {
    this->fixed = false;
    this->order = 1;
};

WebRTCStarPolicy::WebRTCStarPolicy(int order) : FECPolicy(MilliSeconds(1)) {
    this->fixed = false;
    this->order = order;
};

WebRTCStarPolicy::WebRTCStarPolicy(bool fixed, double_t loss_rate, double_t rtt, int order) : FECPolicy(MilliSeconds(1)) {
    this->fixed = fixed;
    this->fixed_loss_rate = loss_rate;
    this->fixed_rtt = rtt;
    this->order = order;
};
WebRTCStarPolicy::~WebRTCStarPolicy() {};

TypeId WebRTCStarPolicy::GetTypeId() {
    static TypeId tid = TypeId ("ns3::WebRTCStarPolicy")
        .SetParent<FECPolicy> ()
        .SetGroupName("bitrate-ctrl")
        .AddConstructor<WebRTCStarPolicy> ()
    ;
    return tid;
};

double_t WebRTCStarPolicy::LinearFECRate(double_t beta, uint16_t ddl_left, uint16_t rtt){
    double_t rtt_to_ddl_left = (double_t) rtt / (double_t) ddl_left;
    double_t new_beta = (2. * beta * rtt_to_ddl_left < 1)? 2. * beta * rtt_to_ddl_left: 1.;
    // std::cout << "beta: "<<beta <<", rtt/ddl_left: "<<rtt_to_ddl_left<<", rtt: "<<rtt<<", ddl_left:"<<ddl_left<<", actual beta:" << new_beta <<"\n";
    return new_beta;
};

double_t WebRTCStarPolicy::QuadraticFECRate(double_t beta, uint16_t ddl_left, uint16_t rtt){
    double_t rtt_to_ddl_left = (double_t) rtt / (double_t) ddl_left;
    double_t new_beta = 4. * beta * rtt_to_ddl_left * rtt_to_ddl_left;
    // std::cout << "beta: "<<beta <<", rtt/ddl_left: "<<rtt_to_ddl_left<<", rtt: "<<rtt<<", ddl_left:"<<ddl_left<<", coeff: "<<4. * rtt_to_ddl_left * rtt_to_ddl_left<<", actual beta:" << new_beta<<"\n";
    // if(new_beta > 0.95) new_beta = round(new_beta);
    return new_beta;
};

double_t WebRTCStarPolicy::SqrtFECRate(double_t beta, uint16_t ddl_left, uint16_t rtt){
    double_t rtt_to_ddl_left = (double_t) rtt / (double_t) ddl_left;
    double_t new_beta = beta *  sqrt(2. * rtt_to_ddl_left);
    // std::cout << "beta: "<<beta <<", rtt/ddl_left: "<<rtt_to_ddl_left<<", rtt: "<<rtt<<", ddl_left:"<<ddl_left<<", coeff: "<<sqrt(2. * rtt_to_ddl_left)<<", actual beta:" << new_beta<<"\n";
    // if(new_beta > 0.95) new_beta = round(new_beta);
    return new_beta;
};

FECPolicy::FECParam WebRTCStarPolicy::GetFECParam(
        Ptr<NetStat> statistic, uint32_t bitrate,
        uint16_t ddl, uint16_t ddl_left,
        bool is_rtx, uint8_t frame_size,
        uint16_t max_group_size, bool fix_group_size
    ) {
    double_t fec;
    double_t loss = statistic->current_loss_rate;
    uint16_t rtt = statistic->current_rtt.GetMilliSeconds();
    if(this->fixed) {
        loss = this->fixed_loss_rate;
        rtt = this->fixed_rtt;
    }
    if(max_group_size > 48)
        max_group_size = 48;
    get_fec_rate_webrtc(loss, max_group_size, ((double_t) bitrate) / 1000, &fec);

    fec = MIN(fec, 1);

    // std::cout
    //      << "[WebrtcPolicy] RTT: " << statistic->current_rtt
    //      << "ms, Loss rate: " << statistic->current_loss_rate
    //      << ", group delay: " << statistic->one_way_dispersion
    //      << ", max group size: " << max_group_size
    //      << ", beta0: " << fec
    //      << '\n';
    if(this->order == 1){
        double_t fec_rate = LinearFECRate(fec, ddl_left, rtt);
        return FECParam(max_group_size, fec_rate);
    }
    else if (this->order == 2)
    {
        double_t fec_rate = QuadraticFECRate(fec, ddl_left, rtt);
        return FECParam(max_group_size, fec_rate);
    }
    else if (this->order == 0){
        double_t fec_rate = SqrtFECRate(fec, ddl_left, rtt);
        return FECParam(max_group_size, fec_rate);
    }
    else{
        NS_LOG_ERROR("Does not support higher-than-2-order Deadline-aware Multiplier, fallback to the original WebRTC");
        return FECParam(max_group_size, fec);
    }

};

void WebRTCStarPolicy::SetOrder(int order){
    this->order = order;
}

}; // namespace ns3