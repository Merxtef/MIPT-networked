#include "raylib.h"
#include <enet/enet.h>
#include <iostream>

#include "utils.h"

const int MAX_NAME_LENGTH = 255 + 1;
const int MAX_BUF_SIZE = 255 + 1;
const uint32_t SEND_POS_DELAY = 120;

void send_fragmented_packet(ENetPeer *peer)
{
  const char *baseMsg = "Stay awhile and listen. ";
  const size_t msgLen = strlen(baseMsg);

  const size_t sendSize = 2500;
  char *hugeMessage = new char[sendSize];
  for (size_t i = 0; i < sendSize; ++i)
    hugeMessage[i] = baseMsg[i % msgLen];
  hugeMessage[sendSize-1] = '\0';

  ENetPacket *packet = enet_packet_create(hugeMessage, sendSize, ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, packet);

  delete[] hugeMessage;
}

void send_micro_packet(ENetPeer *peer)
{
  const char *msg = "dv/dt";
  ENetPacket *packet = enet_packet_create(msg, strlen(msg) + 1, ENET_PACKET_FLAG_UNSEQUENCED);
  enet_peer_send(peer, 1, packet);
}

void remove_player(Player *players, int id)
{
  delete players[id].name;

  players[id].id = -1;
  players[id].name = nullptr;
  players[id].peer = nullptr;
  players[id].rtt = (uint32_t) -1;
}

void update_player(Player *players, int id, const char* name)
{
  char* userName = new char[strlen(name)];
  strcpy(userName, name);

  players[id].id = id;
  players[id].name = userName;
}

void update_rtt(Player *players, int id, uint32_t rtt)
{
  players[id].rtt = rtt;
}

