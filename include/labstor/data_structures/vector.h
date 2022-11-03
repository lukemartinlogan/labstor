//
// Created by lukemartinlogan on 11/1/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_VECTOR_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_VECTOR_H_

#include <labstor/memory/memory.h>
#include "data_structure.h"

namespace labstor {
template<typename T>
class vector_iter;

template<typename T>
class vector;
}  // namespace labstor::memory


namespace labstor {

template<typename T>
class ShmArchive<vector<T>> {
  memory::Pointer header_ptr_;
};

template<typename T>
class ShmHeader<vector<T>> {
  size_t length_;
  size_t max_length_;
};

}  // namespace labstor


namespace labstor {

template<typename T>
struct vector_iter {
  vector<T> &vec_;
  size_t i_;

  explicit vector_iter(vector<T> &vec) : vec_(vec), i_(0) {}
  explicit vector_iter(vector<T> &vec, size_t i) : vec_(vec), i_(i) {}

  T& operator*() const { return vec_[i_]; }
  T* operator->() const { return &vec_[i_]; }

  vector_iter& operator++() {
    ++i_;
    return *this;
  }

  vector_iter operator++(int) {
    return vector_iter(vec_, i_ + 1);
  }

  vector_iter operator+(size_t i) {
    if (i_ > vec_.size()) {
      return vec_.end();
    }
    return vector_iter(vec_, i_ + i);
  }

  friend bool operator==(const vector_iter &a, const vector_iter &b) {
    return a.i_ == b.i_;
  }

  friend bool operator!=(const vector_iter &a, const vector_iter &b) {
    return a.i_ != b.i_;
  }
};

template <typename T>
class vector : public ShmDataStructure<vector<T>> {
 private:
  typedef SHM_PARAM(T) T_Ar;
  typedef SHM_RET(T) T_Ret;

 private:
  T_Ar *vec_;

 public:
  vector(memory::Allocator *alloc) :
    ShmDataStructure<vector<T>>(alloc), vec_(nullptr) {}

  void shm_serialize(ShmArchive<vector<T>> &ar) {
    ar.header_ptr_ = header_ptr_;
  }

  void shm_deserialize(ShmArchive<vector<T>>) {
    header_ptr_ = ar.header_ptr_;
    header_ = mem_mngr_->Convert<ShmHeader<vector<T>>(header_ptr);
  }

  // Index should return either T& or T
  T_Ret operator[](const size_t i) {
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      T obj;
      obj.shm_deserialize(vec_[i]);
      return obj;
    } else {
      return vec_[i];
    }
  }

  template<typename... Args>
  void emplace_back(Args&&... args) {
    if (header_->length_ == header_->max_length_) {
      throw ARRAY_OUT_OF_BOUNDS.format("array::emplace_back");
    }
    vec_[header_->length_++] = std::move(T(args...));
  }

  size_t size() {
    return header_->length_;
  }
};

}  // namespace labstor

#endif  // LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_VECTOR_H_
