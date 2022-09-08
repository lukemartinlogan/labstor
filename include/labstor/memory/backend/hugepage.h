//
// Created by lukemartinlogan on 7/28/22.
//

#ifndef LABSTOR_UNIX_SYSV_H
#define LABSTOR_UNIX_SYSV_H

#include "memory.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

namespace labstor {

class Hugepage : public MemoryBackend {
public:
    void Reserve(std::size_t size) {}

};

}

#endif //LABSTOR_UNIX_SYSV_H
