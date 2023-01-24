//
// Created by lukemartinlogan on 1/12/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_MEMORY_BACKEND_ARRAY_BACKEND_H_
#define LABSTOR_INCLUDE_LABSTOR_MEMORY_BACKEND_ARRAY_BACKEND_H_

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
#include <labstor/constants/macros.h>
#include <labstor/introspect/system_info.h>

namespace labstor::ipc {

class ArrayBackend : public MemoryBackend {
 public:
  ArrayBackend() = default;

  ~ArrayBackend() override {}

  bool shm_init(size_t size, char *region) {
    if (size < sizeof(MemoryBackendHeader)) {
      throw SHMEM_CREATE_FAILED.format();
    }
    SetInitialized();
    Own();
    header_ = reinterpret_cast<MemoryBackendHeader *>(region);
    header_->data_size_ = size - sizeof(MemoryBackendHeader);
    data_size_ = header_->data_size_;
    data_ = region + sizeof(MemoryBackendHeader);
    return true;
  }

  bool shm_deserialize(std::string url) override {
    (void) url;
    throw SHMEM_NOT_SUPPORTED.format();
  }

  void shm_detach() override {}

  void shm_destroy() override {}
};

}  // namespace labstor::ipc

#endif  // LABSTOR_INCLUDE_LABSTOR_MEMORY_BACKEND_ARRAY_BACKEND_H_
