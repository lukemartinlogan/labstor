//
// Created by lukemartinlogan on 7/28/22.
//

#ifndef LABSTOR_MEMORY_H
#define LABSTOR_MEMORY_H

#include <cstdint>
#include <vector>

namespace labstor {

struct MemorySlot {
  char *ptr_;
  size_t off_;
  size_t size_;

  MemorySlot(char *ptr, size_t off, size_t size) : ptr_(ptr), off_(off), size_(size) {}
};

class MemoryBackend {
protected:
  std::vector<MemorySlot> slots_;
  std::size_t cur_size_;
public:
  MemoryBackend() : cur_size_(0) {}
  virtual void Reserve(std::size_t size, int pid) = 0;
  virtual void AttachSlot(std::size_t size, int pid) = 0;
  virtual void MergeSlots() = 0;
  char* GetPtr(int i) { return slots_[i].ptr_; }
};

}

#endif //LABSTOR_MEMORY_H
