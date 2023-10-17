#include <limits>
#include <stdexcept>
#include <iostream>
#include "tcp-copa.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
namespace ns3 {
  
NS_LOG_COMPONENT_DEFINE ("TcpCopa");
NS_OBJECT_ENSURE_REGISTERED (TcpCopa);
namespace {
  uint32_t kMinCwndSegment = 4;
  Time kRTTWindowLength = MilliSeconds(10000);
  Time kSrttWindowLength = MilliSeconds(100);
  uint32_t AddAndCheckOverflow(uint32_t value,const uint32_t toAdd,uint32_t label){
    if (std::numeric_limits<uint32_t>::max() - toAdd < value) {
      // TODO: the error code is CWND_OVERFLOW but this function can totally be
      // used for inflight bytes.
      NS_LOG_DEBUG (value << " " << toAdd << " " << label);
      NS_ASSERT_MSG(0,"Overflow bytes in flight");
    }
    value += (toAdd);
    return value;
  }
  template <class T1>
  void subtractAndCheckUnderflow(T1& value, const T1& toSub) {
    if (value < toSub) {
      // TODO: wrong error code
      throw std::runtime_error("Underflow bytes in flight");
    }
    value -=(toSub);
    return value;
  }
}

TypeId TcpCopa::GetTypeId (void){
  static TypeId tid = TypeId ("ns3::TcpCopa")
  .SetParent<TcpCongestionOps> ()
  .AddConstructor<TcpCopa> ()
  .SetGroupName ("Internet")
  .AddAttribute ("UseRttStanding",
           "True to use rtt standing",
           BooleanValue (false),
           MakeBooleanAccessor (&TcpCopa::m_useRttStanding),
           MakeBooleanChecker ())
  .AddAttribute ("Latencyfactor",
           "Value of latency factor",
           DoubleValue (0.5),
           MakeDoubleAccessor (&TcpCopa::m_deltaParam),
           MakeDoubleChecker<double> ())
  .AddAttribute ("ModeSwitch",
           "True to switch mode",
           BooleanValue (true),
           MakeBooleanAccessor (&TcpCopa::m_copaModeSwitch),
           MakeBooleanChecker ()) 
  .AddAttribute ("DefaultLatencyfactor",
           "Value of Default latency factor",
           DoubleValue (0.5),
           MakeDoubleAccessor (&TcpCopa::kCopaDefaultDelta),
           MakeDoubleChecker<double> ())
  ;
  return tid;
}

TcpCopa::TcpCopa ()
: TcpCongestionOps ()
, m_minRttFilter (kRTTWindowLength.GetMicroSeconds(), Time(0), 0)
, m_standingRttFilter (kSrttWindowLength.GetMicroSeconds(), Time(0), 0) 
, m_curNumAcked {0}
, m_curNumLosses {0}
, m_reportAckedBytes {0}
, m_reportLostBytes {0}
, m_prevLossCycle {Seconds(0)}
, m_prevLossRate {0.}
, kReportIntervalRtt {0.5}
, m_lastReportTime {Seconds(0)}
{
  kCopaDefaultDelta = 0.5;
  m_copaCurMode = Default;
  copa_num_increase = 0;
  m_deltaParam = 0.5;
}

TcpCopa::TcpCopa (const TcpCopa &sock)
: TcpCongestionOps (sock)
, m_minRttFilter (kRTTWindowLength.GetMicroSeconds(), Time(0), 0)
, m_standingRttFilter (kSrttWindowLength.GetMicroSeconds(), Time(0), 0)
, m_useRttStanding (sock.m_useRttStanding)
, m_isSlowStart (sock.m_isSlowStart)
, m_deltaParam (sock.m_deltaParam) 
, m_curNumAcked {0}
, m_curNumLosses {0}
, m_reportAckedBytes {0}
, m_reportLostBytes {0}
, m_prevLossCycle {Seconds(0)}
, m_prevLossRate {0.}
, kReportIntervalRtt {0.5}
, m_lastReportTime {Seconds(0)}
{
  m_lastCwndDoubleTime = Time(0);
  kCopaDefaultDelta = 0.5;
  m_copaCurMode = Default;
  copa_num_increase = 0;
  m_deltaParam = 0.5;
}

TcpCopa::~TcpCopa() {}

std::string TcpCopa::GetName () const{
  return "TcpCopa";
}

void TcpCopa::Init (Ptr<TcpSocketState> tcb){
  if (!tcb->m_pacing) {
    NS_LOG_WARN ("Copa must use pacing");
    tcb->m_pacing = true;
  }
  InitPacingRateFromRtt(tcb,2.0);
}

uint32_t TcpCopa::GetSsThresh (Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight) {
  return tcb->m_ssThresh;;
}

void TcpCopa::IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked) {}

