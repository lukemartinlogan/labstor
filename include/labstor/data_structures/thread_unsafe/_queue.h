//
// Created by lukemartinlogan on 11/4/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_SIMPLE_QUEUE_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_SIMPLE_QUEUE_H_

#include "labstor/types/basic.h"

namespace labstor::ipc {

struct _queue_entry {
  size_t next_;
};

struct _queue_header {
  size_t head_, tail_, size_;
};

template<typename T>
class _queue {
 private:
  _queue_header *header_;
  size_t region_start_;

 public:
  void Create(void *buffer, char *region_start) {
    header_ = reinterpret_cast<_queue_header*>(buffer);
    memset(header_, 0, sizeof(_queue_header));
    region_start_ = reinterpret_cast<size_t>(region_start);
  }

  void Attach(void *buffer, char *region_start) {
    header_ = reinterpret_cast<_queue_header*>(buffer);
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
    verify_queue(); // TODO(llogan): remove
    memset(entry, 0, sizeof(_queue_entry)); // TODO(llogan): remove
    if (header_->size_ == 0) {
      header_->head_ = reinterpret_cast<size_t>(entry) - region_start_;
      header_->tail_ = header_->head_;
      ++header_->size_;
      verify_queue(); // TODO(llogan): remove
      return;
    }
    auto tail = reinterpret_cast<T*>(header_->tail_ + region_start_);
    tail->next_ = reinterpret_cast<size_t>(entry) - region_start_;
    header_->tail_ = tail->next_;
    ++header_->size_;
    verify_queue(); // TODO(llogan): remove
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

#endif  // LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_SIMPLE_QUEUE_H_
