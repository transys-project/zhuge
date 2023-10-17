#ifndef OTHER_POLICY_H
#define OTHER_POLICY_H

#include "fec-policy.h"
#include "ns3/webrtc-fec-array.h"
#include "ns3/fec-array.h"
#include "ns3/object.h"
#include "ns3/nstime.h"
#include <vector>
#include "math.h"

namespace ns3{

class FECOnlyPolicy : public FECPolicy {
public:
    FECOnlyPolicy();
    ~FECOnlyPolicy();
    static TypeId GetTypeId (void);
public:
    FECParam GetFECParam(
        Ptr<NetStat> statistic, uint32_t bitrate,
        uint16_t ddl, uint16_t ddl_left,
        bool is_rtx, uint8_t frame_size,
        uint16_t max_group_size, bool fix_group_size
    );
};  // class FECOnlyPolicy

class RTXOnlyPolicy : public FECPolicy {
public:
    RTXOnlyPolicy();
    ~RTXOnlyPolicy();
    static TypeId GetTypeId (void);
public:
    FECParam GetFECParam(
        Ptr<NetStat> statistic, uint32_t bitrate,
        uint16_t ddl, uint16_t ddl_left,
        bool is_rtx, uint8_t frame_size,
        uint16_t max_group_size, bool fix_group_size
    );
};  // class RTXOnlyPolicy

class WebRTCPolicy : public FECPolicy {
public:
    WebRTCPolicy();
    WebRTCPolicy(bool fixed, double_t loss_rate);
    ~WebRTCPolicy();
    static TypeId GetTypeId (void);
private:
    bool fixed;
    double_t loss_rate;
public:
    FECParam GetFECParam(
        Ptr<NetStat> statistic, uint32_t bitrate,
        uint16_t ddl, uint16_t ddl_left,
        bool is_rtx, uint8_t frame_size,
        uint16_t max_group_size, bool fix_group_size
    );
};  // class WebRTCPolicy

class WebRTCStarPolicy : public FECPolicy {
public:
    WebRTCStarPolicy();
    WebRTCStarPolicy(int order);
    WebRTCStarPolicy(bool fixed, double_t loss_rate, double_t rtt, int order);
    ~WebRTCStarPolicy();
    static TypeId GetTypeId (void);
private:
    bool fixed;
    double_t fixed_loss_rate;
    double_t fixed_rtt;
    int order;
    double_t LinearFECRate(double_t webrtc_fec_rate, uint16_t ddl_left, uint16_t rtt);
    double_t QuadraticFECRate(double_t webrtc_fec_rate, uint16_t ddl_left, uint16_t rtt);
    double_t SqrtFECRate(double_t fec_rate, uint16_t ddl_left, uint16_t rtt);
public:
    FECParam GetFECParam(
        Ptr<NetStat> statistic, uint32_t bitrate,
        uint16_t ddl, uint16_t ddl_left,
        bool is_rtx, uint8_t frame_size,
        uint16_t max_group_size, bool fix_group_size
    );
    void SetOrder(int order);
}; // class WebRTCStarPolicy

}; // namespace ns3

#endif /* OTHER_POLICY_H */