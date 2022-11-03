//
// Created by lukemartinlogan on 10/29/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_MEMORY_BACKEND_MEMORY_BACKEND_FACTORY_H_
#define LABSTOR_INCLUDE_LABSTOR_MEMORY_BACKEND_MEMORY_BACKEND_FACTORY_H_

#include "memory_backend.h"
#include "posix_shm_mmap.h"

namespace labstor::memory {

class MemoryBackendFactory {
 public:
  static std::unique_ptr<MemoryBackend> Get(MemoryBackendType type,
                                            const std::string &url) {
    switch (type) {
      case MemoryBackendType::kPosixShmMmap: {
        return std::make_unique<PosixShmMmap>(url);
      }
      default: return nullptr;
    }
  }
};

}  // namespace labstor::memory

#endif  // LABSTOR_INCLUDE_LABSTOR_MEMORY_BACKEND_MEMORY_BACKEND_FACTORY_H_
