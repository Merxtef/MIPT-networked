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

private:
    uint8_t* buffer;
    size_t left_size;
};
