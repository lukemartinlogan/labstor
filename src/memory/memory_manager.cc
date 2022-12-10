//
// Created by lukemartinlogan on 11/2/22.
//

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
  backend->Attach();
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

}