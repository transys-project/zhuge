#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/nstime.h"
#include "ns3/bitrate-ctrl-module.h"
#include "ns3/zhuge-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/stats-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-address.h"
#include "ns3/on-off-helper.h"
#include "ns3/traffic-control-module.h"

#include <memory>
#include <fstream>
#include <ctime>
#include <sstream>
#include <boost/filesystem.hpp>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("FECEmulator");

const uint32_t TOPO_DEFAULT_BW     = 300;    // in Mbps: 300 Mbps
const double   TOPO_MIN_BW         = 1;      // in Mbps: 1 Mbps
const uint32_t TOPO_DEFAULT_PDELAY = 25;     // in ms:   RTT: 20ms
const uint32_t TOPO_DEFAULT_QSIZE  = 375000; // in byte:  250 MTU-size packets according to ABC [NSDI'20]
const uint32_t DEFAULT_PACKET_SIZE = 1000;
const double   DEFAULT_ERROR_RATE  = 0.00;


const double   BITRATE_MBPS     = TOPO_MIN_BW / 2.;  // in Mbps: 30 Mbps
const uint16_t DELAY_DDL_MS     = 1000;

const uint32_t DEFAULT_RECEIVER_WINDOW = 34; // in ms

const float    EMULATION_DURATION = 300.; // in s

int set_count = 0;

std::string DEFAULT_TRACE = "traces/20mbps.log";

Time delayedDdl = Seconds (1);
Time startTime = Seconds (1);

std::string ccaMap[16] = {"Gcc", "Nada", "", "", 
  "Cubic", "Bbr", "Copa", "LinuxReno", 
  "", "", "", "", 
  "", "", "", "Fixed"};

// Network topology
//
// [GameServer]        [Error] [Rate Limit]  [GameClient]
//      ⬇                  ⬇  ⬇                 ⬇
//      n0 ----------------- n1 ----------------- n2
//   10.1.1.1         10.1.1.2/10.1.2.2         10.1.2.1


void BuildWiredTopo (uint64_t bps, double_t msDelay, int qdisc, bool isPcapEnabled, std::string dir,
  NodeContainer &senders, NodeContainer &receivers, NodeContainer &routers)
{
  NS_LOG_INFO ("Create nodes.");
  senders.Create (1);
  receivers.Create (1);
  routers.Create (1);
  NodeContainer n0n1 = NodeContainer (senders.Get (0), routers.Get (0));
  NodeContainer n2n1 = NodeContainer (receivers.Get (0), routers.Get (0));

  InternetStackHelper internet;
  internet.Install (senders);
  internet.Install (receivers);
  internet.Install (routers);

  NS_LOG_INFO ("Create channels.");
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (1e9/* Far beyond max bw */)));
  p2p.SetChannelAttribute ("Delay", StringValue ("9ms"));
  NetDeviceContainer d0d1 = p2p.Install (n0n1);

  // PointToPointHelper p2p2;
  p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (bps)));
  p2p.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (msDelay)));
  NetDeviceContainer d2d1 = p2p.Install (n2n1);

  TrafficControlHelper tch;
  if (qdisc == 1) {
    // qdisc == 1: fq_codel
    tch.SetRootQueueDisc ("ns3::FqCoDelQueueDisc");
  }
  else if (qdisc == 0) {
    // qdisc == 0: pfifo_fast
    tch.SetRootQueueDisc ("ns3::PfifoFastQueueDisc", "MaxSize", StringValue ("250p"));
  }
  else {
    NS_ABORT_MSG ("Unknown qdisc type." << qdisc);
  }
  tch.Install (d0d1);
  tch.Install (d2d1);

  NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  ipv4.Assign (d0d1);
  ipv4.SetBase ("10.1.2.0", "255.255.255.0");
  ipv4.Assign (d2d1);
  NS_LOG_INFO ("Use global routing.");
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  if (isPcapEnabled) {
    // p2p.EnablePcapAll (dir + "/pcap");
    AsciiTraceHelper ascii;
    p2p.EnableAsciiAll (ascii.CreateFileStream (dir + "/pcap.tr"));
  }
}

