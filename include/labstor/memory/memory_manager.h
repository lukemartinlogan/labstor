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


#ifndef LABSTOR_MEMORY_MEMORY_MANAGER_H_
#define LABSTOR_MEMORY_MEMORY_MANAGER_H_

#include "labstor/memory/allocator/allocator.h"
#include "backend/memory_backend.h"
#include "labstor/memory/allocator/allocator_factory.h"
#include <labstor/constants/data_structure_singleton_macros.h>

namespace labstor::ipc {

class MemoryManager {
 private:
  std::unordered_map<std::string, std::unique_ptr<MemoryBackend>> backends_;
  std::unordered_map<allocator_id_t, std::unique_ptr<Allocator>> allocators_;
  Allocator *root_allocator_;
  Allocator *default_allocator_;

 public:
  /** The default amount of memory a single allocator manages */
  static const size_t kDefaultSlotSize = GIGABYTES(64);

  /**
   * Constructor. Create the "root" allocator, used until the program defines
   * its own custom allocator
   * */
  MemoryManager() {
  }

  /**
   * Create a memory backend. Memory backends are divided into slots.
   * Each slot corresponds directly with a single allocator.
   * There can be multiple slots per-backend, enabling multiple allocation
   * policies over a single memory region.
   * */
  MemoryBackend* CreateBackend(MemoryBackendType type, const std::string &url);

  /**
   * Attaches to an existing memory backend located at \a url url.
   * */
  MemoryBackend* AttachBackend(MemoryBackendType type, const std::string &url);

  /**
   * Returns a pointer to a backend that has already been attached.
   * */
  MemoryBackend* GetBackend(const std::string &url);

  /**
   * Destroys the memory allocated by the entire backend.
   * */
  void DestroyBackend(const std::string &url);

  /**
   * Scans all attached backends for new memory allocators.
   * */
  void ScanBackends();

  /**
   * Registers an allocator. Used internally by ScanBackends, but may
   * also be used externally.
   * */
  void RegisterAllocator(std::unique_ptr<Allocator> &alloc);

  /**
   * Create and register a memory allocator for a particular backend.
   * */
  template<typename ...Args>
  Allocator* CreateAllocator(AllocatorType type,
                             const std::string &url,
                             allocator_id_t alloc_id,
                             size_t custom_header_size,
                             size_t slot_size,
                             Args&& ...args) {
    auto backend = GetBackend(url);
    auto slot = backend->CreateSlot(slot_size);
    if (alloc_id.is_null()) {
      alloc_id = allocator_id_t(LABSTOR_SYSTEM_INFO->pid_,
                                allocators_.size());
    }
    auto alloc = AllocatorFactory::Create(
      type, slot.slot_id_, backend, alloc_id,
      custom_header_size, std::forward<Args>(args)...);
    RegisterAllocator(alloc);
    return GetAllocator(alloc_id);
  }

  /**
   * Locates an allocator of a particular id
   * */
  template<typename T = Allocator>
  T* GetAllocator(allocator_id_t alloc_id) {
    if (alloc_id.is_null()) {
      return nullptr;
    }
    auto iter = allocators_.find(alloc_id);
    if (iter == allocators_.end()) {
      ScanBackends();
    }
    return reinterpret_cast<T*>(allocators_[alloc_id].get());
  }

  /**
   * Gets the allocator used by default when no allocator is
   * used to construct an object.
   * */
  Allocator* GetDefaultAllocator() {
    return default_allocator_;
  }

  /**
   * Sets the allocator used by default when no allocator is
   * used to construct an object.
   * */
  void SetDefaultAllocator(Allocator *alloc) {
    default_allocator_ = alloc;
  }

  /**
   * Convert a process-independent pointer into a process-specific pointer.
   * */
  template<typename T>
  T* Convert(Pointer &p) {
    if (p.is_null()) {
      return nullptr;
    }
    return GetAllocator(p.allocator_id_)->Convert<T>(p);
  }

  /**
   * Convert a process-specific pointer into a process-independent pointer
   *
   * @param allocator_id the allocator the pointer belongs to
   * @param ptr the pointer to convert
   * */
  template<typename T>
  Pointer Convert(allocator_id_t allocator_id, T *ptr) {
    return GetAllocator(allocator_id)->Convert(ptr);
  }

  /**
   * Convert a process-specific pointer into a process-independent pointer when
   * the allocator is unkown.
   *
   * @param ptr the pointer to convert
   * */
  template<typename T>
  Pointer Convert(T *ptr) {
    for (auto &[alloc_id, alloc] : allocators_) {
      if (alloc->ContainsPtr(ptr)) {
        return alloc->Convert(ptr);
      }
    }
    return kNullPointer;
  }
};

}  // namespace labstor::ipc

#endif  // LABSTOR_MEMORY_MEMORY_MANAGER_H_
