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
