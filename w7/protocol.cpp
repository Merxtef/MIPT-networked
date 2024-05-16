#include "protocol.h"
#include "quantisation.h"
#include "bitsteam.h"
#include <cstring> // memcpy
#include <iostream>

void send_join(ENetPeer *peer)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t), ENET_PACKET_FLAG_RELIABLE);

  Bitstream stream(packet);
  stream.write<uint8_t>(E_CLIENT_TO_SERVER_JOIN);

  enet_peer_send(peer, 0, packet);
}

void send_new_entity(ENetPeer *peer, const Entity &ent)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(Entity),
                                                   ENET_PACKET_FLAG_RELIABLE);

  Bitstream stream(packet);
  stream.write<uint8_t>(E_SERVER_TO_CLIENT_NEW_ENTITY);
  stream.write(ent);

  enet_peer_send(peer, 0, packet);
}

void send_set_controlled_entity(ENetPeer *peer, uint16_t eid)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t),
                                                   ENET_PACKET_FLAG_RELIABLE);

  Bitstream stream(packet);
  stream.write<uint8_t>(E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY);
  stream.write(eid);

  enet_peer_send(peer, 0, packet);
}

void send_entity_input(ENetPeer *peer, uint16_t eid, float thr, float ori)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t) +
                                                   sizeof(uint8_t),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);

  PackedFloat2<uint8_t, 4, 4> thrSteer(Float2(thr, ori), Float2(-1.f, -1.f), Float2(1.f, 1.f));
  Bitstream stream(packet);
  stream.write<uint8_t>(E_CLIENT_TO_SERVER_INPUT);
  stream.write(eid);
  stream.write(thrSteer.packedVal);

  enet_peer_send(peer, 1, packet);
}

typedef PackedFloat<uint16_t, 11> PositionXQuantized;
typedef PackedFloat<uint16_t, 10> PositionYQuantized;

void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y, float ori)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t) +
                                                   sizeof(uint32_t),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);


  PackedFloat3<uint32_t, 11, 11, 8> xyOri(Float3(x, y, ori), Float3(-16, -8, -PI), Float3(16, 8, PI));
  Bitstream stream(packet);
  stream.write<uint8_t>(E_SERVER_TO_CLIENT_SNAPSHOT);
  stream.write(eid);
  stream.write(xyOri.packedVal);

  enet_peer_send(peer, 1, packet);
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
}

void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid)
{
  Bitstream stream(packet);
  stream.shift<uint8_t>();
  stream.read(eid);
}

void deserialize_entity_input(ENetPacket *packet, uint16_t &eid, float &thr, float &steer)
{ 
  uint8_t thrSteerPacked = 0;
  Bitstream stream(packet);
  stream.shift<uint8_t>();
  stream.read(eid);
  stream.read(thrSteerPacked);
  PackedFloat2<uint8_t, 4, 4> thrSteer(thrSteerPacked);
  Float2 unpacked = thrSteer.unpack(Float2(-1.f, -1.f), Float2(1.f, 1.f));

  thr = unpacked.x;
  steer = unpacked.y;
}

void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y, float &ori)
{
  uint32_t xyOriPacked = 0;
  Bitstream stream(packet);
  stream.shift<uint8_t>();
  stream.read(eid);
  stream.read(xyOriPacked);
  PackedFloat3<uint32_t, 11, 11, 8> xyOri(xyOriPacked);
  Float3 unpacked = xyOri.unpack(Float3(-16, -8, -PI), Float3(16, 8, PI));

  x = unpacked.x;
  y = unpacked.y;
  ori = unpacked.z;
}

