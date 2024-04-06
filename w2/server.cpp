#include <enet/enet.h>
#include <iostream>
#include "utils.h"

const uint32_t PING_SEND_DELAY = 1000;
const int MAX_MSG_BUFFER = 511 + 1;

const char* nickname_list[MAX_PLAYERS] = {
    "R.Sanchez",
    "A.Jensen",
    "A.Pearce",
    "M.U.L.E.",
    "Vergil",
    "R.Rodriguez",
    "AmShaegar",
    "CJ",
};

int place_player(bool* is_taken, Player* players, ENetPeer* peer)
{
    int i = 0;
    while(i < MAX_PLAYERS && is_taken[i]) ++i;

    if (i < MAX_PLAYERS)
    {
        players[i].id = i;
        players[i].name = nickname_list[i];
        players[i].peer = peer;

        is_taken[i] = true;

        return i;
    }

    return -1;
}

int erase_player(bool* is_taken, Player* players, ENetPeer* peer)
{
    int i = 0;
    while(i < MAX_PLAYERS && players[i].peer != peer) ++i;

    if (i < MAX_PLAYERS)
    {
        is_taken[i] = false;
        players[i].peer = nullptr;
        return i;
    }

    return -1;
}

int get_player_number(bool* is_taken)
{
    int count = 0;
    for(int i = 0; i < MAX_PLAYERS; ++i)
        if(is_taken[i])
            ++count;
    
    return count;
}

int main()
{
    if (enet_initialize() != 0)
    {
        printf("Cannot init ENet");
        return 1;
    }
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = SERVER_PORT;

    ENetHost* server = enet_host_create(&address, MAX_PLAYERS, 2, 0, 0);
    if (!server) {
        printf("Cannot create ENet server\n");
        return 1;
    }

    Player players[MAX_PLAYERS] = {};
    bool isSlotTaken[MAX_PLAYERS] = {};
    char msgBuffer[MAX_MSG_BUFFER] = {};
    uint32_t lastPingTime = enet_time_get();

    ENetEvent event;
    while (true)
    {
        if (enet_host_service(server, &event, 10) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
                printf("New connection, %x:%u\n", event.peer->address.host, event.peer->address.port);
                
                {
                    if (event.peer->address.port == LOBBY_PORT)
                        continue;
                    
                    int id = place_player(isSlotTaken, players, event.peer);

                    if (id == -1)
                    {
                        printf("Room is full\n");
                        send_reliable(event.peer, 0, "room is full");
                        continue;
                    }

                    sprintf_s(msgBuffer, "id=%i,name=%s", id, players[id].name);
                    send_reliable(event.peer, 0, msgBuffer);

                    printf("Placed player under id [%i]\n", id);

                    for (int i = 0; i < MAX_PLAYERS; ++i)
                    {
                        if (!isSlotTaken[i] || id == i)
                            continue;
                        
                        sprintf(msgBuffer, "player,id=%i,name=%s", i, players[i].name);
                        send_reliable(event.peer, 0, msgBuffer);
                    }

                    sprintf(msgBuffer, "player,id=%i,name=%s", id, players[id].name);

                    for (int i = 0; i < MAX_PLAYERS; ++i)
                    {
                        if (!isSlotTaken[i] || id == i)
                            continue;

                        send_reliable(players[i].peer, 0, msgBuffer);
                    }
                }
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                printf("Disconnected with %x:%u\n", event.peer->address.host, event.peer->address.port);
                
                {
                    if (event.peer->address.port == LOBBY_PORT)
                        continue;
                    
                    int id = erase_player(isSlotTaken, players, event.peer);

                    if (id == -1)
                    {
                        printf("Error while errasing player (id == -1)\n");
                        continue;
                    }

                    for (int i = 0; i < MAX_PLAYERS; ++i)
                    {
                        if (!isSlotTaken[i])
                            continue;
                        
                        sprintf(msgBuffer, "left,id=%i", id);
                        send_reliable(players[i].peer, 0, msgBuffer);
                    }

                    printf("Player with id %i left\n", id);
                }
                break;
            
            case ENET_EVENT_TYPE_RECEIVE:
                printf("New packet \"%s\"\n", event.packet->data);
                enet_packet_destroy(event.packet);
                break;
            }
        }

        uint32_t currentTime = enet_time_get();
        if (currentTime - lastPingTime > PING_SEND_DELAY && get_player_number(isSlotTaken) > 0)
        {
            printf("Sending rtt\n");
            lastPingTime = currentTime;

            char* buf = msgBuffer;
            buf[0] = 'p';
            buf[1] = 'i';
            buf[2] = 'n';
            buf[3] = 'g';

            buf += 4;

            buf[0] = get_player_number(isSlotTaken) + '0';
            ++buf;

            for (int i = 0; i < MAX_PLAYERS; ++i)
            {
                if (!isSlotTaken[i])
                    continue;
                
                buf[0] = i + '0';
                ++buf;

                uint32_t rtt = players[i].peer->roundTripTime;
                if ( rtt < 10000)
                    for (int j = 0; j < 4; ++j)
                    {
                        buf[j] = (rtt % 10) + '0';
                        rtt /= 10;
                    }
                else
                    for (int j = 0; j < 4; ++j)
                        buf[j] = '9';
                buf[5] = 0;
                buf += 4;
            }

            buf[0] = '\0';

            for (int i = 0; i < MAX_PLAYERS; ++i)
            {
                if (!isSlotTaken[i])
                    continue;
                
                send_unreliable(players[i].peer, 1, msgBuffer);
            }
        }
    }

    enet_host_destroy(server);

    atexit(enet_deinitialize);
    return 0;
}
