#pragma once

#include <enet/enet.h>
#include <cstdint>

const int LOBBY_PORT = 10887;
const int SERVER_PORT = 10888;

void send_reliable(ENetPeer* peer, enet_uint8 channelID, const char* msg);

void send_unreliable(ENetPeer* peer, enet_uint8 channelID, const char* msg);

const int MAX_PLAYERS = 8;
struct Player
{
    int id = -1;
    const char* name = nullptr;
    ENetPeer* peer = nullptr;
    uint32_t rtt = (uint32_t) -1;
};
