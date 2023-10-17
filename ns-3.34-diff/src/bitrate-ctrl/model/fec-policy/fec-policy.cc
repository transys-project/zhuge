#include "fec-policy.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE("FECPolicy");

/* class FECPolicy */

FECPolicy::FECParam::FECParam(uint16_t group_size, double_t fec_rate) {
    this->fec_group_size = group_size;
    this->fec_rate = fec_rate;
};

FECPolicy::FECParam::FECParam() {};
FECPolicy::FECParam::~FECParam() {};

TypeId FECPolicy::NetStat::GetTypeId() {
    static TypeId tid = TypeId ("ns3::FECPolicy::NetStat")
        .SetParent<Object> ()
        .SetGroupName("bitrate-ctrl")
        .AddConstructor<FECPolicy::NetStat> ()
    ;
    return tid;
};

FECPolicy::NetStat::NetStat() {};
FECPolicy::NetStat::~NetStat() {};

FECPolicy::FECPolicy(Time rtx_check_intvl) : pacing_flag {false}, rtx_check_interval {rtx_check_intvl} {
};
FECPolicy::~FECPolicy() {};

TypeId FECPolicy::GetTypeId() {
    static TypeId tid = TypeId ("ns3::FECPolicy")
        .SetParent<Object> ()
        .SetGroupName("bitrate-ctrl")
    ;
    return tid;
};

bool FECPolicy::GetPacingFlag() {
    return this->pacing_flag;
};
}; // namespace ns3