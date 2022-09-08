//
// Created by lukemartinlogan on 7/28/22.
//

#ifndef LABSTOR_UNIX_SYSV_H
#define LABSTOR_UNIX_SYSV_H

#include "memory.h"
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

namespace labstor::shmem {

class Posix : public MemoryBackend {
private:
    int fd_;
    std::size_t size_;

    void _Open(int pid) {
        std::string name = "labstor_client_" + std::to_string(pid);
        fd_ = shm_open(name.c_str(), O_CREAT | O_RDWR, 0666);
    }
public:
    Posix() : MemoryBackend() {}

    void Reserve(std::size_t size, int pid) {
        _Open(pid);
        ftruncate(fd_, size);
    }
    void Attach(std::size_t size, int pid) {
        if(!ptr_) {
            size_ = size;
            ptr_ = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
        } else {
            ptr_ = mremap(ptr_, size_, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
        }
    }

};

}

#endif //LABSTOR_UNIX_SYSV_H
