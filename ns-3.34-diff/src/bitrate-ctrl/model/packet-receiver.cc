#include "packet-receiver.h"
#include "game-client.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("PacketReceiver");

TypeId PacketReceiver::GetTypeId() {
    static TypeId tid = TypeId ("ns3::PacketReceiver")
        .SetParent<Object> ()
        .SetGroupName("bitrate-ctrl")
    ;
    return tid;
};

PacketReceiver::PacketReceiver ()
: game_client{NULL}
, m_socket {NULL}
{};

PacketReceiver::~PacketReceiver () {};

void PacketReceiver::StartApplication(GameClient* client, Ptr<VideoDecoder> decoder, Time delay_ddl, Ptr<Node> node) {
    this->game_client = client;
    this->decoder = decoder;
    this->delay_ddl = delay_ddl;
}


void PacketReceiver::StopRunning() {
    // if(this->m_feedbackTimer.IsRunning()){
    //     this->m_feedbackTimer.Cancel();
    // }
};

}; // namespace ns3