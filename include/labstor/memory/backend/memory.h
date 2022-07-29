//
// Created by lukemartinlogan on 7/28/22.
//

#ifndef LABSTOR_MEMORY_H
#define LABSTOR_MEMORY_H

#include <cstdint>

namespace labstor {

class MemoryBackend {
public:
    void Reserve(std::size_t size);
    void* Get();
};

}

#endif //LABSTOR_MEMORY_H
