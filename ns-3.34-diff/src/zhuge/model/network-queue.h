#ifndef NETWORK_QUEUE_BASE_H
#define NETWORK_QUEUE_BASE_H

#include "ns3/fec-policy.h"
#include "ns3/packet-receiver-udp.h"
#include "ns3/packet-receiver-tcp.h"
#include "ns3/video-decoder.h"

#include "delta-delay.h"

#include "ns3/application.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/simulator.h"
#include "ns3/assert.h"
#include "ns3/socket.h"
#include "ns3/wifi-module.h"
#include "ns3/wifi-net-device.h"
#include "ns3/pointer.h"
#include "ns3/wifi-net-device.h"
#include "ns3/minstrel-wifi-manager.h"
#include "ns3/minstrel-ht-wifi-manager.h"
#include "ns3/ap-wifi-mac.h"
#include "ns3/ampdu-subframe-header.h"
#include "ns3/mobility-model.h"
#include "ns3/log.h"
#include "ns3/radiotap-header.h"
#include "ns3/config.h"
#include "ns3/names.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/wifi-mac-queue.h"
#include "ns3/qos-utils.h"
#include "ns3/ht-configuration.h"
#include "ns3/vht-configuration.h"
#include "ns3/he-configuration.h"
#include "ns3/obss-pd-algorithm.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/llc-snap-header.h"
#include "ns3/queue-disc.h"
#include "ns3/traffic-control-layer.h"
#include "ns3/ipv4-interface.h"
#include <unordered_map>
#include <numeric>

namespace ns3 {

class NetworkQueue : public Application {

public:
    static TypeId GetTypeId (void);
    NetworkQueue();
    ~NetworkQueue();

protected:
    Ptr<WifiNetDevice> m_sndDev;
    Ptr<PointToPointNetDevice> m_rcvDev;

private:
    bool m_wirtcEnable;
    Ptr<PacketReceiverUdp> receiver;

    Ipv4Address m_sendIP;
    uint16_t m_sendPort;
    Ipv4Address m_recvIP;
    uint16_t m_recvPort;
    uint16_t c_sendPort;
    bool cc_create;
    Ptr<QosTxop> m_edca;
    int wifi_same_ap;
    int node_number;
};  // class NetworkQueue

};  // namespace ns3

#endif  /* NETWORK_QUEUE_H */