static void InstallApp (NodeContainer senders, NodeContainer receivers, Ptr<Node> router,
  float stopTime, Ptr<FECPolicy> fecPolicy, uint16_t interval,
  uint32_t queues, uint32_t ccas, uint32_t apps,
  Time competeTime, uint32_t flowNum, int mode,
  int qsize, bool fastAckRtx, std::string dir)
{
  AsciiTraceHelper ascii;
  std::string bwTrFileName = dir + "/bw.tr";
  std::string appTrFileName = dir + "/app.tr";
  std::string debugTrFileName = dir + "/debug.tr";
  Ptr<OutputStreamWrapper> bwStream = ascii.CreateFileStream (bwTrFileName);
  Ptr<OutputStreamWrapper> appStream = ascii.CreateFileStream (appTrFileName);
  Ptr<OutputStreamWrapper> debugStream = ascii.CreateFileStream (debugTrFileName);

  // Install applications
  uint32_t portBase = 10000;
  Ipv4Header::DscpType dscp = Ipv4Header::DscpDefault;
  uint32_t ccaOption, queueOption, appOption;
  for (uint32_t flowIndex = 0; flowIndex < flowNum; flowIndex++) {
    ccaOption = ccas & 0x0000000F;
    queueOption = queues & 0x0000000F;
    if (flowIndex == 0) {
      appOption = apps & 0x0000000F;
      startTime = Seconds (1);
    }
    else {
      appOption = apps & 0x000000F0;
      startTime = competeTime;
    }
    bool tcpEnabled = ccaOption & 0xFC;
    Ptr<Node> sender = senders.Get (flowIndex % senders.GetN ());
    Ptr<Node> receiver = receivers.Get (flowIndex % receivers.GetN ());
    Ipv4Address senderIp = sender->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ();
    Ipv4Address receiverIp = receiver->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ();
    uint32_t port = portBase + flowIndex;

    /* Install the server and client on the sender and receiver */
    if (appOption == 0) {
      Ptr<GameServer> sendApp = CreateObject<GameServer> ();
      Ptr<GameClient> recvApp = CreateObject<GameClient> ();
      sender->AddApplication (sendApp);
      receiver->AddApplication (recvApp);

      sendApp->Setup (senderIp, port, receiverIp, port, delayedDdl, interval,
        fecPolicy, tcpEnabled, dscp, appStream, bwStream);
      recvApp->Setup (senderIp, port, port, interval, delayedDdl, tcpEnabled);

      Simulator::Schedule (Seconds(stopTime), &GameServer::StopEncoding, sendApp);

      sendApp->SetController (ccaMap[ccaOption]);

      sendApp->SetStartTime (startTime);
      sendApp->SetStopTime (Seconds (stopTime + 1));
      recvApp->SetStartTime (startTime);
      recvApp->SetStopTime (Seconds (stopTime + 1));
    }
    else if (appOption == 1) {
      BulkSendHelper source ("ns3::TcpSocketFactory", InetSocketAddress (receiverIp, port));
      source.SetAttribute ("MaxBytes", UintegerValue (0));
      source.SetAttribute ("SendSize", UintegerValue (1448));
      ApplicationContainer sendApp = source.Install (sender);
      sendApp.Start (startTime);
      sendApp.Stop (Seconds (stopTime + 1));

      PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
      ApplicationContainer recvApp = sink.Install (receiver);
      recvApp.Start (startTime);
      recvApp.Stop (Seconds (stopTime + 1));
    }

    /* Install the queue management on the router */
    if (queueOption == 1) {
      Ptr<NetDevice> routerRcvDev = router->GetDevice (1);
      Ptr<NetDevice> routerSndDev = router->GetDevice (2);
      Ptr<NetworkQueueZhuge> queueApp = CreateObject<NetworkQueueZhuge> ();
      queueApp->Setup(senderIp, port, receiverIp, port, true, interval, tcpEnabled, mode, qsize, routerSndDev, routerRcvDev, fastAckRtx);
      router->AddApplication (queueApp);
      queueApp->SetStartTime (startTime);
      queueApp->SetStopTime (Seconds(stopTime + 1));
    }
    else if (queueOption == 2) {
      Ptr<NetworkQueueAbc> queueApp = CreateObject<NetworkQueueAbc> ();
      queueApp->Setup(senderIp, port, receiverIp, port, interval, tcpEnabled, mode);
      router->AddApplication (queueApp);
      queueApp->SetStartTime(startTime);
      queueApp->SetStopTime(Seconds(stopTime + 1));
    }
    else if (queueOption == 3) {
      Ptr<NetworkQueueFastAck> queueApp = CreateObject<NetworkQueueFastAck> ();
      queueApp->Setup(senderIp, port, receiverIp, port, tcpEnabled, mode);
      router->AddApplication (queueApp);
      queueApp->SetStartTime(startTime);
      queueApp->SetStopTime(Seconds(stopTime + 1));      
    }
    else if (queueOption == 9) {
    }
    else {
      NS_ABORT_MSG ("Unknown queue option " << queueOption);
    }

    if ((ccas & 0xFFFFFFF0) != 0) {
      ccas >>= 4;
    }

    if ((queues & 0xFFFFFFF0) != 0) {
      queues >>= 4;
    }
  }
}

