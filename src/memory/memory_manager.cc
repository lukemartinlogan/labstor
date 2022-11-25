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
      auto alloc = AllocatorFactory::Get(
        type, slot_id, backend.get());
      alloc->Attach();
      RegisterAllocator(alloc);
    }
  }
}

Allocator* MemoryManager::CreateAllocator(AllocatorType type,
                                          const std::string &url,
                                          allocator_id_t alloc_id,
                                          size_t slot_size,
                                          size_t custom_header_size) {
  auto backend = GetBackend(url);
  auto slot = backend->CreateSlot(slot_size);
  auto alloc = AllocatorFactory::Get(type,
                                     slot.slot_id_, backend,
                                     custom_header_size);
  if (alloc_id.is_null()) {
    alloc_id = allocator_id_t(LABSTOR_SYSTEM_INFO->pid_,
                              allocators_.size());
  }
  alloc->Create(alloc_id);
  RegisterAllocator(alloc);
  return GetAllocator(alloc_id);
}

void MemoryManager::RegisterAllocator(std::unique_ptr<Allocator> &alloc) {
  if (default_allocator_ == nullptr) {
    default_allocator_ = alloc.get();
  }
  allocators_.emplace(alloc->GetId(), std::move(alloc));
}

Allocator* MemoryManager::GetAllocator(allocator_id_t alloc_id) {
  auto iter = allocators_.find(alloc_id);
  if (iter == allocators_.end()) {
    ScanBackends();
  }
  return allocators_[alloc_id].get();
}

Allocator* MemoryManager::GetDefaultAllocator() {
  return default_allocator_;
}

}