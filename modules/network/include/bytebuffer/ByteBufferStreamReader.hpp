#pragma once

#include "io/Read.hpp"
#include "ByteBuffer.hpp"

namespace tw::net {

class ByteBufferStreamReader {
public:
    static size_t read(Read<std::byte>* from, RingByteBuffer* to) {
        auto block = to->get_next_available_block();
        auto r = from->read_into(block);
        if(!r || *r == 0) {
            return 0;
        }

        to->skip_write(*r);

        if(*r == block.size()) {
            auto next_block = to->get_next_available_block();
            if(next_block.size() == 0) {
                return *r;
            }

            auto r2 = from->read_into(next_block);
            if(!r2 || *r2 == 0) {
                return *r;
            }

            to->skip_write(*r2);
            return *r + *r2;
        }

        return *r;
    }
};

}
