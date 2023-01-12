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
#include "labstor/data_structures/thread_unsafe/_array.h"
#include <labstor/constants/macros.h>
#include <labstor/introspect/system_info.h>

namespace labstor::ipc {

class ArrayBackend : public MemoryBackend {
 private:
  size_t total_size_;

 public:
  ArrayBackend() = default;

  ~ArrayBackend() override {}

  bool shm_init(char *region, size_t size) {
    if (size < sizeof(MemoryBackendHeader)) {
      throw SHMEM_CREATE_FAILED.format();
    }
    SetInitialized();
    Own();
    total_size_ = sizeof(MemoryBackendHeader) + size;
    char *ptr = (char *) malloc(sizeof(MemoryBackendHeader));
    header_ = reinterpret_cast<MemoryBackendHeader *>(ptr);
    header_->data_size_ = size;
    data_size_ = size;
    data_ = nullptr;
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
  void _Detach() {
    free(header_);
  }

  void _Destroy() {
    free(header_);
  }
};

}  // namespace labstor::ipc

#endif  // LABSTOR_INCLUDE_LABSTOR_MEMORY_BACKEND_ARRAY_BACKEND_H_
