//
// Created by lukemartinlogan on 7/28/22.
//

#ifndef LABSTOR_MEMORY_H
#define LABSTOR_MEMORY_H

#include <cstdint>
#include <vector>
#include <string>
#include <labstor/constants/singleton_macros.h>

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
  int pid_;
  std::string mem_url_;
public:
  MemoryBackend() : cur_size_(0) {
    auto sys_info = LABSTOR_SYSTEM_INFO;
    pid_ = sys_info->pid_;
    mem_url_ = "labstor_mem_" + std::to_string(pid_);
  }
  virtual void Reserve(std::size_t size) = 0;
  virtual void AttachSlot(std::size_t size) = 0;
  virtual void MergeSlots() = 0;
  const MemorySlot& GetSlot(int i) const { return slots_[i]; }
};

}

#endif //LABSTOR_MEMORY_H