Time oldDelay, newDelay;
Time remainInterval;
std::streampos pos = std::ios::beg;
Time minSetInterval = MicroSeconds (13);
Time maxAllowedDiff = MicroSeconds (11);
uint16_t BW_CHANGE = 0b1;
uint16_t DELAY_CHANGE = 0b10;
uint16_t LOSS_CHANGE = 0b100;

void BandwidthTrace (Ptr<Node> node0, 
                     Ptr<Node> node1, 
                     std::string trace, 
                     uint16_t change, 
                     int interval,
                     bool readNewLine) {
  Ptr<PointToPointNetDevice> n0SndDev = StaticCast<PointToPointNetDevice,NetDevice> (node0->GetDevice (1));
  Ptr<PointToPointNetDevice> n1RcvDev = StaticCast<PointToPointNetDevice,NetDevice> (node1->GetDevice (1));
  Ptr<PointToPointNetDevice> n1SndDev = StaticCast<PointToPointNetDevice,NetDevice> (node1->GetDevice (2));
  std::ifstream traceFile;
  std::string newBwStr = "300Mbps";
  double newErrorRate = DEFAULT_ERROR_RATE;
  NS_ASSERT (maxAllowedDiff <= minSetInterval);

  bool bwChange = (change & BW_CHANGE) == BW_CHANGE;
  bool delayChange = (change & DELAY_CHANGE) == DELAY_CHANGE;
  bool lossChange = (change & LOSS_CHANGE) == LOSS_CHANGE;

  if (readNewLine) {
    std::string traceLine;
    std::vector<std::string> traceData;
    std::vector<std::string> bwValue;
    std::vector<std::string> rttValue;

    traceLine.clear ();
    traceFile.open (trace);
    traceFile.seekg (pos);
    std::getline (traceFile, traceLine);
    pos = traceFile.tellg ();
    if (traceLine.find (' ') == std::string::npos) {
      traceFile.close ();
      return;
    }
    traceData.clear ();
    SplitString (traceLine, traceData," ");

    if (bwChange) {
      bwValue.clear ();
      SplitString (traceData[0], bwValue, "Mbps");
      newBwStr = std::to_string (std::stod (bwValue[0]) / 13. * 15.) + "Mbps";
      n1SndDev->SetAttribute ("DataRate", StringValue (newBwStr));
    }

    if (delayChange) {
      rttValue.clear ();
      SplitString (traceData[1], rttValue, "ms");
      /* Set delay of n0-n1 as rtt/2 - 1, the delay of n1-n2 is 1ms */ 
      newDelay = MilliSeconds (std::stod (rttValue[0]) / 2. - 1);
    }

    if (lossChange) {
      newErrorRate = std::stod (traceData[2]);
      ObjectFactory factoryErrModel;
      factoryErrModel.SetTypeId ("ns3::RateErrorModel");
      factoryErrModel.Set ("ErrorUnit", EnumValue (RateErrorModel::ERROR_UNIT_PACKET),
                           "ErrorRate", DoubleValue (newErrorRate));
      Ptr<ErrorModel> em = factoryErrModel.Create<ErrorModel> ();
      n1RcvDev->SetAttribute ("ReceiveErrorModel", PointerValue (em));
    }
    
    NS_LOG_ERROR (Simulator::Now ().GetMilliSeconds () << 
      " delay " << newDelay << " bw " << newBwStr << " errorRate " << newErrorRate);
    remainInterval = MilliSeconds (interval);
  }

  if (delayChange) {
    /* Set propagation delay, smoothsize to decrease 0.3ms every 0.33ms to avoid out of order
      These values are calculated based on 30Mbps and 1500B MTU --> 0.4ms / pkt */
    Ptr<Channel> channel = n0SndDev->GetChannel ();
    bool smoothDecrease = newDelay < oldDelay - maxAllowedDiff ? 1 : 0;
    oldDelay = smoothDecrease ? (oldDelay - maxAllowedDiff) : newDelay;
    channel->SetAttribute ("Delay", TimeValue (oldDelay));
    readNewLine = smoothDecrease && remainInterval > minSetInterval;
    remainInterval -= minSetInterval;
  }
  Simulator::Schedule (readNewLine ? remainInterval : minSetInterval, &BandwidthTrace, node0, node1, trace, 
    change, interval, readNewLine);
}

