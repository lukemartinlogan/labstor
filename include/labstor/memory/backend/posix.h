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

  void AttachSlot(std::size_t size, int pid) {
    char *ptr = reinterpret_cast<char*>(
        mmap(0, size, PROT_READ | PROT_WRITE,
             MAP_SHARED, fd_, 0));
    slots_.emplace_back(ptr, cur_size_, size);
    cur_size_ += size;
  }

  void Detach() {
    for (auto &slot : slots_) {
      munmap(slot.ptr_, slot.size_);
    }
    slots_.erase(slots_.begin(), slots_.end());
  }

  void MergeSlots() {
    Detach();
    char *ptr = reinterpret_cast<char*>(
        mmap(0, cur_size_, PROT_READ | PROT_WRITE,
             MAP_SHARED, fd_, 0));
    slots_.emplace_back(ptr, 0, cur_size_);
  }
};

}

#endif //LABSTOR_UNIX_SYSV_H
