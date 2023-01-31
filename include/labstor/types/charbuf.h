//
// Created by lukemartinlogan on 1/31/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_TYPES_CHARBUF_H_
#define LABSTOR_INCLUDE_LABSTOR_TYPES_CHARBUF_H_

#include "basic.h"
#include "labstor/memory/memory_manager.h"
#include <string>

namespace labstor {

/** An uninterpreted array of bytes */
struct charbuf {
  lipc::Allocator *alloc_; /**< The allocator used to allocate data */
  char *data_; /**< The pointer to data */
  size_t size_; /**< The size of data */
  bool destructable_;  /**< Whether or not this container owns data */

  /** Default constructor */
  charbuf() : alloc_(nullptr), data_(nullptr), size_(0), destructable_(false) {}

  /** Destructor */
  ~charbuf() { Free(); }

  /** Size-based constructor */
  explicit charbuf(size_t size) {
    Allocate(LABSTOR_MEMORY_MANAGER->GetDefaultAllocator(), size);
  }

  /** String-based constructor */
  explicit charbuf(const std::string &data) {
    Allocate(LABSTOR_MEMORY_MANAGER->GetDefaultAllocator(), data.size());
    memcpy(data_, data.data(), data.size());
  }

  /** Pointer-based constructor */
  explicit charbuf(char *data, size_t size)
  : alloc_(nullptr), data_(data), size_(size), destructable_(false) {}

  /** Copy constructor */
  charbuf(const charbuf &other) {
    if (!Allocate(LABSTOR_MEMORY_MANAGER->GetDefaultAllocator(),
                  other.size())) {
      return;
    }
    memcpy(data_, other.data(), size());
  }

  /** Copy assignment operator */
  charbuf& operator=(const charbuf &other) {
    if (this != &other) {
      Free();
      if (!Allocate(LABSTOR_MEMORY_MANAGER->GetDefaultAllocator(),
                    other.size())) {
        return *this;
      }
      memcpy(data_, other.data(), size());
    }
    return *this;
  }

  /** Move constructor */
  charbuf(charbuf &&other) {
    alloc_ = other.alloc_;
    data_ = other.data_;
    size_ = other.size_;
    destructable_ = other.destructable_;
    other.destructable_ = false;
  }

  /** Move assignment operator */
  charbuf& operator=(charbuf &other) {
    if (this != &other) {
      Free();
      alloc_ = other.alloc_;
      data_ = other.data_;
      size_ = other.size_;
      destructable_ = other.destructable_;
      other.destructable_ = false;
    }
    return *this;
  }

  /** Reference data */
  char* data() {
    return data_;
  }

  /** Reference data */
  char* data() const {
    return data_;
  }

  /** Reference size */
  size_t size() const {
    return size_;
  }

  /**
 * Comparison operators
 * */

  int _strncmp(const char *a, size_t len_a,
               const char *b, size_t len_b) const {
    if (len_a != len_b) {
      return int((int64_t)len_a - (int64_t)len_b);
    }
    int sum = 0;
    for (size_t i = 0; i < len_a; ++i) {
      sum += a[i] - b[i];
    }
    return sum;
  }

#define LABSTOR_STR_CMP_OPERATOR(op) \
  bool operator op(const char *other) const { \
    return _strncmp(data(), size(), other, strlen(other)) op 0; \
  } \
  bool operator op(const std::string &other) const { \
    return _strncmp(data(), size(), other.data(), other.size()) op 0; \
  } \
  bool operator op(const charbuf &other) const { \
    return _strncmp(data(), size(), other.data(), other.size()) op 0; \
  }

  LABSTOR_STR_CMP_OPERATOR(==)
  LABSTOR_STR_CMP_OPERATOR(!=)
  LABSTOR_STR_CMP_OPERATOR(<)
  LABSTOR_STR_CMP_OPERATOR(>)
  LABSTOR_STR_CMP_OPERATOR(<=)
  LABSTOR_STR_CMP_OPERATOR(>=)

#undef LABSTOR_STR_CMP_OPERATOR

 private:
  /** Allocate charbuf */
  bool Allocate(lipc::Allocator *alloc, size_t size) {
    lipc::OffsetPointer p;
    if (size == 0) {
      alloc_ = nullptr;
      data_ = nullptr;
      size_ = 0;
      destructable_ = false;
      return false;
    }
    alloc_ = alloc;
    data_ = alloc->AllocatePtr<char>(size, p);
    size_ = size;
    destructable_ = true;
    return true;
  }

  /** Explicitly free the charbuf */
  void Free() {
    if (destructable_ && data_ && size_) {
      alloc_->FreePtr<char>(data_);
    }
  }
};

}  // namespace labstor

#endif //LABSTOR_INCLUDE_LABSTOR_TYPES_CHARBUF_H_
