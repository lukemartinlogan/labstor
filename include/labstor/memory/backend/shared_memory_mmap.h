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

#include "memory_backend.h"
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

#include <labstor/util/errors.h>

namespace labstor::memory {

class SharedMemoryMmap : public MemoryBackend {
private:
  int fd_;

public:
  SharedMemoryMmap(const std::string &url) : MemoryBackend(url), fd_(-1) {}
  ~SharedMemoryMmap() override = default;

  bool Create() override {
    fd_ = shm_open(url_.c_str(), O_CREAT | O_RDWR, 0666);
    if (fd_ < 0) {
      return false;
    }
    return true;
  }

  bool Attach() override {
    fd_ = shm_open(url_.c_str(), O_RDWR, 0666);
    if (fd_ < 0) {
      return false;
    }
    return true;
  }

 protected:
  void _Reserve(std::size_t size) override {
    ftruncate(fd_, size);
  }

  void _MapSlot(MemorySlot &slot) override {
    char *ptr = reinterpret_cast<char*>(
        mmap(0, slot.size_, PROT_READ | PROT_WRITE,
             MAP_SHARED, fd_, 0));
    if (ptr == MAP_FAILED) {
      throw SHMEM_CREATE_FAILED.format();
    }
  }

  void _FreeSlot(MemorySlot &slot) override {
    munmap(slot.ptr_, slot.size_);
  }

  void _Detach() override {
    close(fd_);
  }

  void _Destroy() override {
    shm_unlink(url_.c_str());
  }
};

}  // namespace labstor::memory

#endif  // LABSTOR_UNIX_SYSV_H
