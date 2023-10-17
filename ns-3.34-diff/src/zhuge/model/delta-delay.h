#ifndef DELTA_DELAY_H
#define DELTA_DELAY_H

#include "ns3/simulator.h"
#include "ns3/random-variable-stream.h"
#include <deque>

namespace ns3 {

class DeltaDelay : public Object {

public:
    static TypeId GetTypeId (void);
    DeltaDelay ();
    ~DeltaDelay ();
    Time SampleDeltaDelay (Time minDeltaDelay);
    void AddDeltaDelay (Time deltaDelay);
    void Reset (void);
    void ConsumeToken (Time consume);

protected:
    void ClearAgedDeltas (void);
    void ClearAgedTokens (void);
    void ClearAgedPopTokens (void);
    void ClearAgedLastwndPkt (void);
    std::deque<std::pair<Time, Time>> m_historyDeltas;
    std::deque<std::pair<Time, Time>> m_historyTokens;
    std::deque<std::pair<Time, Time>> m_lastwndPkt;
    std::deque<std::pair<Time, Time>> m_PopTokens;
    Time m_historyLen;
    Ptr<UniformRandomVariable> m_rand;
    Time m_delayToken;
};  // class DeltaDelay

};  // namespace ns3

#endif  /* DELTA_DELAY_H */