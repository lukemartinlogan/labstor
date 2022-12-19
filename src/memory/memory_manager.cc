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


#include <labstor/memory/memory_manager.h>
#include "labstor/memory/backend/memory_backend_factory.h"
#include "labstor/memory/allocator/allocator_factory.h"
#include <labstor/introspect/system_info.h>
#include <labstor/constants/singleton_macros.h>

namespace labstor::ipc {

MemoryBackend* MemoryManager::CreateBackend(MemoryBackendType type,
                                            const std::string &url) {
  backends_.emplace(url, MemoryBackendFactory::Get(type, url));
  auto backend = backends_[url].get();
  backend->Create();
  return backend;
}

MemoryBackend* MemoryManager::AttachBackend(MemoryBackendType type,
                                            const std::string &url) {
  backends_.emplace(url, MemoryBackendFactory::Get(type, url));
  auto backend = backends_[url].get();
  if (!backend->Attach()) {
    throw MEMORY_BACKEND_NOT_FOUND.format();
  }
  ScanBackends();
  return backend;
}

MemoryBackend* MemoryManager::GetBackend(const std::string &url) {
  return backends_[url].get();
}

void MemoryManager::DestroyBackend(const std::string &url) {
  auto backend = GetBackend(url);
  backend->Destroy();
}

void MemoryManager::ScanBackends() {
  for (auto &[url, backend] : backends_) {
    for (auto slot_id = backend->FirstAllocatorSlot();
          slot_id < backend->GetNumSlots(); ++slot_id) {
      auto &slot = backend->GetSlot(slot_id);
      auto header = reinterpret_cast<AllocatorHeader*>(slot.ptr_);
      auto type = static_cast<AllocatorType>(header->allocator_type_);
      auto alloc = AllocatorFactory::Attach(type, slot_id, backend.get());
      RegisterAllocator(alloc);
    }
  }
}

void MemoryManager::RegisterAllocator(std::unique_ptr<Allocator> &alloc) {
  if (default_allocator_ == nullptr) {
    default_allocator_ = alloc.get();
  }
  allocators_.emplace(alloc->GetId(), std::move(alloc));
}

}  // namespace labstor::ipc
