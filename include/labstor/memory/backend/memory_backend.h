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
#include <labstor/memory/memory.h>
#include "labstor/constants/macros.h"
#include <limits>

namespace labstor::memory {

struct MemorySlot {
  slot_id_t slot_id_;
  char *ptr_;
  size_t off_;
  size_t size_;

  MemorySlot() = default;
};

struct MemoryBackendHeader {
  size_t num_slots_;
  size_t slot_table_size_;
  size_t cur_size_;
  size_t max_size_;
};

enum class MemoryBackendType {
  kPosixShmMmap
};

class MemoryBackend {
 protected:
  std::string url_;
  size_t slot_table_size_, max_size_;
  MemoryBackendHeader *header_;
  std::vector<MemorySlot> slots_;

 public:
  explicit MemoryBackend(std::string url) :
    url_(std::move(url)), slot_table_size_(MEGABYTES(1)),
    max_size_(std::numeric_limits<size_t>::max()),
    header_(nullptr) {}

  explicit MemoryBackend(std::string url,
                         size_t slot_table_size, size_t max_size) :
    url_(std::move(url)), slot_table_size_(slot_table_size),
    max_size_(max_size), header_(nullptr) {}

  virtual ~MemoryBackend() = default;

  size_t GetNumSlots() {
    return header_->num_slots_;
  }

  size_t GetMappedSlots() {
    return slots_.size();
  }

  MemorySlot& CreateSlot(size_t size) {
    MemorySlot slot;
    slot.size_ = size;
    if (header_) {
      slot.slot_id_ = header_->num_slots_;
      _Reserve(header_->cur_size_ + size);
    } else {
      slot.slot_id_ = 0;
    }
    _MapSlot(slot, true);
    slots_.emplace_back(slot);
    if (header_) {
      header_->num_slots_ += 1;
      header_->cur_size_ += size;
    }
    return slots_[slot.slot_id_];
  }

  MemorySlot& GetSlot(slot_id_t slot_id) {
    for(slot_id_t i = slots_.size(); i < slot_id; ++i) {
      MemorySlot slot;
      slot.slot_id_ = slot_id;
      _MapSlot(slot, false);
      slots_.emplace_back(slot);
    }
    return slots_[slot_id];
  }

  virtual bool Create() = 0;
  virtual bool Attach() = 0;
  virtual void Detach() = 0;
  virtual void Destroy() = 0;

 protected:
  virtual void _Reserve(size_t size) = 0;
  virtual void _MapSlot(MemorySlot &slot, bool create) = 0;
};

}  // namespace labstor::memory

#endif  // LABSTOR_MEMORY_H