void BuildTcpStaticRoute(NodeContainer c, bool wifiSameAp, int node_number){
  if(wifiSameAp == 0){
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4> ipv4A = c.Get(0)->GetObject<Ipv4> ();
    Ptr<Ipv4> ipv4B = c.Get(1)->GetObject<Ipv4> ();
    Ptr<Ipv4> ipv4C = c.Get(2)->GetObject<Ipv4> ();
  
    Ptr<Ipv4StaticRouting> staticRoutingA = ipv4RoutingHelper.GetStaticRouting (ipv4A);
    staticRoutingA->AddHostRouteTo (Ipv4Address ("10.1.51.1"), Ipv4Address ("10.1.1.2"), 1);
    Ptr<Ipv4StaticRouting> staticRoutingB = ipv4RoutingHelper.GetStaticRouting (ipv4B);
    staticRoutingB->AddHostRouteTo (Ipv4Address ("10.1.1.1"), Ipv4Address ("10.1.1.1"), 0);
    Ptr<Ipv4StaticRouting> staticRoutingC = ipv4RoutingHelper.GetStaticRouting (ipv4C);
    staticRoutingC->AddHostRouteTo (Ipv4Address ("10.1.1.1"), Ipv4Address ("10.1.51.2"), 1);

    for(int i = 1; i < node_number; i ++){
        Ptr<Ipv4> ipv41 = c.Get(3*i)->GetObject<Ipv4> ();
        Ptr<Ipv4> ipv43 = c.Get(3*i+2)->GetObject<Ipv4> ();
        uint32_t adrbase1 = 0x0A010100;
        uint32_t adrbase2 = 0x0A013300;
        uint32_t adr1 = (i << 8) + adrbase1 + 2;
        uint32_t adr2 = (i << 8) + adrbase1 + 1;
        uint32_t adr3 = adrbase2 + 1 + 2 * i;
        uint32_t adr4 = adrbase2 + 2 + 2 * i;  
        Ptr<Ipv4StaticRouting> staticRouting1 = ipv4RoutingHelper.GetStaticRouting (ipv41);
        staticRouting1->AddHostRouteTo (Ipv4Address (adr3), Ipv4Address (adr1), 1);
        Ptr<Ipv4StaticRouting> staticRouting3 = ipv4RoutingHelper.GetStaticRouting (ipv43);
        staticRouting3->AddHostRouteTo (Ipv4Address (adr2), Ipv4Address (adr4), 1);  
    }   
  }
  if(wifiSameAp ==1){
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4> ipv4A = c.Get(0)->GetObject<Ipv4> ();
    Ptr<Ipv4> ipv4B = c.Get(1)->GetObject<Ipv4> ();
    Ptr<Ipv4> ipv4C = c.Get(2)->GetObject<Ipv4> ();
  
    Ptr<Ipv4StaticRouting> staticRoutingA = ipv4RoutingHelper.GetStaticRouting (ipv4A);
    staticRoutingA->AddHostRouteTo (Ipv4Address ("10.1.51.2"), Ipv4Address ("10.1.1.2"), 1);
    Ptr<Ipv4StaticRouting> staticRoutingB = ipv4RoutingHelper.GetStaticRouting (ipv4B);
    staticRoutingB->AddHostRouteTo (Ipv4Address ("10.1.1.1"), Ipv4Address ("10.1.1.1"), 0);
    Ptr<Ipv4StaticRouting> staticRoutingC = ipv4RoutingHelper.GetStaticRouting (ipv4C);
    staticRoutingC->AddHostRouteTo (Ipv4Address ("10.1.1.1"), Ipv4Address ("10.1.51.1"), 1);

    for(int i = 1; i < node_number; i ++){
        Ptr<Ipv4> ipv41 = c.Get(3*i)->GetObject<Ipv4> ();
        Ptr<Ipv4> ipv43 = c.Get(3*i+2)->GetObject<Ipv4> ();
        uint32_t adrbase1 = 0x0A010100;
        uint32_t adrbase2 = 0x0A013300;
        uint32_t adr1 = (i << 8) + adrbase1 + 2;
        uint32_t adr2 = (i << 8) + adrbase1 + 1;
        uint32_t adr3 = adrbase2 + 2 + i;
        uint32_t adr4 = adrbase2 + 1;  
        Ptr<Ipv4StaticRouting> staticRouting1 = ipv4RoutingHelper.GetStaticRouting (ipv41);
        staticRouting1->AddHostRouteTo (Ipv4Address (adr3), Ipv4Address (adr1), 1);
        Ptr<Ipv4StaticRouting> staticRouting3 = ipv4RoutingHelper.GetStaticRouting (ipv43);
        staticRouting3->AddHostRouteTo (Ipv4Address (adr2), Ipv4Address (adr4), 1);  
    } 
  }
}

