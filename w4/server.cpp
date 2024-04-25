#include <enet/enet.h>
#include <iostream>
#include "entity.h"
#include "protocol.h"
#include <stdlib.h>
#include <vector>
#include <map>

static std::vector<Entity> entities;
static std::map<uint16_t, ENetPeer*> controlledMap;

static uint16_t create_random_entity(bool isBot)
{
  uint16_t newEid = entities.size();
  uint32_t color = 0xff000000 +
                   0x00550000 * (1 + rand() % 3) +
                   0x00005500 * (1 + rand() % 3) +
                   0x00000055 * (1 + rand() % 3);
  float x = (rand() % 40 - 20) * 5.f;
  float y = (rand() % 40 - 20) * 5.f;
  Entity ent = {color, x, y, newEid, false, 0.f, 0.f};

  if (isBot)
  {
    ent.serverControlled = true;
    sprintf_s(ent.name, "bot-%i", entities.size());
  }
  else
    sprintf_s(ent.name, "player-%i", entities.size());

  entities.push_back(ent);
  return newEid;
}

void respawn(Entity& ent)
{
  ent.x = (rand() % 40 - 20) * 5.f;
  ent.y = (rand() % 40 - 20) * 5.f;
  ent.radius = spawn_radius;
  ent.immune_time = spawn_immune_time;
  ent.is_immune = true;

  if (!ent.serverControlled)
    ent.radius *= 2;
}

void on_join(ENetPacket *packet, ENetPeer *peer, ENetHost *host)
{
  // send all entities
  for (const Entity &ent : entities)
    send_new_entity(peer, ent);

  // find max eid
  uint16_t newEid = create_random_entity(false);
  Entity& ent = entities[newEid];
  ent.radius *= 2;

  controlledMap[newEid] = peer;


  // send info about new entity to everyone
  for (size_t i = 0; i < host->peerCount; ++i)
    send_new_entity(&host->peers[i], ent);
  // send info about controlled entity
  send_set_controlled_entity(peer, newEid);
}

void on_state(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  float x = 0.f; float y = 0.f; float r = 0.f;
  deserialize_entity_state(packet, eid, x, y, r);
  for (Entity &e : entities)
    if (e.eid == eid)
    {
      e.x = x;
      e.y = y;
      e.radius = r;
    }
}

void on_entity_devour(Entity& hunter, Entity& prey)
{
  hunter.radius += prey.radius;
  respawn(prey);

  if (!hunter.serverControlled)
  {
    send_snapshot(controlledMap[hunter.eid], hunter);
  }

  if (!prey.serverControlled)
  {
    send_snapshot(controlledMap[prey.eid], prey);
  }
}

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }
  ENetAddress address;

  address.host = ENET_HOST_ANY;
  address.port = 10131;

  ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);

  if (!server)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }

  constexpr int numAi = 10;

  for (int i = 0; i < numAi; ++i)
  {
    uint16_t eid = create_random_entity(true);
    controlledMap[eid] = nullptr;
  }

  uint32_t lastTime = enet_time_get();
  while (true)
  {
    uint32_t curTime = enet_time_get();
    float dt = (curTime - lastTime) * 0.001f;
    lastTime = curTime;
    ENetEvent event;
    while (enet_host_service(server, &event, 0) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        switch (get_packet_type(event.packet))
        {
          case E_CLIENT_TO_SERVER_JOIN:
            on_join(event.packet, event.peer, server);
            break;
          case E_CLIENT_TO_SERVER_STATE:
            on_state(event.packet);
            break;
        };
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }
    for (Entity &e : entities)
    {
      if (e.serverControlled)
      {
        const float diffX = e.targetX - e.x;
        const float diffY = e.targetY - e.y;
        const float dirX = diffX > 0.f ? 1.f : -1.f;
        const float dirY = diffY > 0.f ? 1.f : -1.f;
        constexpr float spd = 50.f;
        e.x += dirX * spd * dt;
        e.y += dirY * spd * dt;
        if (fabsf(diffX) < 10.f && fabsf(diffY) < 10.f)
        {
          e.targetX = (rand() % 40 - 20) * 15.f;
          e.targetY = (rand() % 40 - 20) * 15.f;
        }
      }
      
      if (e.is_immune)
      {
        if (e.immune_time > 0.f)
          e.immune_time -= dt;
        else
          e.is_immune = false;
      }
    }
    for (const Entity &e : entities)
    {

      for (size_t i = 0; i < server->peerCount; ++i)
      {
        ENetPeer *peer = &server->peers[i];
        if (controlledMap[e.eid] != peer)
          send_snapshot(peer, e);
      }
    }
    size_t ent_count = entities.size();
    for (int i = 0; i < ent_count; ++i)
      for (int j = i + 1; j < ent_count; ++j)
      {
        auto& ent1 = entities.at(i);
        auto& ent2 = entities.at(j);

        if (ent1.is_immune || ent2.is_immune)
          continue;

        const double dx = ent2.x - ent1.x;
        const double dy = ent2.y - ent1.y;
        const double sum_radius = (ent1.radius + ent2.radius);

        if (dx * dx + dy * dy < sum_radius * sum_radius)
        {
          if (ent2.radius > ent1.radius)
            on_entity_devour(ent2, ent1);
          else
            on_entity_devour(ent1, ent2);
          std::cout << "Devour!\n";
        }
      }
    //usleep(400000);
  }

  enet_host_destroy(server);

  atexit(enet_deinitialize);
  return 0;
}


