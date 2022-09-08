//
// Created by lukemartinlogan on 7/28/22.
//

#ifndef LABSTOR_MEMORY_H
#define LABSTOR_MEMORY_H

#include <cstdint>

namespace labstor {

class MemoryBackend {
protected:
    void *ptr_;
public:
    MemoryBackend() : ptr_(nullptr) {}
    virtual void Reserve(std::size_t size, int pid) = 0;
    virtual void Attach(std::size_t size, int pid) = 0;
    char* GetPtr() { return static_cast<char*>(ptr_); }
};

}

#endif //LABSTOR_MEMORY_H
