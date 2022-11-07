//
// Created by lukemartinlogan on 11/4/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_SIMPLE_QUEUE_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_SIMPLE_QUEUE_H_

#include "labstor/types/basic.h"

namespace labstor::ipc::lockless {

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
  void Create(void *buffer, size_t size, char *region_start) {
    header_ = reinterpret_cast<_queue_header*>(buffer);
    memset(header_, 0, size);
    region_start_ = reinterpret_cast<size_t>(region_start);
  }

  void Attach(void *buffer, char *region_start) {
    header_ = reinterpret_cast<_queue_header*>(buffer);
    region_start_ = reinterpret_cast<size_t>(region_start);
  }

  void enqueue(T *entry) {
    if (header_->size_ == 0) {
      header_->head_ = reinterpret_cast<size_t>(entry) - region_start_;
      header_->tail_ = header_->head_;
      ++header_->size_;
      return;
    }
    auto tail = reinterpret_cast<T*>(header_->tail_ + region_start_);
    tail->next_ = reinterpret_cast<size_t>(entry) - region_start_;
    header_->tail_ = tail->next_;
    ++header_->size_;
  }

  void enqueue_off(size_t off) {
    enqueue(reinterpret_cast<T*>(region_start_ + off));
  }

  size_t dequeue_off() {
    if (header_->size_ == 0) {
      return 0;
    }
    auto head = reinterpret_cast<T*>(header_->head_ + region_start_);
    header_->head_ = head->next_;
    --header_->size_;
    return header_->head_;
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

}  // namespace labstor

#endif  // LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_SIMPLE_QUEUE_H_
