#include "delta-delay.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("DeltaDelay");

TypeId DeltaDelay::GetTypeId()
{
    static TypeId tid = TypeId("ns3::DeltaDelay")
                            .SetParent<Object>()
                            .SetGroupName("zhuge")
                            .AddConstructor<DeltaDelay>();
    return tid;
};

DeltaDelay::DeltaDelay ()
: m_historyLen {MilliSeconds(40)}
, m_delayToken {Seconds(0)}
{
    m_rand = CreateObject<UniformRandomVariable> ();
};

DeltaDelay::~DeltaDelay () {};

Time DeltaDelay::SampleDeltaDelay (Time minDeltaDelay) {
    ClearAgedDeltas();
    ClearAgedTokens();
    ClearAgedPopTokens();
    ClearAgedLastwndPkt();
    if (m_historyDeltas.empty()) {
        return minDeltaDelay;
    }
    
    Time sampledDeltaDelay = m_historyDeltas[m_rand->GetInteger(0, m_historyDeltas.size() - 1)].second;
    NS_LOG_INFO("[DeltaDelay] Time " << Simulator::Now() << " delta history ");
    for (auto it = m_historyDeltas.begin() ; it!=m_historyDeltas.end(); it++){
        NS_LOG_INFO(it->first<<" "<<it->second<<" ");
    }
    NS_LOG_INFO("\n");
    NS_LOG_INFO("[DeltaDelay] Time " << Simulator::Now() << " token history ");
    for (auto it = m_historyTokens.begin() ; it!=m_historyTokens.end(); it++){
        NS_LOG_INFO(it->first<<" "<<it->second<<" ");
    }
    NS_LOG_INFO("\n");
    Time resDeltaDelay;

    if (sampledDeltaDelay < minDeltaDelay) {
        resDeltaDelay = minDeltaDelay;
    }
    else{
        resDeltaDelay = sampledDeltaDelay;
    }

    while(!m_historyTokens.empty()) {
        Time taketoken = m_historyTokens.front().second;
        if(taketoken < resDeltaDelay){
            resDeltaDelay -= taketoken;
            m_PopTokens.push_back(std::make_pair(m_historyTokens.front().first,m_historyTokens.front().second));
            m_historyTokens.pop_front();
            NS_LOG_INFO( "[DeltaDelay] Time " << Simulator::Now() << " pop token " << taketoken);
        }
        else{
            m_historyTokens.front().second = taketoken - resDeltaDelay;
            resDeltaDelay = Time(0);
            NS_LOG_INFO( "[DeltaDelay] Time " << Simulator::Now() << " delay consumed" << m_historyTokens.front().second );
            break;
        }
    }
    
    if ( m_lastwndPkt.size() > (m_PopTokens.size() + m_historyTokens.size() + m_historyDeltas.size()) ) {
        resDeltaDelay = (double)(m_PopTokens.size() + m_historyTokens.size() + m_historyDeltas.size()) / m_lastwndPkt.size() * resDeltaDelay;
    }
    NS_LOG_INFO ( "[DeltaDelay] Time " << Simulator::Now() << " " << m_PopTokens.size() << " " << m_historyDeltas.size() << " " << m_historyTokens.size() <<" "<< m_lastwndPkt.size());
    NS_ASSERT(resDeltaDelay <= MilliSeconds(10));

    NS_LOG_INFO( "[DeltaDelay] Time " << Simulator::Now()
        << " sampledDeltaDelay " << sampledDeltaDelay
        << " resDeltaDelay " << resDeltaDelay
        << " minDeltaDelay " << minDeltaDelay);

    return resDeltaDelay;
};

void DeltaDelay::AddDeltaDelay (Time deltaDelay) {
    ClearAgedDeltas();
    ClearAgedTokens();
    ClearAgedPopTokens();
    ClearAgedLastwndPkt();
    if (deltaDelay > MicroSeconds(-1)) {
        deltaDelay = std::min(MilliSeconds(10), deltaDelay);
        m_historyDeltas.push_back(std::make_pair(Simulator::Now(), deltaDelay));
        NS_LOG_INFO( "[DeltaDelay] Time " << Simulator::Now() << " add delay " << deltaDelay );
    }
    else {
        Time deltaToken = -1 * deltaDelay;
        deltaToken = std::min(MilliSeconds(10), deltaToken);
        m_historyTokens.push_back(std::make_pair(Simulator::Now(), deltaToken));
        NS_LOG_INFO( "[DeltaDelay] Time " << Simulator::Now() << " add token " << deltaToken );
    }
};

void DeltaDelay::Reset (void) {
    m_historyDeltas.clear();
    m_historyTokens.clear();
}

void DeltaDelay::ConsumeToken (Time consume) {
    m_delayToken = std::max(m_delayToken - consume, Seconds(0));
}

void DeltaDelay::ClearAgedDeltas (void) {
    Time now = Simulator::Now();
    while (!m_historyDeltas.empty()) {
        if (now - m_historyDeltas.front().first > m_historyLen) {
            NS_LOG_INFO( "[DeltaDelay] Time " << Simulator::Now() <<" pop delta "<<m_historyDeltas.front().first <<" "<<m_historyDeltas.front().second);
            m_lastwndPkt.push_back(std::make_pair(m_historyDeltas.front().first,m_historyDeltas.front().second));
            m_historyDeltas.pop_front();
        }
        else {
            break;
        }
    }
}

void DeltaDelay::ClearAgedTokens (void) {
    Time now = Simulator::Now();
    while (!m_historyTokens.empty()) {
        if (now - m_historyTokens.front().first > m_historyLen) {
            NS_LOG_INFO("[DeltaDelay] Time " << Simulator::Now() <<" pop token "<<m_historyTokens.front().first <<" "<<m_historyTokens.front().second);
            m_lastwndPkt.push_back(std::make_pair(m_historyTokens.front().first,m_historyTokens.front().second));
            m_historyTokens.pop_front();
        }
        else {
            break;
        }
    }
}

void DeltaDelay::ClearAgedPopTokens (void) {
    Time now = Simulator::Now();
    while (!m_PopTokens.empty()) {
        if (now - m_PopTokens.front().first > m_historyLen) {
            NS_LOG_INFO("[DeltaDelay] Time " << Simulator::Now() <<" pop used token "<<m_PopTokens.front().first <<" "<<m_historyTokens.front().second);
            m_lastwndPkt.push_back(std::make_pair(m_PopTokens.front().first,m_PopTokens.front().second));
            m_PopTokens.pop_front();
        }
        else {
            break;
        }
    }
}

void DeltaDelay::ClearAgedLastwndPkt (void) {
    Time now = Simulator::Now();
    while (!m_lastwndPkt.empty()) {
        if (now - m_lastwndPkt.front().first > 2 * m_historyLen) {
            m_lastwndPkt.pop_front();
        }
        else {
            break;
        }
    }
}

}