void BuildWifiTopo (double_t msDelay, uint32_t byteQSize, int wifiSameAp, 
  int nodeNumber, int qdisc,
  NodeContainer &senders, NodeContainer &receivers, NodeContainer &routers)
{
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (1e9/* Far beyond max bw */)));
  p2p.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (msDelay)));
 
  NetDeviceContainer d0d1[nodeNumber];
  if (wifiSameAp == 0) {
    senders.Create (nodeNumber);
    receivers.Create (nodeNumber);
    routers.Create (nodeNumber);
    for (int i = 0; i < nodeNumber; i++){
      d0d1[i] = p2p.Install (NodeContainer (senders.Get (i), routers.Get (i)));
    }
  }
  else if (wifiSameAp == 1) {
    senders.Create (1);
    receivers.Create (nodeNumber);
    routers.Create (1);
    for (int i = 0; i < nodeNumber; i++){
      d0d1[i] = p2p.Install (NodeContainer (senders.Get (0), routers.Get (0)));
    }
  }

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy;
  phy.SetChannel (channel.Create());
  phy.Set ("ChannelNumber", UintegerValue (1) );  

  WifiHelper wifi;
  wifi.SetStandard (WIFI_STANDARD_80211n_2_4GHZ);
  wifi.SetRemoteStationManager ("ns3::IdealWifiManager");
  
  NetDeviceContainer staDevices, apDevices;
  if (wifiSameAp == 1) {
    WifiMacHelper mac;
    for (int i = 0; i < nodeNumber; i++) {
      mac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (Ssid ("wifi-default")),
                   "QosSupported", BooleanValue (true),
                   "ActiveProbing", BooleanValue (false));
      staDevices.Add (wifi.Install (phy, mac, receivers.Get (i)));
    }     
    mac.SetType ("ns3::ApWifiMac",
              "Ssid", SsidValue (Ssid ("wifi-default")),
              "QosSupported", BooleanValue (true));
    apDevices.Add (wifi.Install (phy, mac, routers.Get (0)));    
  }
  else if (wifiSameAp == 0) {
    WifiMacHelper mac;
    for (int i = 0; i < nodeNumber; i++) {
      mac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (Ssid (std::to_string (i))),
                   "QosSupported", BooleanValue (true),
                   "ActiveProbing", BooleanValue (false));
      staDevices.Add (wifi.Install (phy, mac, receivers.Get (i)));
  
      mac.SetType ("ns3::ApWifiMac",
              "Ssid", SsidValue (Ssid (std::to_string (i))),
              "QosSupported", BooleanValue (true));
      apDevices.Add (wifi.Install (phy, mac, routers.Get (i)));
    }  
  }
  else {
    NS_FATAL_ERROR ("wifiSameAp must be 0 or 1");
  }

  MobilityHelper mobility;
  // Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  // for(int i=0;i<node_number;i++){
  //   positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  //   positionAlloc->Add (Vector (0.0, 1, 0.0));
  // }
  for (int i = 0 ; i < nodeNumber; i++) {
      mobility.Install (routers.Get (i));
      mobility.Install (receivers.Get (i));
  }

  NS_LOG_INFO ("set internet stack");
  InternetStackHelper stack;  
  stack.Install (senders); 
  stack.Install (receivers);
  stack.Install (routers);

  TrafficControlHelper tch;
  if (qdisc) {
    // qdisc == 1: fq_codel
    tch.SetRootQueueDisc ("ns3::FqCoDelQueueDisc");
  }
  else {
    // qdisc == 0: pfifo_fast
    tch.SetRootQueueDisc ("ns3::PfifoFastQueueDisc", "MaxSize", StringValue ("250p"));
  }
  for(int i = 0; i < nodeNumber; i++){
    tch.Install (d0d1[i]);
    tch.Install (apDevices.Get (i));
  }
  
  Ipv4AddressHelper ipv4;
  Ipv4InterfaceContainer p2pInterfaces[nodeNumber];
  uint32_t addr = 0x0A010100;
  for (int i = 0; i < nodeNumber; i++) {
    ipv4.SetBase(Ipv4Address((i << 8) + addr),"255.255.255.0");
    p2pInterfaces[i] = ipv4.Assign(d0d1[i]);
  }
  ipv4.SetBase("10.1.51.0","255.255.255.0");
  if (wifiSameAp == 1) {
    ipv4.Assign (apDevices.Get (0));
    for (int i = 0; i < nodeNumber; i++) {
      ipv4.Assign (staDevices.Get (i));
    }
  }
  else if (wifiSameAp == 0) {
    for (int i = 0; i < nodeNumber; i++) {
      ipv4.Assign (staDevices.Get (i));
      ipv4.Assign (apDevices.Get (i));
    }
  }
  
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  // AsciiTraceHelper ascii;
  // p2p.EnableAsciiAll (ascii.CreateFileStream (dir + "fec-emulation-"+std::to_string(wifiSameAp)+std::to_string(queue)+".tr"));
  // phy.EnableAsciiAll (ascii.CreateFileStream (dir + "third-"+std::to_string(wifiSameAp)+std::to_string(queue)+".tr"));
  // p2p.EnablePcapAll (dir + "fec-emulation", true);
  // phy.EnablePcapAll (dir+"third", true);

  // wifiPhy_c.EnableAsciiAll (ascii.CreateFileStream (dir + "other.tr"));
  // wifiPhy_c.EnablePcap (dir+"other", apD_c.Get (0));
}