void TcpCopa::PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt) {}

void TcpCopa::CongestionStateSet (Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCongState_t newState) {}

void TcpCopa::CwndEvent (Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCAEvent_t event) {}

bool TcpCopa::HasCongControl () const { return true; }

void TcpCopa::CongControl (Ptr<TcpSocketState> tcb,const TcpRateOps::TcpRateConnection &rc,
              const TcpRateOps::TcpRateSample &rs) {
  // CongControl is called per packet, thus store the states and check the update period
  
  NS_LOG_DEBUG ("[TcpCopa] " << Simulator::Now() << " lastRtt " << tcb->m_lastRtt); 
  Time rtt = rs.m_rtt;
  Time srtt = tcb->m_lastRtt;
  Time now = Simulator::Now();
  m_reportAckedBytes += rs.m_ackedSacked;
  m_reportLostBytes += rs.m_bytesLoss;
  m_minRttFilter.Update(rtt, now.GetMicroSeconds());
  m_standingRttFilter.SetWindowLength(srtt.GetMicroSeconds()/2);
  m_standingRttFilter.Update(rtt, now.GetMicroSeconds());
  auto rtt_min = m_minRttFilter.GetBest();
  new_rtt_sample(rtt, rtt_min, srtt, now);
  if (!(now > Seconds(0) && m_lastReportTime + kReportIntervalRtt * srtt < now)) {
    // Copa report measurement results every 0.5 * rtt
    // https://github.com/venkatarun95/ccp_copa/blob/30a8ecd4b3f0ee79f5e19aedaa2b4c94949c2f2b/src/agg_measurement.rs#L90
    return;
  }
  NS_LOG_DEBUG ("[TcpCopa] " << now << " cur cwnd " << tcb->m_cWnd);
  NS_ASSERT (Time(0) != rtt_min);
  Time rttStanding = m_standingRttFilter.GetBest();
  NS_ASSERT (Time(0) != rttStanding);
  if (rttStanding < rtt_min) {
    return;
  }

  NS_LOG_DEBUG ("[TcpCopa] " << now << 
    " rtt_min " << rtt_min << " rtt_standing " << rttStanding << 
    " acked " << m_reportAckedBytes << " lost " << m_reportLostBytes <<
    " lastreport time " << m_lastReportTime);
  // Every interval*rtt, new measurement reported
  ReportMeasurement(srtt, rtt_min, m_reportAckedBytes, m_reportLostBytes);

  uint64_t delay_us;
  uint32_t mss = tcb->m_segmentSize;
  uint32_t cwnd_bytes = tcb->m_cWnd;
  uint32_t acked_packets = (m_reportAckedBytes + mss - 1) / mss;
  if (m_useRttStanding) {
    delay_us = rttStanding.GetMicroSeconds() - rtt_min.GetMicroSeconds();
  }
  else {
    delay_us = rtt.GetMicroSeconds() - rtt_min.GetMicroSeconds();
  }
  NS_LOG_DEBUG ("[tcp copa] time now " << Simulator::Now() <<
    " srtt= " << srtt << " min rtt=" << rtt_min << 
    " rtt standing=" << rttStanding << " delay=" << delay_us);
  
  // https://github.com/venkatarun95/ccp_copa/blob/30a8ecd4b3f0ee79f5e19aedaa2b4c94949c2f2b/src/lib.rs#L66
  bool increase_cwnd;
  if (0 == delay_us) {
    increase_cwnd = true;
  }
  else {
    double target_rate = 1.0 * mss * 1000000 / (delay_us * m_deltaParam);
    double current_rate = 1.0 * cwnd_bytes * 1000000 / rttStanding.GetMicroSeconds();
    this->m_bitrate = current_rate * 8;
    increase_cwnd = target_rate >= current_rate;
    NS_LOG_DEBUG ("[tcp copa] time now " << Simulator::Now() <<
      " srtt=" << srtt << " min rtt=" << rtt_min << " rtt standing=" << rttStanding <<
      " cur rate=" << current_rate << " target rate=" << target_rate);
  }
  
  if (!(increase_cwnd && m_isSlowStart)) {
    CheckAndUpdateDirection(now, srtt, cwnd_bytes, m_reportAckedBytes);
  }

  // https://github.com/venkatarun95/ccp_copa/blob/30a8ecd4b3f0ee79f5e19aedaa2b4c94949c2f2b/src/lib.rs#L135
  uint32_t change = (acked_packets * mss * mss * m_velocityState.velocity) / (m_deltaParam * cwnd_bytes);
  change = std::min(change, cwnd_bytes);
  if (increase_cwnd) {
    if (m_isSlowStart) {
    // When a flow starts, Copa performs slow-start where
    // cwnd doubles once per RTT until current rate exceeds target rate".
      if (Time(0) == m_lastCwndDoubleTime) {
        m_lastCwndDoubleTime = now;
      }
      else if (now - m_lastCwndDoubleTime > srtt) {
        uint32_t new_cwnd = AddAndCheckOverflow(cwnd_bytes, m_reportAckedBytes, 154);
        tcb->m_cWnd = new_cwnd;
        m_lastCwndDoubleTime = now;
      }
    } 
    else {
      if (m_velocityState.direction != VelocityState::Direction::Up
        && m_velocityState.velocity > 1){
      // if our current rate is much different than target, we double v every
      // RTT. That could result in a high v at some point in time. If we
      // detect a sudden direction change here, while v is still very high but
      // meant for opposite direction, we should reset it to 1.
        ChangeDirection(now, VelocityState::Direction::Up, cwnd_bytes);
      }
      tcb->m_cWnd = AddAndCheckOverflow(cwnd_bytes, change, 174);;
      NS_LOG_DEBUG ("[tcp copa] time now " << Simulator::Now() << 
        " addition=" << change << " new cwnd=" << tcb->m_cWnd <<
        " v=" << m_velocityState.velocity << " acks=" << acked_packets);
    }
  } 
  else {
    if (m_velocityState.direction != VelocityState::Direction::Down && m_velocityState.velocity > 1) {
      ChangeDirection(now, VelocityState::Direction::Down, cwnd_bytes);
    }
    tcb->m_cWnd = std::max<uint32_t>(cwnd_bytes - change, kMinCwndSegment * mss);
    if (m_isSlowStart) {
      tcb->m_cWnd = std::min(cwnd_bytes >> 1, (uint32_t) tcb->m_cWnd);
    }
    m_isSlowStart = false;
    NS_LOG_DEBUG ("[tcp copa] time now " << Simulator::Now() <<
      " reduction=" << change << " new cwnd=" << tcb->m_cWnd <<
      " v=" << m_velocityState.velocity << " acks=" << acked_packets);
  }
  SetPacingRate(tcb, 2.0);

  // Clear up the states of this report interval
  // https://github.com/venkatarun95/ccp_copa/blob/30a8ecd4b3f0ee79f5e19aedaa2b4c94949c2f2b/src/agg_measurement.rs#L93
  m_lastReportTime = now;
  m_reportAckedBytes = 0;
  m_reportLostBytes = 0;
  m_standingRttFilter.Reset(Seconds(1), now.GetMicroSeconds());
  tcb->m_ssThresh = tcb->m_cWnd;
}

