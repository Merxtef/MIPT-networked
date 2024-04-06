#include <enet/enet.h>
#include "utils.h"

void send_reliable(ENetPeer* peer, enet_uint8 channelID, const char* msg) {
    ENetPacket* packet = enet_packet_create(msg, strlen(msg) + 1, ENET_PACKET_FLAG_RELIABLE);

    enet_peer_send(peer, channelID, packet);
}

void send_unreliable(ENetPeer* peer, enet_uint8 channelID, const char* msg) {
    ENetPacket* packet = enet_packet_create(msg, strlen(msg) + 1, ENET_PACKET_FLAG_UNSEQUENCED);

    enet_peer_send(peer, channelID, packet);
}
