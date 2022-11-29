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

template<typename T, typename T_Ref, bool FORWARD_ITER>
struct vector_iterator_templ {
  vector<T> &vec_;
  off64_t i_;

  explicit vector_iterator_templ(vector<T> &vec, size_t i) :
    vec_(vec), i_(static_cast<off64_t>(i)) {}

  T_Ref operator*() const { return vec_[i_]; }

  vector_iterator_templ& operator++() {
    if constexpr(FORWARD_ITER) {
      ++i_;
    } else {
      --i_;
    }
    return *this;
  }

  vector_iterator_templ& operator--() {
    if constexpr(FORWARD_ITER) {
      --i_;
    } else {
      ++i_;
    }
    return *this;
  }

  vector_iterator_templ operator++(int) const {
    if constexpr(FORWARD_ITER) {
      return vector_iterator_templ(vec_, i_ + 1);
    } else {
      return vector_iterator_templ(vec_, i_ - 1);
    }
  }

  vector_iterator_templ operator--(int) const {
    if constexpr(FORWARD_ITER) {
      return vector_iterator_templ(vec_, i_ - 1);
    } else {
      return vector_iterator_templ(vec_, i_ + 1);
    }
  }

  vector_iterator_templ operator+(size_t count) const {
    if constexpr(FORWARD_ITER) {
      if (i_ + count > vec_.size()) {
        return vector_iterator_templ(vec_, vec_.size());
      }
      return vector_iterator_templ(vec_, i_ + count);
    } else {
      if (i_ < count - 1) {
        return vector_iterator_templ(vec_, -1);
      }
      return vector_iterator_templ(vec_, i_ - count);
    }
  }

  vector_iterator_templ operator-(size_t count) const {
    if constexpr(FORWARD_ITER) {
      if (i_ < count) {
        return vector_iterator_templ(vec_, 0);
      }
      return vector_iterator_templ(vec_, i_ - count);
    } else {
      if (i_ + count > vec_.size() - 1) {
        return vector_iterator_templ(vec_, vec_.size() - 1);
      }
      return vector_iterator_templ(vec_, i_ + count);
    }
  }

  void operator+=(size_t count) {
    if constexpr(FORWARD_ITER) {
      if (i_ + count > vec_.size()) {
        i_ = vec_.size();
        return;
      }
      i_ += count;
      return;
    } else {
      if (i_ < count - 1) {
        i_ = 0;
        return;
      }
      i_ -= count;
      return;
    }
  }

  void operator-=(size_t count) {
    if constexpr(FORWARD_ITER) {
      if (i_ < count) {
        i_ = 0;
        return;
      }
      i_ -= count;
      return;
    } else {
      if (i_ + count > vec_.size() - 1) {
        i_ = vec_.size();
        return;
      }
      i_ += count;
      return;
    }
  }

  friend bool operator==(const vector_iterator_templ &a,
                         const vector_iterator_templ &b) {
    return a.i_ == b.i_;
  }

  friend bool operator!=(const vector_iterator_templ &a,
                         const vector_iterator_templ &b) {
    return a.i_ != b.i_;
  }
};

template<typename T, typename T_Ref>
using vector_iterator = vector_iterator_templ<T, T_Ref, true>;
template<typename T, typename T_Ref>
using vector_riterator = vector_iterator_templ<T, T_Ref, false>;

template<typename T>
class vector : public ShmDataStructure<vector<T>> {
 SHM_DATA_STRUCTURE_TEMPLATE(vector, vector<T>)

 private:
  typedef SHM_T_OR_ARCHIVE(T) T_Ar;
  typedef SHM_T_OR_REF_T(T) T_Ref;

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

  void shm_destroy() {
    erase(begin(), end());
    alloc_->Free(header_->vec_ptr_);
    alloc_->Free(header_ptr_);
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
        AllocateObjs<ShmHeader<vector<T>>>(1, header_ptr_);
      header_->length_ = 0;
      header_->max_length_ = 0;
      header_->vec_ptr_ = kNullPointer;
    }
    grow_vector(_data(), length);
  }

  void resize(size_t length) {
    reserve(length);
    header_->length_ = length;
  }

  T_Ref operator[](const size_t i) {
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
  void emplace(vector_iterator<T, T_Ref> pos, Args&&... args) {
    if (pos == end()) {
      emplace_back(args...);
      return;
    }
    T_Ar *vec = _data();
    if (header_->length_ == header_->max_length_) {
      vec = grow_vector(vec);
    }
    shift_right(pos);
    _construct<T, T_Ar>(*(vec + pos.i_), args...);
    ++header_->length_;
  }

  void erase(vector_iterator<T, T_Ref> first, vector_iterator<T, T_Ref> last) {
    size_t count = last.i_ - first.i_;
    if (count == 0) return;
    shift_left(first, count);
    header_->length_ -= count;
  }

  size_t size() const {
    return header_->length_;
  }

  void* data() {
    return mem_mngr_->template
      Convert<void>(header_->vec_ptr_);
  }

 private:
  T_Ar* grow_vector(T_Ar *vec, size_t max_length = 0) {
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
      ReallocateConstructObjs<T_Ar>(header_->vec_ptr_,
                              header_->max_length_, max_length);
    if (new_vec == nullptr) {
      throw OUT_OF_MEMORY.format("vector::emplace_back",
                                 max_length*sizeof(T_Ar));
    }

    // Copy contents of old vector into new
    memcpy(new_vec, vec,
           header_->length_*sizeof(T_Ar));

    // Update vector header
    header_->max_length_ = max_length;

    return new_vec;
  }

  void shift_left(const vector_iterator<T, T_Ref> pos, int count = 1) {
    T_Ar *vec = _data();
    for (int i = 0; i < count; ++i) {
      _destruct<T, T_Ar>(*(vec + pos.i_ + i));
    }
    auto dst = vec + pos.i_;
    auto src = dst + count;
    for (auto i = pos.i_ + count; i < size(); ++i) {
      memcpy(dst, src, sizeof(T_Ar));
      dst += 1; src += 1;
    }
  }

  void shift_right(const vector_iterator<T, T_Ref> pos, int count = 1) {
    auto src = _data() + size() - 1;
    auto dst = src + count;
    auto sz = static_cast<off64_t>(size());
    for (auto i = sz - 1; i >= pos.i_; --i) {
      memcpy(dst, src, sizeof(T_Ar));
      dst -= 1; src -= 1;
    }
  }

  T_Ar* _data() {
    return mem_mngr_->template
      Convert<T_Ar>(header_->vec_ptr_);
  }


  /**
   * ITERATORS
   * */

 public:
  vector_iterator<T, T_Ref> begin() {
    return vector_iterator<T, T_Ref>(*this, 0);
  }

  vector_iterator<T, T_Ref> end() {
    return vector_iterator<T, T_Ref>(*this, size());
  }

  vector_riterator<T, T_Ref> rbegin() {
    return vector_riterator<T, T_Ref>(*this, size() - 1);
  }

  vector_riterator<T, T_Ref> rend() {
    return vector_riterator<T, T_Ref>(*this, -1);
  }
};

}  // namespace labstor::ipc::lockless

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_LOCKLESS_VECTOR_H_
