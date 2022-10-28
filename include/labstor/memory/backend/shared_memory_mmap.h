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
#include <labstor/memory/data_structures/array.h>
#include <labstor/constants/macros.h>

namespace labstor::memory {

struct SharedMemorySlot {
  size_t off_;
  size_t size_;

  SharedMemorySlot() = default;
  explicit SharedMemorySlot(const MemorySlot &slot) {
    off_ = slot.off_;
    size_ = slot.size_;
  }
};

class SharedMemoryMmap : public MemoryBackend {
private:
  int fd_;
  size_t header_size_, reserve_step_;
  Array<SharedMemorySlot> slot_array_;

public:
  SharedMemoryMmap(const std::string &url) :
    MemoryBackend(url), fd_(-1), header_size_(MEGABYTES(32)) {}
  ~SharedMemoryMmap() override = default;

  bool Create() override {
    fd_ = shm_open(url_.c_str(), O_CREAT | O_RDWR, 0666);
    if (fd_ < 0) {
      return false;
    }
    MapSlot(header_size_);
    auto &slot = GetSlot(0);
    slot_array_.Create(slot.ptr_, slot.size_);
    slot_array_.emplace_back(slot);
    return true;
  }

  bool Attach() override {
    // Map shared memory
    fd_ = shm_open(url_.c_str(), O_RDWR, 0666);
    if (fd_ < 0) {
      return false;
    }
    // Load the slot array header
    MapSlot(header_size_);
    auto &slot = GetSlot(0);
    slot_array_.Attach(slot.ptr_);
    // Attach all known slots
    for (auto shm_slot = slot_array_.begin() + 1;
         shm_slot != slot_array_.end(); ++shm_slot) {
      MapSlot(shm_slot->size_);
    }
    return true;
  }

  void Detach() override {
    for (auto &slot : slots_) {
      munmap(slot.ptr_, slot.size_);
    }
    close(fd_);
  }

 protected:
  void _Reserve(size_t size) override {
    ftruncate64(fd_, static_cast<off64_t>(size));
  }

  void _MapSlot(MemorySlot &slot) override {
    slot.ptr_ = reinterpret_cast<char*>(
      mmap64(nullptr, slot.size_, PROT_READ | PROT_WRITE,
             MAP_SHARED, fd_, static_cast<off64_t>(slot.off_)));
    if (slot.ptr_ == MAP_FAILED) {
      throw SHMEM_CREATE_FAILED.format();
    }
    if (slot_array_.IsInitialized()) {
      slot_array_.emplace_back(slot);
    }
  }

  void _Destroy() override {
    shm_unlink(url_.c_str());
  }

  void _GetSlot(uint32_t slot_id) {
    for (uint32_t i = slots_.size(); i <= slot_id; ++i) {
      auto &shm_slot = slot_array_[i];
      MapSlot(shm_slot.size_);
    }
  }
};

}  // namespace labstor::memory

#endif  // LABSTOR_UNIX_SYSV_H
