//
// Created by lukemartinlogan on 11/6/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_LOCKLESS_VECTOR_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_LOCKLESS_VECTOR_H_

#include <labstor/data_structures/data_structure.h>

namespace labstor::ipc {

template<typename T>
class vector;

template<typename T>
class ShmArchive<vector<T>> {
  Pointer header_ptr_;
};

template<typename T>
class ShmHeader<vector<T>> {
  Pointer vec_ptr_;
  size_t max_length_, length_;
};

}

namespace labstor::ipc::lockless {

template<typename T>
class vector : public ShmDataStructure<vector<T>> {
 private:
  typedef SHM_T_OR_ARCHIVE(T) T_Ar;
  typedef SHM_T_OR_REF_T(T) T_Ret;
  using ShmDataStructure<vector<T>>::header_;
  using ShmDataStructure<vector<T>>::header_ptr_;
  using ShmDataStructure<vector<T>>::mem_mngr_;
  using ShmDataStructure<vector<T>>::alloc_;

 public:
  vector() {}

  vector(size_t length) {
    Create(length);
  }

  vector(size_t length, Allocator *alloc) :
      ShmDataStructure<vector<T>>(alloc) {
    Create(length);
  }

  vector(size_t length, allocator_id_t alloc_id) :
      ShmDataStructure<vector<T>>(alloc_id) {
    Create(length);
  }

  void Create(size_t length) {
    if (length > 0) {
      header_->vec_ptr_ = alloc_->Allocate(length * sizeof(T));
    }
  }

  void shm_serialize(ShmArchive<vector<T>> &ar) {
    ar.header_ptr_ = header_ptr_;
  }

  void shm_deserialize(ShmArchive<vector<T>> &ar) {
    header_ = mem_mngr_->Convert(ar.header_ptr_);
  }

  T_Ret operator[](const size_t i) {
    if (header_->vec_ptr_ != )
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      T obj;
      obj << vec_[i];
      return obj;
    } else {
      return vec_[i];
    }
  }

  template<typename... Args>
  void emplace_back(Args&&... args) {
    if (header_->length_ == header_->max_length_) {
      throw ARRAY_OUT_OF_BOUNDS.format("vector::emplace_back");
    }
    vec_[header_->length_++] = std::move(T(args...));
  }

  size_t size() {
    return header_->length_;
  }
};

}

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_LOCKLESS_VECTOR_H_