int main(int argc, const char **argv)
{
  int width = 800;
  int height = 600;
  InitWindow(width, height, "w6 AI MIPT");

  const int scrWidth = GetMonitorWidth(0);
  const int scrHeight = GetMonitorHeight(0);
  if (scrWidth < width || scrHeight < height)
  {
    width = std::min(scrWidth, width);
    height = std::min(scrHeight - 150, height);
    SetWindowSize(width, height);
  }

  SetTargetFPS(60);               // Set our game to run at 60 frames-per-second

  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }

  ENetHost *client = enet_host_create(nullptr, 2, 2, 0, 0);
  if (!client)
  {
    printf("Cannot create ENet client\n");
    return 1;
  }

  ENetAddress lobbyAddress;
  enet_address_set_host(&lobbyAddress, "localhost");
  lobbyAddress.port = 10887;

  ENetPeer *lobbyPeer = enet_host_connect(client, &lobbyAddress, 2, 0);
  if (!lobbyPeer)
  {
    printf("Cannot connect to lobby");
    return 1;
  }

  ENetPeer *serverPeer = nullptr;

  Player players[MAX_PLAYERS] = {};

  uint32_t timeStart = enet_time_get();
  uint32_t lastPosSendTime = timeStart;
  bool connected = false;
  bool wasEnterPressed = false;
  float posx = 0.f;
  float posy = 0.f;
  int id = -1;
  char name[MAX_NAME_LENGTH] = {};
  char buf[MAX_BUF_SIZE] = {};
  int idBuf = -1;
  while (!WindowShouldClose())
  {
    const float dt = GetFrameTime();
    ENetEvent event;
    while (enet_host_service(client, &event, 10) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        connected = true;
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        printf("Packet received '%s'\n", event.packet->data);
        {
          const char* data = (const char*) event.packet->data;
          
          if (strcmp(data, "server-info") == 0)
          {
            ENetAddress serverAddress;
            enet_address_set_host(&serverAddress, "localhost");
            serverAddress.port = SERVER_PORT;

            serverPeer = enet_host_connect(client, &serverAddress, 2, 0);

            if (!serverPeer)
              printf("Unable to connect to server\n");
            else
              printf("Connected to server");
          }
          int read = sscanf(data, "id=%i,name=%s", &id, name);

          if (read > 0)
          {
            printf("Got my id = %i and name = \"%s\"\n", id, name);
            update_player(players, id, name);
          }
          
          read = sscanf(data, "left,id=%i", &idBuf);
          
          if (read > 0)
          {
            printf("Player with id [%i] left\n", idBuf);
            remove_player(players, idBuf);
          }

          read = sscanf(data, "player,id=%i,name=%s", &idBuf, buf);

          if (read > 0)
          {
            printf("Got other player id = %i and name = \"%s\"\n", idBuf, buf);
            update_player(players, idBuf, buf);
          }

          if (data[0] == 'p' && data[1] == 'i' && data[2] == 'n' && data[3] == 'g')
          {
            const char* reader = data + 4;
            printf("Updating ping\n");

            int num = reader[0] - '0';
            ++reader;

            for (int i = 0; i < num; ++i)
            {
              int id = reader[0] - '0';
              ++reader;

              uint32_t rtt = 0;
              int mul = 1;
              for (int i = 0; i < 4; ++i)
              {
                rtt += (reader[i] - '0') * mul;
                mul *= 10;
              }
              reader += 4;

              update_rtt(players, id, rtt);
            }
          }
        }
        enet_packet_destroy(event.packet);

        break;
      default:
        break;
      };
    }
    if (connected)
    {
      uint32_t curTime = enet_time_get();
      // if (curTime - lastFragmentedSendTime > 1000)
      // {
      //   lastFragmentedSendTime = curTime;
      //   send_fragmented_packet(lobbyPeer);
      // }
      // if (curTime - lastMicroSendTime > 100)
      // {
      //   lastMicroSendTime = curTime;
      //   send_micro_packet(lobbyPeer);
      // }
      if (wasEnterPressed)
      {
        wasEnterPressed = false;

        send_reliable(lobbyPeer, 0, "start_game");
      }
    }

    wasEnterPressed = wasEnterPressed || IsKeyDown(KEY_ENTER);
    bool left = IsKeyDown(KEY_LEFT);
    bool right = IsKeyDown(KEY_RIGHT);
    bool up = IsKeyDown(KEY_UP);
    bool down = IsKeyDown(KEY_DOWN);
    constexpr float spd = 10.f;
    posx += ((left ? -1.f : 0.f) + (right ? 1.f : 0.f)) * dt * spd;
    posy += ((up ? -1.f : 0.f) + (down ? 1.f : 0.f)) * dt * spd;

    uint32_t current_time = enet_time_get();
    if(serverPeer != nullptr &&  current_time - lastPosSendTime > SEND_POS_DELAY)
    {
      lastPosSendTime = current_time;
      sprintf(buf, "x=%f,y=%f", posx, posy);
      send_unreliable(serverPeer, 1, buf);
    }

    BeginDrawing();
      ClearBackground(BLACK);
      DrawText(TextFormat("Current status: %s", "unknown"), 20, 20, 20, WHITE);
      DrawText(TextFormat("My position: (%d, %d)", (int)posx, (int)posy), 20, 40, 20, WHITE);
      DrawText("List of players:", 20, 60, 20, WHITE);

      int offset = 80;
      for (int i = 0; i < MAX_PLAYERS; ++i)
      {
        if (players[i].id < 0)
          continue;
        
        DrawText(players[i].name, 60, offset, 16, (i == id) ? YELLOW : WHITE);
        if (players[i].rtt != -1)
        {
          auto ttsColor = GREEN;
          if (players[i].rtt >= 100)
            ttsColor = ORANGE;
          else if (players[i].rtt >= 500)
            ttsColor = RED;
          
          DrawText(((players[i].rtt < 1000) ? TextFormat("%i", players[i].rtt) : ">999"), 22, offset, 16, ttsColor);
        }
        else
        {
          DrawText("???", 22, offset, 16, RED);
        }

        offset += 20;
      }

    EndDrawing();
  }
  return 0;
}
