//
// Created by lukemartinlogan on 11/6/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_LOCKLESS_VECTOR_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_LOCKLESS_VECTOR_H_

#include <labstor/data_structures/data_structure.h>

namespace labstor::ipc::lockless {
template<typename T>
class vector;
}  // namespace labstor::ipc::lockless

namespace labstor::ipc {

template<typename T>
struct ShmArchive<lockless::vector<T>> {
  Pointer header_ptr_;
};

template<typename T>
struct ShmHeader<lockless::vector<T>> {
  Pointer vec_ptr_;
  size_t max_length_, length_;
};

}  // namespace labstor::ipc

namespace labstor::ipc::lockless {

template<typename T>
struct vector_iterator {
  vector<T> &vec_;
  size_t i_;

  explicit vector_iterator(vector<T> &vec) : vec_(vec), i_(0) {}
  explicit vector_iterator(vector<T> &vec, size_t i) : vec_(vec), i_(i) {}

  T& operator*() const { return vec_[i_]; }
  T* operator->() const { return &vec_[i_]; }

  vector_iterator& operator++() {
    ++i_;
    return *this;
  }

  vector_iterator& operator--() {
    --i_;
    return *this;
  }

  vector_iterator operator++(int) const {
    return vector_iterator(vec_, i_ + 1);
  }

  vector_iterator operator--(int) const {
    return vector_iterator(vec_, i_ - 1);
  }

  vector_iterator operator+(size_t i) const {
    if (i_ > vec_.size()) {
      return vec_.end();
    }
    return vector_iterator(vec_, i_ + i);
  }

  vector_iterator operator-(size_t i) const {
    return vector_iterator(vec_, i_ - i);
  }

  friend bool operator==(const vector_iterator &a, const vector_iterator &b) {
    return a.i_ == b.i_;
  }

  friend bool operator!=(const vector_iterator &a, const vector_iterator &b) {
    return a.i_ != b.i_;
  }
};

template<typename T>
class vector : public ShmDataStructure<vector<T>> {
 SHM_DATA_STRUCTURE_TEMPLATE(vector, vector<T>)

 private:
  typedef SHM_T_OR_ARCHIVE(T) T_Ar;
  typedef SHM_T_OR_REF_T(T) T_Ret;

 public:
  vector() = default;

  explicit vector(Allocator *alloc) : ShmDataStructure<vector<T>>(alloc) {}

  explicit vector(size_t length) {
    reserve(length);
  }

  explicit vector(size_t length, Allocator *alloc) :
      ShmDataStructure<vector<T>>(alloc) {
    reserve(length);
  }

  explicit vector(size_t length, allocator_id_t alloc_id) :
      ShmDataStructure<vector<T>>(alloc_id) {
    reserve(length);
  }

  void destroy() {
    erase(begin(), end());
    alloc_->Free(header_->vec_ptr_);
  }

  void shm_serialize(ShmArchive<vector<T>> &ar) {
    ar.header_ptr_ = header_ptr_;
  }

  void shm_deserialize(ShmArchive<vector<T>> &ar) {
    InitDataStructure(ar.header_ptr_);
  }

  void reserve(size_t length) {
    if (header_ == nullptr) {
      header_ = alloc_->template
        AllocateObj<ShmHeader<vector<T>>>(1, header_ptr_);
      header_->length_ = 0;
      header_->max_length_ = 0;
    }
    grow_vector(_data(), length);
  }

  void resize(size_t length) {
    reserve(length);
    header_->length_ = length;
  }

  T_Ret operator[](const size_t i) {
    T_Ar *vec = _data();
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      T obj;
      obj << vec[i];
      return obj;
    } else {
      return vec[i];
    }
  }

  template<typename... Args>
  void emplace_back(Args&&... args) {
    T_Ar *vec = _data();
    if (header_->length_ == header_->max_length_) {
      vec = grow_vector(vec);
    }
    _construct<T, T_Ar>(*(vec + header_->length_), args...);
    ++header_->length_;
  }

  template<typename ...Args>
  void emplace(const vector_iterator<T> pos, Args&&... args) {
    if (pos == end()) {
      emplace_back(args...);
      return;
    }
    T_Ar *vec = _data();
    if (header_->length_ == header_->max_length_) {
      vec = grow_vector(vec);
    }
    shift_right(pos);
    _construct<T, T_Ar>(*pos, args...);
    ++header_->length_;
  }

  void erase(vector_iterator<T> first, vector_iterator<T> last) {
    size_t count = last.i_ - first.i_;
    if (count == 0) return;
    shift_left(first, count);
    header_->length_ -= count;
  }

  size_t size() {
    return header_->length_;
  }

  void* data() {
    return mem_mngr_->template
      Convert<void>(header_->vec_ptr_);
  }

  T_Ar* _data() {
    return mem_mngr_->template
      Convert<T_Ar>(header_->vec_ptr_);
  }

  /**
   * ITERATORS
   * */

  vector_iterator<T> begin() {
    return vector_iterator<T>(*this);
  }

  vector_iterator<T> end() {
    return vector_iterator<T>(*this, size());
  }

  vector_iterator<T> rbegin() {
  }

  vector_iterator<T> rend() {
  }

  vector_iterator<T> cbegin() {
  }

  vector_iterator<T> cend() {
  }

  vector_iterator<T> crbegin() {
  }

  vector_iterator<T> crend() {
  }

 private:
  T_Ar* grow_vector(T_Ar *vec, size_t max_length = 0) {
    Pointer new_ptr;

    // Grow vector by 25%
    if (max_length == 0) {
      max_length = 5 * header_->max_length_ / 4;
      if (max_length <= header_->max_length_ + 10) {
        max_length += 10;
      }
    }
    if (max_length < header_->max_length_) {
      return nullptr;
    }

    // Allocate new shared-memory vec
    T_Ar *new_vec = alloc_->template
      ConstructObj<T_Ar>(max_length, new_ptr);
    if (new_vec == nullptr) {
      throw OUT_OF_MEMORY.format("vector::emplace_back",
                                 max_length*sizeof(T_Ar));
    }

    // Copy contents of old vector into new
    memcpy(new_vec, vec,
           header_->length_*sizeof(T_Ar));

    // Update vector header
    header_->max_length_ = max_length;
    header_->vec_ptr_ = new_ptr;

    return new_vec;
  }

  void shift_left(const vector_iterator<T> pos, int count = 1) {
    for (int i = 0; i < count; ++i) { _destruct<T, T_Ar>(*(pos + i)); }
    for (auto i = pos + count; i != end(); ++i) {
      *(i - count) = (*i);
    }
  }

  void shift_right(const vector_iterator<T> pos, int count = 1) {
    auto pos_left = pos - 1;
    for (auto i = end() - 1; i != pos_left; --i) {
      *(i + count) = (*i);
    }
  }
};

}  // namespace labstor::ipc::lockless

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_LOCKLESS_VECTOR_H_