Ptr<TcpCongestionOps> TcpCopa::Fork (){
  return CopyObject<TcpCopa> (this);
}

void TcpCopa::InitPacingRateFromRtt(Ptr<TcpSocketState> tcb,float gain){
  uint32_t cwnd_bytes = tcb->m_cWnd;
  Time rtt = tcb->m_lastRtt;
  double bps = 1000000;
  DataRate pacing_rate;
  if (rtt != Time (0)) {
    bps = 1.0 * cwnd_bytes * 8000 / rtt.GetMilliSeconds();
  }
  pacing_rate = DataRate(gain*bps);
  if (pacing_rate > tcb->m_maxPacingRate) {
    pacing_rate = tcb->m_maxPacingRate;
  }
  tcb->m_pacingRate = pacing_rate;
}
void TcpCopa::SetPacingRate(Ptr<TcpSocketState> tcb,float gain){
  InitPacingRateFromRtt(tcb,gain);
}
DataRate TcpCopa::BandwidthEstimate(Ptr<TcpSocketState> tcb){
  Time srtt = tcb->m_lastRtt;
  DataRate rate;
  double bps = 0.0;
  if(srtt != Time(0)){
    bps = 1.0 * tcb->m_cWnd * 8000 / srtt.GetMilliSeconds();
  }
  rate = DataRate (bps);
  return rate;
}
/**
 * Once per window, the sender
 * compares the current cwnd to the cwnd value at
 *  the time that the latest acknowledged packet was
 *  sent (i.e., cwnd at the start of the current window).
 *  If the current cwnd is larger, then set direction to
 *  'up'; if it is smaller, then set direction to 'down'.
 *  Now, if direction is the same as in the previous
 *  window, then double v. If not, then reset v to 1.
 *  However, start doubling v only after the direction
 *  has remained the same for three RTTs
 */