static void EnableLogging (LogLevel logLevel) {
  LogComponentEnable ("NetworkQueueZhuge", logLevel);
}

int
main (int argc, char *argv[])
{

  // arguments
  uint16_t delayDdl       = DELAY_DDL_MS;
  double_t lossRate       = DEFAULT_ERROR_RATE;
  uint64_t linkBw         = TOPO_DEFAULT_BW;
  double_t msDelay        = TOPO_DEFAULT_PDELAY;
  uint32_t byteQSize      = TOPO_DEFAULT_QSIZE;
  float appStart          = 2.;
  float duration          = EMULATION_DURATION;  // in s
  uint32_t queues         = 0;
  uint32_t ccas           = 4;
  uint32_t apps           = 0;
  int wifiSameAp          = 0;
  bool network_variation  = true;
  int flowNum             = 1;
  int qdisc               = 0;
  int mode                = 1;
  int qsize               = 2;
  int competeTime         = 20;
  int copaDeltaHundred    = 50;
  bool fastrtx            = false;
  bool isPcapEnabled      = false;
  uint16_t traceInterval  = 200;
  uint16_t frameInterval  = 40;
  uint32_t receiver_wnd   = DEFAULT_RECEIVER_WINDOW;

  std::string trace = DEFAULT_TRACE;

  CommandLine cmd;
  cmd.AddValue("ddl",      "Frame deadline, in ms", delayDdl);
  cmd.AddValue("duration", "Duration of the emulation in seconds", duration);

  cmd.AddValue("queues", "queue management bitmap", queues);
  cmd.AddValue("ccas", "Congestion control algorithm bitmap", ccas);
  cmd.AddValue("vary", "Network varies according to traces", network_variation);
  cmd.AddValue("trace", "Trace file directory", trace);
  cmd.AddValue("traceInterval", "Network condition change interval, in ms", traceInterval);
  cmd.AddValue("frameInterval", "The interval between sending two frames, in ms", frameInterval);
  cmd.AddValue("window", "Receiver sliding window size, in ms", receiver_wnd);
  cmd.AddValue("wifiSameAp", "whether through the same ap", wifiSameAp);
  cmd.AddValue("node_number", "how many groups of server-ap-sta", flowNum);
  cmd.AddValue("qdisc", "QueueDisc, FqCodel (1, default) or PfifoFast (0)", qdisc);
  cmd.AddValue("qsize", "qsize, 1p(1) or 100p(2,default)", qsize);
  cmd.AddValue("mode", "Run Mode, p2p (1, default) or Wifi (0)", mode);
  cmd.AddValue("wifiCompeteTime", "For Wifi Mode, 60 + cut", competeTime);
  cmd.AddValue("copadelta", "delta value for copa", copaDeltaHundred);
  cmd.AddValue("fastrtx", "fastrtx", fastrtx);
  cmd.AddValue("isPcapEnabled", "Capture all the packets", isPcapEnabled);
  cmd.Parse (argc, argv);

  std::string dir = "logs/" + std::to_string (queues) + "/" + std::to_string (ccas);

  if (network_variation) {
    std::string traceDir, traceName;
    std::stringstream tracePath (trace);
    std::getline (tracePath, traceDir, '/');
    std::getline (tracePath, traceName, '/');
    dir += "/" + traceName;
  }

  boost::filesystem::create_directories (dir);

  if (network_variation) {
      std::string tmp;
      std::ifstream traceFile;
      float n = 0.;
      traceFile.open (trace, std::ios::in);
      if (traceFile.fail ()) {
        NS_FATAL_ERROR ("Trace file not found!");
      }
      else {
        while (getline (traceFile, tmp)) {
          n += 1.;
        }
        traceFile.close ();
        duration = std::min (duration, (float)(n * traceInterval /1000.));
      }
  }

  float appStop = appStart + duration;    /* emulation duration */

  EnableLogging (LOG_LEVEL_DEBUG);

  // set FEC policy
  Ptr<FECPolicy> fecPolicy = CreateObject<RTXOnlyPolicy> ();

  Config::SetDefault ("ns3::RateErrorModel::ErrorRate", DoubleValue (lossRate));
	Config::SetDefault ("ns3::RateErrorModel::ErrorUnit", StringValue ("ERROR_UNIT_PACKET"));

  Config::SetDefault ("ns3::ArpCache::PendingQueueSize", UintegerValue (100));
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1448));
  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (5 << 20)); // if (rwnd > 5M)，retransmission (RTO) will accumulate
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (5 << 20)); // over 5M packet cause metadata overflow
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpCubic"));
  // Config::SetDefault ("ns3::DropTailQueue<Packet>::MaxSize", QueueSizeValue (QueueSize ("1p")));
  Config::SetDefault ("ns3::WifiMacQueue::MaxSize", QueueSizeValue(QueueSize ("100p")));
  Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (1));
  Config::SetDefault ("ns3::TcpSocketBase::Timestamp", BooleanValue (false));
  // Config::SetDefault ("ns3::TcpCopa::ModeSwitch", BooleanValue (false));
  Config::SetDefault ("ns3::TcpCopa::DefaultLatencyfactor", DoubleValue ((double) copaDeltaHundred / 100.));

  NodeContainer senders, receivers, routers;
  if (mode == 1) {
    BuildWiredTopo (linkBw * 1e6, msDelay, qdisc, isPcapEnabled, dir, senders, receivers, routers);
    BandwidthTrace (senders.Get(0), routers.Get(0), trace, 
      BW_CHANGE & ~DELAY_CHANGE & ~LOSS_CHANGE, 
      traceInterval, true);
  }
  else if (mode == 0) {
    BuildWifiTopo(msDelay, byteQSize, wifiSameAp, flowNum, qdisc, senders, receivers, routers);
  }
  else
    NS_FATAL_ERROR ("Mode not supported!");

  InstallApp (senders, receivers, routers.Get (0), appStop, fecPolicy,
    frameInterval, queues, ccas, apps, Seconds (competeTime),
    flowNum, mode, qsize, fastrtx, dir);
  
  
  Simulator::Run ();
  Simulator::Stop(Seconds(appStop));
  Simulator::Destroy ();
}
