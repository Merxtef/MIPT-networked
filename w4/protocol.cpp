#include "protocol.h"
#include "bitsteam.h"
#include <cstring> // memcpy

void send_join(ENetPeer *peer)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t), ENET_PACKET_FLAG_RELIABLE);
  *packet->data = E_CLIENT_TO_SERVER_JOIN;

  enet_peer_send(peer, 0, packet);
}

void send_new_entity(ENetPeer *peer, const Entity &ent)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(Entity) + max_name_length,
                                                   ENET_PACKET_FLAG_RELIABLE);
  Bitstream stream(packet);
  stream.write(E_SERVER_TO_CLIENT_NEW_ENTITY);
  stream.write(ent);
  stream.write(ent.name);

  enet_peer_send(peer, 0, packet);
}

void send_set_controlled_entity(ENetPeer *peer, uint16_t eid)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t),
                                                   ENET_PACKET_FLAG_RELIABLE);
  Bitstream stream(packet);
  stream.write(E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY);
  stream.write(eid);

  enet_peer_send(peer, 0, packet);
}

void send_entity_state(ENetPeer *peer, const Entity &ent)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t) +
                                                   3 * sizeof(float),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);
  Bitstream stream(packet);
  stream.write(E_CLIENT_TO_SERVER_STATE);
  stream.write(ent.eid);
  stream.write(ent.x);
  stream.write(ent.y);
  stream.write(ent.radius);

  enet_peer_send(peer, 1, packet);
}

void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y, float r, bool is_immune)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t) +
                                                   3 * sizeof(float) + sizeof(bool),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);
  Bitstream stream(packet);
  stream.write(E_SERVER_TO_CLIENT_SNAPSHOT);
  stream.write(eid);
  stream.write(x);
  stream.write(y);
  stream.write(r);
  stream.write(is_immune);

  enet_peer_send(peer, 1, packet);
}

void send_snapshot(ENetPeer *peer, const Entity& ent)
{
  send_snapshot(peer, ent.eid, ent.x, ent.y, ent.radius, ent.is_immune);
}

MessageType get_packet_type(ENetPacket *packet)
{
  return (MessageType)*packet->data;
}

void deserialize_new_entity(ENetPacket *packet, Entity &ent)
{
  Bitstream stream(packet);
  stream.shift<uint8_t>();
  stream.read(ent);
  stream.read(ent.name);
}

void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid)
{
  Bitstream stream(packet);
  stream.shift<uint8_t>();
  stream.read(eid);
}

void deserialize_entity_state(ENetPacket *packet, uint16_t &eid, float &x, float &y, float& r)
{
  Bitstream stream(packet);
  stream.shift<uint8_t>();
  stream.read(eid);
  stream.read(x);
  stream.read(y);
  stream.read(r);
}

void deserialize_entity_state(ENetPacket *packet, Entity &ent)
{
  deserialize_entity_state(packet, ent.eid, ent.x, ent.y, ent.radius);
}

void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y, float& r, bool& is_immune)
{
  Bitstream stream(packet);
  stream.shift<uint8_t>();
  stream.read(eid);
  stream.read(x);
  stream.read(y);
  stream.read(r);
  stream.read(is_immune);
}

void deserialize_snapshot(ENetPacket *packet, Entity& ent)
{
  deserialize_snapshot(packet, ent.eid, ent.x, ent.y, ent.radius, ent.is_immune);
}