void TcpCopa::CheckAndUpdateDirection(Time event_time,Time srtt,uint32_t cwnd_bytes, uint32_t acked_bytes){
  if (Time (0) == m_velocityState.lastCwndRecordTime) {
    m_velocityState.lastCwndRecordTime = event_time;
    m_velocityState.lastRecordedCwndBytes = cwnd_bytes;
    return;
  }
  NS_LOG_DEBUG ("[tcp copa] time now " << event_time <<
    " acked bytes=" << acked_bytes << " total bytes=" << m_TotalAckedBytes << " cwnd=" << cwnd_bytes);
  NS_ASSERT (event_time >= m_velocityState.lastCwndRecordTime);
  // auto elapsed_time = event_time-m_velocityState.lastCwndRecordTime;
  // The official implementation changes v every 2 RTT: 
  // https://github.com/venkatarun95/ccp_copa/blob/30a8ecd4b3f0ee79f5e19aedaa2b4c94949c2f2b/src/lib.rs#L91
  m_TotalAckedBytes += acked_bytes;
  if (m_TotalAckedBytes >= cwnd_bytes) {
    VelocityState::Direction new_direction = cwnd_bytes > m_velocityState.lastRecordedCwndBytes
               ? VelocityState::Direction::Up
               : VelocityState::Direction::Down;
    if((new_direction == m_velocityState.direction) && (event_time - m_velocityState.time_since_direction > 3 * srtt)){
      m_velocityState.velocity = 2 * m_velocityState.velocity;
    }
    else if (new_direction != m_velocityState.direction) {
      m_velocityState.velocity = 1;
      m_velocityState.time_since_direction = event_time;
    }
    NS_LOG_DEBUG ("[tcp copa] time now " << event_time <<
      " direction=" << m_velocityState.direction << " new direction=" << new_direction << 
      " lastRecordedCwndBytes=" << m_velocityState.lastRecordedCwndBytes << " cwnd=" << cwnd_bytes <<
      " last change=" << m_velocityState.time_since_direction << " v=" << m_velocityState.velocity);
    m_velocityState.direction=new_direction;
    m_velocityState.lastCwndRecordTime=event_time;
    m_velocityState.lastRecordedCwndBytes=cwnd_bytes;
    m_TotalAckedBytes = 0;
  }
}
void TcpCopa::ChangeDirection(Time event_time, VelocityState::Direction new_direction,uint32_t cwnd_bytes){
  if(new_direction==m_velocityState.direction){
    return ;
  }
  
  m_velocityState.direction=new_direction;
  m_velocityState.velocity=1;
  m_velocityState.time_since_direction = event_time;
  m_velocityState.lastRecordedCwndBytes=cwnd_bytes;
}
double TcpCopa::GetRate(){
  return this->m_bitrate;
}

// From https://github.com/venkatarun95/ccp_copa/blob/30a8ecd4b3f0ee79f5e19aedaa2b4c94949c2f2b/src/rtt_window.rs
void TcpCopa::new_rtt_sample(Time rtt, Time rtt_min, Time srtt, Time now) {
  NS_ASSERT(copa_rtts.size() == copa_times.size());
  copa_max_time = uint64_t(Time("10s").GetMicroSeconds());

  // Push back data
  copa_rtts.push_back(uint64_t(rtt.GetMicroSeconds()));
  copa_times.push_back(uint64_t(now.GetMicroSeconds()));
  NS_LOG_DEBUG ("[tcp copa] time now " << now << " insert (" << rtt << "," << now << ") " << srtt);
  // Update min. RTT (already implemented)
  // Update srtt (already implemented)

  // Update increase (delete increase judgement, only to count)
  if (copa_increase.size() == uint32_t(0) ||
    copa_increase.back() < (uint64_t(now.GetMicroSeconds()) - 2 * uint64_t(rtt_min.GetMicroSeconds()))) {
      copa_num_increase += 1;
      copa_increase.push_back(uint64_t(now.GetMicroSeconds()));
    }

  // Delete old data
  clear_old_hist(now);
}

