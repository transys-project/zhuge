#include "network-queue.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NetworkQueue");

TypeId NetworkQueue::GetTypeId()
{
    static TypeId tid = TypeId("ns3::NetworkQueue")
                            .SetParent<Application>()
                            .SetGroupName("zhuge")
                            .AddConstructor<NetworkQueue>();
    return tid;
};

NetworkQueue::NetworkQueue ()
: receiver {NULL} 
{};

NetworkQueue::~NetworkQueue () {};

}; // namespace ns3
