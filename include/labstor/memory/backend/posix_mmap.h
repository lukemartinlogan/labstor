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

#ifndef LABSTOR_INCLUDE_MEMORY_BACKEND_POSIX_MMAP_H
#define LABSTOR_INCLUDE_MEMORY_BACKEND_POSIX_MMAP_H

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
#include "labstor/data_structures/thread_unsafe/_array.h"
#include <labstor/constants/macros.h>
#include <labstor/introspect/system_info.h>

namespace labstor::ipc {

class PosixMmap : public MemoryBackend {
 private:
  size_t total_size_;

 public:
  PosixMmap() = default;

  ~PosixMmap() override {
    if (IsOwned()) {
      _Destroy();
    } else {
      _Detach();
    }
  }

  bool shm_init(size_t size) {
    SetInitialized();
    Own();
    total_size_ = sizeof(MemoryBackendHeader) + size;
    char *ptr = _Map(total_size_);
    header_ = reinterpret_cast<MemoryBackendHeader*>(ptr);
    header_->data_size_ = size;
    data_size_ = size;
    data_ = reinterpret_cast<char*>(header_ + 1);
    return true;
  }

  bool shm_deserialize(std::string url) override {
    (void) url;
    throw SHMEM_NOT_SUPPORTED.format();
  }

  void shm_detach() override {
    _Detach();
  }

  void shm_destroy() override {
    _Destroy();
  }

 protected:
  template<typename T=char>
  T* _Map(size_t size) {
    T *ptr = reinterpret_cast<T*>(
      mmap64(nullptr, NextPageSizeMultiple(size), PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    if (ptr == MAP_FAILED) {
      perror("map failed");
      throw SHMEM_CREATE_FAILED.format();
    }
    return ptr;
  }

  void _Detach() {
    if (!IsInitialized()) { return; }
    munmap(reinterpret_cast<void*>(header_), total_size_);
    UnsetInitialized();
  }

  void _Destroy() {
    if (!IsInitialized()) { return; }
    _Detach();
    UnsetInitialized();
  }
};

}  // namespace labstor::ipc

#endif  // LABSTOR_INCLUDE_MEMORY_BACKEND_POSIX_MMAP_H
