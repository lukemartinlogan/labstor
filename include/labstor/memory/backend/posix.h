/*
 * Copyright (C) 2022  SCS Lab <scslab@iit.edu>,
 * Luke Logan <llogan@hawk.iit.edu>,
 * Jaime Cernuda Garcia <jcernudagarcia@hawk.iit.edu>
 * Jay Lofstead <gflofst@sandia.gov>,
 * Anthony Kougkas <akougkas@iit.edu>,
 * Xian-He Sun <sun@iit.edu>
 *
 * This file is part of LabStor
 *
 * LabStor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */


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

  void Reserve(std::size_t size) {
    _Open(pid_);
    ftruncate(fd_, size);
  }

  void AttachSlot(std::size_t size) {
    char *ptr = reinterpret_cast<char*>(
        mmap(0, size, PROT_READ | PROT_WRITE,
             MAP_SHARED, fd_, 0));
    slots_.emplace_back(ptr, cur_size_, size);
    cur_size_ += size;
  }

  void MergeSlots() {
    Detach();
    char *ptr = reinterpret_cast<char*>(
        mmap(0, cur_size_, PROT_READ | PROT_WRITE,
             MAP_SHARED, fd_, 0));
    slots_.emplace_back(ptr, 0, cur_size_);
  }

 private:
  void Detach() {
    for (auto &slot : slots_) {
      munmap(slot.ptr_, slot.size_);
    }
    slots_.erase(slots_.begin(), slots_.end());
  }
};

}

#endif //LABSTOR_UNIX_SYSV_H
