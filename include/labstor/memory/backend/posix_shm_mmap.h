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

#ifndef LABSTOR_INCLUDE_MEMORY_BACKEND_POSIX_SHM_MMAP_H
#define LABSTOR_INCLUDE_MEMORY_BACKEND_POSIX_SHM_MMAP_H

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
#include "labstor/data_structures/lockless/_array.h"
#include <labstor/constants/macros.h>

namespace labstor::ipc {

using labstor::ipc::lockless::_array;

struct SharedMemorySlot {
  size_t off_;
  size_t size_;

  SharedMemorySlot() = default;
  explicit SharedMemorySlot(const MemorySlot &slot) {
    off_ = slot.off_;
    size_ = slot.size_;
  }
};

class PosixShmMmap : public MemoryBackend {
 private:
  int fd_;
  _array<SharedMemorySlot> slot_array_;

 public:
  explicit PosixShmMmap(const std::string &url) :
    MemoryBackend(url), fd_(-1) {}
  ~PosixShmMmap() override = default;

  bool Create() override {
    fd_ = shm_open(url_.c_str(), O_CREAT | O_RDWR, 0666);
    if (fd_ < 0) {
      return false;
    }
    auto &header_slot = CreateSlot(header_size_);
    header_ = reinterpret_cast<MemoryBackendHeader*>(header_slot.ptr_);
    header_->num_slots_ = 1;
    header_->header_size_ = header_size_;
    header_->cur_size_ = header_size_;
    header_->max_size_ = max_size_;
    slot_array_.Create(header_slot.ptr_ + sizeof(MemoryBackendHeader),
                       header_slot.size_ - sizeof(MemoryBackendHeader));
    slot_array_.emplace_back(header_slot);
    return true;
  }

  bool Attach() override {
    // Map shared memory
    fd_ = shm_open(url_.c_str(), O_RDWR, 0666);
    if (fd_ < 0) {
      return false;
    }

    // Get the size of the memory header
    auto only_header_slot = GetHeaderSlot();
    header_ = reinterpret_cast<MemoryBackendHeader*>(only_header_slot.ptr_);
    header_size_ = header_->header_size_;
    _UnmapSlot(only_header_slot);

    // Map the memory header
    auto header_slot = GetHeaderSlot(header_size_, true);
    header_ = reinterpret_cast<MemoryBackendHeader*>(header_slot.ptr_);
    slot_array_.Attach(reinterpret_cast<void*>(header_ + 1));
    return true;
  }

  void Detach() override {
    _Detach();
  }

  void Destroy() override {
    _Destroy();
  }

 protected:
  void _Reserve(size_t size) override {
    ftruncate64(fd_, static_cast<off64_t>(size));
  }

  void _MapSlot(MemorySlot &slot, bool create) override {
    if (!create && slot_array_.IsInitialized()) {
      auto &shm_slot = slot_array_[slot.slot_id_];
      slot.off_ = shm_slot.off_;
      slot.size_ = shm_slot.size_;
    }
    slot.ptr_ = reinterpret_cast<char*>(
      mmap64(nullptr, slot.size_, PROT_READ | PROT_WRITE,
             MAP_SHARED, fd_, static_cast<off64_t>(slot.off_)));
    if (slot.ptr_ == MAP_FAILED) {
      throw SHMEM_CREATE_FAILED.format();
    }
    if (create && slot_array_.IsInitialized()) {
      slot_array_.emplace_back(slot);
    }
  }

  void _UnmapSlot(MemorySlot &slot) {
    munmap(slot.ptr_, slot.size_);
  }

  void _Detach() {
    for (auto &slot : slots_) {
      _UnmapSlot(slot);
    }
    close(fd_);
  }

  void _Destroy() {
    Detach();
    shm_unlink(url_.c_str());
  }
};

}  // namespace labstor::ipc

#endif  // LABSTOR_INCLUDE_MEMORY_BACKEND_POSIX_SHM_MMAP_H
