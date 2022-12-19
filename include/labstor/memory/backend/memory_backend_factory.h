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


#ifndef LABSTOR_MEMORY_BACKEND_MEMORY_BACKEND_FACTORY_H_
#define LABSTOR_MEMORY_BACKEND_MEMORY_BACKEND_FACTORY_H_

#include "memory_backend.h"
#include "posix_shm_mmap.h"

namespace labstor::ipc {

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

}  // namespace labstor::ipc

#endif  // LABSTOR_MEMORY_BACKEND_MEMORY_BACKEND_FACTORY_H_
