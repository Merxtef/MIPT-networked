#pragma once
#include "mathUtils.h"
#include <limits>

template<typename T>
T pack_float(float v, float lo, float hi, int num_bits)
{
  T range = (1 << num_bits) - 1;//std::numeric_limits<T>::max();
  return range * ((clamp(v, lo, hi) - lo) / (hi - lo));
}

template<typename T>
float unpack_float(T c, float lo, float hi, int num_bits)
{
  T range = (1 << num_bits) - 1;//std::numeric_limits<T>::max();
  return float(c) / range * (hi - lo) + lo;
}

template<typename T, int num_bits>
struct PackedFloat
{
  T packedVal;

  PackedFloat(float v, float lo, float hi) { pack(v, lo, hi); }
  PackedFloat(T compressed_val) : packedVal(compressed_val) {}

  void pack(float v, float lo, float hi) { packedVal = pack_float<T>(v, lo, hi, num_bits); }
  float unpack(float lo, float hi) { return unpack_float<T>(packedVal, lo, hi, num_bits); }
};

struct Float2
{
  float x;
  float y;

  Float2(): x(0), y(0) {}
  Float2(float x, float y): x(x), y(y) {}
};

struct Float3
{
  float x;
  float y;
  float z;

  Float3(): x(0), y(0), z(0) {}
  Float3(float x, float y, float z): x(x), y(y), z(z) {}
};

template<typename T, int bits_x, int bits_y>
struct PackedFloat2
{
  T packedVal;

  PackedFloat2 (const Float2& v, const Float2& lo, const Float2& hi)
  {
    pack(v, lo, hi);
  }

  PackedFloat2 (T packedVal): packedVal(packedVal) {}

  void pack(const Float2& v, const Float2& lo, const Float2& hi)
  {
    T val_x = pack_float<T>(v.x, lo.x, hi.x, bits_x);
    T val_y = pack_float<T>(v.y, lo.y, hi.y, bits_y);

    packedVal = (val_y << bits_x) | val_x;
  }

  Float2 unpack(const Float2& lo, const Float2& hi)
  {
    return {
      unpack_float<T>(packedVal & ((1 << bits_x) - 1),  lo.x, hi.x, bits_x),
      unpack_float<T>(packedVal >> bits_x,              lo.y, hi.y, bits_y)
    };
  }
};

template<typename T, int bits_x, int bits_y, int bits_z>
struct PackedFloat3
{
  T packedVal;

  PackedFloat3 (const Float3& v, const Float3& lo, const Float3& hi)
  {
    pack(v, lo, hi);
  }

  PackedFloat3 (T packedVal): packedVal(packedVal) {}

  void pack(const Float3& v, const Float3& lo, const Float3& hi)
  {
    T val_x = pack_float<T>(v.x, lo.x, hi.x, bits_x);
    T val_y = pack_float<T>(v.y, lo.y, hi.y, bits_y);
    T val_z = pack_float<T>(v.z, lo.z, hi.z, bits_z);

    packedVal = (val_z << (bits_x + bits_y)) | (val_y << bits_x) | val_x;
  }

  Float3 unpack(const Float3& lo, const Float3& hi)
  {
    T val_x = packedVal & ((1 << bits_x) - 1);
    packedVal >>= bits_x;
    T val_y = packedVal & ((1 << bits_y) - 1);
    packedVal >>= bits_y;
    return {
      unpack_float<T>(val_x,      lo.x, hi.x, bits_x),
      unpack_float<T>(val_y,      lo.y, hi.y, bits_y),
      unpack_float<T>(packedVal,  lo.z, hi.z, bits_z)
    };
  }
};

typedef PackedFloat<uint8_t, 4> float4bitsQuantized;

