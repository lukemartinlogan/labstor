//
// Created by lukemartinlogan on 10/17/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_MEMORY_ALLOCATOR_ALLOCATOR_H_
#define LABSTOR_INCLUDE_LABSTOR_MEMORY_ALLOCATOR_ALLOCATOR_H_

#include <cstdint>
#include <labstor/memory/backend/memory_backend.h>

namespace labstor::memory {

struct Pointer {
  uint32_t allocator_id_;
  uint32_t slot_id_;
  uint64_t off_;
};

class Allocator : public Attachable {
 private:
  uint64_t id_;
  std::unique_ptr<MemoryBackend> backend_;

 public:
  Allocator(uint64_t id) : id_(id) {}
  virtual Pointer Allocate(std::size_t size) = 0;
  virtual void Free(Pointer &ptr) = 0;

  void* Convert(Pointer p) {
    auto &slot = backend_->GetSlot(p.slot_id_);
    return slot.ptr_ + p.off_;
  }
};

}  // namespace labstor::memory

#endif //LABSTOR_INCLUDE_LABSTOR_MEMORY_ALLOCATOR_ALLOCATOR_H_