void TcpCopa::clear_old_hist(Time now) {
  NS_ASSERT(copa_rtts.size() == copa_times.size());

  // Delete all samples older than max_time. However, if there is only one
  // sample left, don't delete it (delete judge minrtt part)
  while ((uint64_t(now.GetMicroSeconds()) > copa_max_time) && copa_times.size() > uint32_t(1) && copa_times.front() < (uint64_t(now.GetMicroSeconds()) - copa_max_time)) {
    NS_LOG_INFO ("[TcpCopa] time now " << now << 
      " pop " << copa_rtts.front() << " " << copa_times.front());
    copa_times.pop_front();
    copa_rtts.pop_front();
  }

  // If necessary, recompute min rtt (already implemented)

  // Delete all old increase/decrease samples
  while (copa_increase.size() > uint32_t(40)) {
    copa_num_increase -= 1;
    copa_increase.pop_front();
  }
}

bool TcpCopa::tcp_detected(Time srtt, Time rtt_min) {
  if (copa_rtts.size() == uint32_t(0)) {
    return false;
  }

  uint64_t min1 = UINT64_MAX;
  uint64_t max1 = 0;

  for (uint32_t i = 0; i < copa_rtts.size(); i++) {
    if (copa_times[i] > (copa_times.back() - uint64_t(srtt.GetMicroSeconds()) * 10)) {
        min1 = std::min(min1, copa_rtts[i]);
        max1 = std::max(max1, copa_rtts[i]);
      }
  }
  uint64_t thresh = uint64_t(rtt_min.GetMicroSeconds()) + 
            (max1 - uint64_t(rtt_min.GetMicroSeconds())) / 10 + 
            uint64_t(Time("0.1ms").GetMicroSeconds());
  NS_LOG_DEBUG ("[tcp copa] tcp detect now: " << Simulator::Now() <<
    " short min rtt " << min1 << " short max rtt " << max1 << " min rtt " << rtt_min << " thresh " << thresh);
  bool res = true;
  if(min1 > thresh){res = true;}
  else{res = false;}
  return res;
}

void TcpCopa::ReportMeasurement(Time srtt, Time rtt_min, uint32_t acked, uint32_t lost) {
  m_curNumAcked += acked;
  m_curNumLosses += lost;
  Time now = Simulator::Now();
  NS_LOG_DEBUG ("[TcpCopa] " << now <<
    " srtt " << srtt << " rtt min " << rtt_min << " acked " << acked << " lost " << lost);
  if (now > m_prevLossCycle + rtt_min + rtt_min) {
    if (m_curNumLosses + m_curNumAcked > 0) {
      m_prevLossRate = (double) m_curNumLosses / ((m_curNumLosses + m_curNumAcked));
    }
    NS_LOG_DEBUG ("[TcpCopa] " << now << 
      " curNumLosses " << m_curNumLosses << " m_curNumAcked " << m_curNumAcked << 
      " m_prevLossRate " << m_prevLossRate << " prevLossCycle " << m_prevLossCycle);
    m_curNumLosses = 0;
    m_curNumAcked = 0;
    m_prevLossCycle = now;
  }

  if (m_prevLossRate >= 0.1) {
    m_copaCurMode = TCPLoss;
  }
  else if (tcp_detected(srtt, rtt_min)) {
    m_copaCurMode = TCPCoop;
  } 
  else {
    m_copaCurMode = Default;
    m_deltaParam = kCopaDefaultDelta;
  }

  if (m_copaCurMode == Default || !m_copaModeSwitch) {
    // Coming from TCPCoop mode
    m_deltaParam = kCopaDefaultDelta;
  }
  else if (m_copaCurMode == TCPCoop) {
    if (lost > 0) {
      m_deltaParam *= 2.;
    }
    else {
      m_deltaParam = 1. / (1. + 1. / m_deltaParam);
    }
    if (m_deltaParam > kCopaDefaultDelta) {
      m_deltaParam = kCopaDefaultDelta;
    }
  }
  else if (m_copaCurMode == TCPLoss) {
    if (lost > 0) {
      m_deltaParam *= 2.;
    }
    if (m_deltaParam > kCopaDefaultDelta) {
      m_deltaParam = kCopaDefaultDelta;
    }
  }
  else {
    NS_FATAL_ERROR ("[TcpCopa] Unknown copa mode");
  }
  NS_LOG_DEBUG ("[tcp copa] mode now: " << Simulator::Now() <<
    " mode " << m_copaCurMode << " delta " << m_deltaParam);
}

}