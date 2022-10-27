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
#include <labstor/memory/attachable.h>

namespace labstor::memory {

struct MemorySlot {
  char *ptr_;
  size_t off_;
  size_t size_;

  MemorySlot() = default;

  explicit MemorySlot(char *ptr, size_t off, size_t size) :
      ptr_(ptr), off_(off), size_(size) {}
};

class MemoryBackend : public Attachable {
protected:
  std::vector<MemorySlot> slots_;
  std::size_t cur_size_, max_size_;
  std::string url_;

public:
  explicit MemoryBackend(const std::string &url) :
    cur_size_(0), max_size_(0), url_(url) {}
  virtual ~MemoryBackend() = default;

  void MapSlot(std::size_t size) {
    MemorySlot slot;
    slot.off_ = cur_size_;
    slot.size_ = size;
    _MapSlot(slot);
    slots_.emplace_back(slot);
    cur_size_ += size;
  }

  const MemorySlot& GetSlot(int i) const { return slots_[i]; }

  void Detach() override {
    _UnmapSlots();
    _Detach();
  }

  void Destroy() override {
    _UnmapSlots();
    _Detach();
    _Destroy();
  }

  void Reserve(std::size_t size) {
    max_size_ = size;
    _Reserve(size);
  }

 protected:
  void _UnmapSlots() {
    for (auto &slot : slots_) {
      _FreeSlot(slot);
    }
    slots_.erase(slots_.begin(), slots_.end());
    cur_size_ = 0;
  }

  virtual void _MapSlot(MemorySlot &slot) = 0;
  virtual void _FreeSlot(MemorySlot &slot) = 0;
  virtual void _Reserve(std::size_t size) = 0;
  virtual void _Detach() = 0;
  virtual void _Destroy() = 0;
};

}

#endif //LABSTOR_MEMORY_H
