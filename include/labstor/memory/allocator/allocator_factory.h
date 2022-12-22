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


#ifndef LABSTOR_MEMORY_ALLOCATOR_ALLOCATOR_FACTORY_H_
#define LABSTOR_MEMORY_ALLOCATOR_ALLOCATOR_FACTORY_H_

#include "allocator.h"
#include "page_allocator.h"

namespace labstor::ipc {

class AllocatorFactory {
 public:
  /**
   * Create a new memory allocator
   * */
  template<typename ...Args>
  static std::unique_ptr<Allocator> shm_init(AllocatorType type,
                                             MemoryBackend *backend,
                                             allocator_id_t alloc_id,
                                             size_t custom_header_size,
                                             Args&& ...args) {
    switch (type) {
      case AllocatorType::kPageAllocator: {
        auto alloc = std::make_unique<PageAllocator>();
        alloc->shm_init(backend,
                        alloc_id,
                        custom_header_size,
                        std::forward<Args>(args)...);
        return alloc;
      }
      default: return nullptr;
    }
  }

  /**
   * Deserialize the allocator managing this backend.
   * */
  static std::unique_ptr<Allocator> shm_deserialize(MemoryBackend *backend) {
    auto header_ = reinterpret_cast<AllocatorHeader*>(backend->data_);
    switch (static_cast<AllocatorType>(header_->allocator_type_)) {
      case AllocatorType::kPageAllocator: {
        auto alloc = std::make_unique<PageAllocator>();
        alloc->shm_deserialize(backend);
        return alloc;
      }
      default: return nullptr;
    }
  }
};

}  // namespace labstor::ipc

#endif  // LABSTOR_MEMORY_ALLOCATOR_ALLOCATOR_FACTORY_H_
