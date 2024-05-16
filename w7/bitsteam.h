#pragma once
#include <cstdint>
#include <enet/enet.h>
#include <cassert>

class Bitstream {
public:
    Bitstream(void* buffer, size_t size):
        buffer(reinterpret_cast<uint8_t*>(buffer)),
        left_size(size) {}
    
    Bitstream(ENetPacket* packet):
        buffer(reinterpret_cast<uint8_t*>(packet->data)),
        left_size(packet->dataLength) {}

    template <typename T>
    void write(const T& val)
    {
        assert(left_size >= sizeof(T));

        memcpy(buffer, &val, sizeof(T));
        shift<T>();
    }

    template <typename T>
    void read(T& val)
    {
        assert(left_size >= sizeof(T));

        memcpy(&val, buffer, sizeof(T));
        shift<T>();
    }

    template <typename T>
    void shift()
    {
        left_size -= sizeof(T);
        buffer += sizeof(T);
    }

    void writePackedUint32(uint32_t val)
    {
        assert(val < UINT32_MAX >> 2);
        if (val < UINT8_MAX >> 1)
        {
            uint8_t v = val;
            write(v);
        }
        else if (val < UINT16_MAX >> 2)
        {
            uint16_t v = val | (2 << 14);
            write(v);
        }
        else if (val < UINT32_MAX >> 2)
        {
            uint32_t v = val | (3 << 30);
            write(v);
        }
    }

    void readPackedUint32(uint32_t& val)
    {
        if (buffer[0] & (1 << 7) == 0)
        {
            uint8_t v = 0;
            read(v);
            val = v;
        }
        else if (buffer[0] & (1 << 6) == 0)
        {
            uint16_t v = 0;
            read(v);
            val = v;
        }
        else
        {
            read(val);
        }
    }

private:
    uint8_t* buffer;
    size_t left_size;
};
