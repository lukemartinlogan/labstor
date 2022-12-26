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


#ifndef LABSTOR_DATA_STRUCTURES_SIMPLE_QUEUE_H_
#define LABSTOR_DATA_STRUCTURES_SIMPLE_QUEUE_H_

#include "labstor/types/basic.h"

namespace labstor::ipc {

struct _queue_entry {
  size_t next_;
};

template<typename T>
struct _queue_header {
  size_t head_, tail_, size_;
};

template<typename T>
class _queue {
 private:
  _queue_header<T> *header_;
  size_t region_start_;

 public:
  void shm_init(void *buffer, char *region_start) {
    header_ = reinterpret_cast<_queue_header<T>*>(buffer);
    memset(header_, 0, sizeof(_queue_header<T>));
    region_start_ = reinterpret_cast<size_t>(region_start);
  }

  void shm_deserialize(void *buffer, char *region_start) {
    header_ = reinterpret_cast<_queue_header<T>*>(buffer);
    region_start_ = reinterpret_cast<size_t>(region_start);
  }

  void verify_queue() {
    size_t cur = header_->head_;
    for (size_t i = 0; i < header_->size_; ++i) {
      if (cur > GIGABYTES(1)) {
        throw 1;
      }
      auto entry = reinterpret_cast<_queue_entry*>(cur + region_start_);
      cur = entry->next_;
    }
  }

  void enqueue(T *entry) {
    // verify_queue(); // TODO(llogan): remove
    // memset(entry, 0, sizeof(_queue_entry)); // TODO(llogan): remove
    if (header_->size_ == 0) {
      header_->head_ = reinterpret_cast<size_t>(entry) - region_start_;
      header_->tail_ = header_->head_;
      ++header_->size_;
      // verify_queue(); // TODO(llogan): remove
      return;
    }
    auto tail = reinterpret_cast<T*>(header_->tail_ + region_start_);
    tail->next_ = reinterpret_cast<size_t>(entry) - region_start_;
    header_->tail_ = tail->next_;
    ++header_->size_;
    // verify_queue(); // TODO(llogan): remove
  }

  void enqueue_off(size_t off) {
    enqueue(reinterpret_cast<T*>(region_start_ + off));
  }

  size_t dequeue_off() {
    if (header_->size_ == 0) {
      return 0;
    }
    auto head_off = header_->head_;
    auto head = reinterpret_cast<T*>(head_off + region_start_);
    header_->head_ = head->next_;
    --header_->size_;
    return head_off;
  }

  T* dequeue() {
    if (header_->size_ == 0) {
      return nullptr;
    }
    auto head = reinterpret_cast<T*>(header_->head_ + region_start_);
    header_->head_ = head->next_;
    --header_->size_;
    return head;
  }

  size_t size() { return header_->size_; }
};

}  // namespace labstor::ipc

#endif  // LABSTOR_DATA_STRUCTURES_SIMPLE_QUEUE_H_
