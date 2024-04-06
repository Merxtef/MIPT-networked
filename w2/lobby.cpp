#include <enet/enet.h>
#include <iostream>
#include <vector>

#include "utils.h"

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }
  ENetAddress address;

  address.host = ENET_HOST_ANY;
  address.port = 10887;

  ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);

  if (!server)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }

  bool isGameStarted = false;
  std::vector<ENetPeer*> peerList = {};

  while (true)
  {
    ENetEvent event;
    while (enet_host_service(server, &event, 10) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);

        peerList.push_back(event.peer);

        if (isGameStarted)
          send_reliable(event.peer, 0, "server-info");

        // send_reliable(event.peer, 0, "server-info,id=0,name=test");
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        printf("Packet received '%s'\n", event.packet->data);

        if (!isGameStarted)
        {
          if (strcmp((const char*) event.packet->data, "start_game") == 0)
          {
            isGameStarted = true;
            printf("Starting game");

            for (ENetPeer* peer : peerList)
            {
              send_reliable(peer, 0, "server-info");
            }
          }
        }
          

        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }
  }

  enet_host_destroy(server);

  atexit(enet_deinitialize);
  return 0;
}

