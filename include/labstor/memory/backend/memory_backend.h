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
};

class MemoryBackend : public Attachable {
 protected:
  std::string url_;
  std::vector<MemorySlot> slots_;
  size_t cur_size_;

 public:
  explicit MemoryBackend(const std::string &url) :
    url_(url), cur_size_(0) {}

  virtual ~MemoryBackend() = default;

  void Reserve(size_t size) {
    _Reserve(size);
  }

  void MapSlot(size_t size, bool create) {
    MemorySlot slot;
    slot.off_ = cur_size_;
    slot.size_ = size;
    _MapSlot(slot, create);
    slots_.emplace_back(slot);
    cur_size_ += size;
  }

  [[nodiscard]]
  const MemorySlot& GetSlot(uint32_t slot_id) {
    if (slot_id >= slots_.size()) {
      _GetSlot(slot_id);
    }
    return slots_[slot_id];
  }

 protected:
  virtual void _Reserve(size_t size) = 0;
  virtual void _MapSlot(MemorySlot &slot, bool create) = 0;
  virtual void _GetSlot(uint32_t slot_id) = 0;
};

}  // namespace labstor::memory

#endif  // LABSTOR_MEMORY_H
