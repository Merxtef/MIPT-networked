#pragma once
#include <cstdint>

constexpr size_t max_name_length = 255 + 1;
constexpr uint16_t invalid_entity = -1;
constexpr float spawn_immune_time = 1.5f;
constexpr float spawn_radius = 5.f;

struct Entity
{
  uint32_t color = 0xff00ffff;
  float x = 0.f;
  float y = 0.f;
  uint16_t eid = invalid_entity;
  bool serverControlled = false;
  float targetX = 0.f;
  float targetY = 0.f;
  float radius = spawn_radius;
  char name[max_name_length] = {};
  
  bool is_immune = true;
  float immune_time = spawn_immune_time;
};

