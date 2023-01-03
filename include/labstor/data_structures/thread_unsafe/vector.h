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


#ifndef LABSTOR_DATA_STRUCTURES_LOCKLESS_VECTOR_H_
#define LABSTOR_DATA_STRUCTURES_LOCKLESS_VECTOR_H_

#include "labstor/data_structures/data_structure.h"
#include "labstor/data_structures/smart_ptr/manual_ptr.h"
#include "labstor/data_structures/internal/shm_ar.h"

#include <vector>

namespace labstor::ipc {

/** forward pointer for vector */
template<typename T>
class vector;

/**
 * The vector iterator implementation
 * */
template<typename T, bool FORWARD_ITER, bool CONST_ITER>
struct vector_iterator_templ {
 public:
  typedef SHM_CONST_T_OR_T(vector<T>, CONST_ITER) VecT_Const;

 public:
  VecT_Const *vec_;
  off64_t i_;

  /** Default constructor */
  vector_iterator_templ() = default;

  /** Construct an iterator */
  explicit vector_iterator_templ(VecT_Const *vec)
    : vec_(vec) {}

  /** Construct an iterator at \a i offset */
  explicit vector_iterator_templ(VecT_Const *vec, size_t i)
    : vec_(vec), i_(static_cast<off64_t>(i)) {}

  /** Copy constructor */
  vector_iterator_templ(const vector_iterator_templ &other)
  : vec_(other.vec_), i_(other.i_) {
  }

  /** Copy assignment operator  */
  vector_iterator_templ&
  operator=(const vector_iterator_templ &other) {
    if (this != &other) {
      vec_ = other.vec_;
      i_ = other.i_;
    }
    return *this;
  }

  /** Move constructor */
  vector_iterator_templ(vector_iterator_templ &&other) {
    vec_ = other.vec_;
    i_ = other.i_;
  }

  /** Move assignment operator  */
  vector_iterator_templ&
  operator=(vector_iterator_templ &&other) {
    if (this != &other) {
      vec_ = other.vec_;
      i_ = other.i_;
    }
    return *this;
  }

  /** Change the vector pointer */
  void change_pointer(VecT_Const *other) {
    vec_ = other;
  }

  /** Dereference the iterator */
  Ref<T> operator*() {
    if constexpr(!CONST_ITER) {
      return Ref<T>(vec_->data_ar()[i_].internal_ref());
    } else {
      return Ref<T>(vec_->data_ar_const()[i_].internal_ref());
    }
  }

  /** Dereference the iterator */
  const Ref<T> operator*() const {
    if constexpr(!CONST_ITER) {
      return Ref<T>(vec_->data_ar()[i_].internal_ref());
    } else {
      return Ref<T>(vec_->data_ar_const()[i_].internal_ref());
    }
  }

  /** Increment iterator in-place */
  vector_iterator_templ& operator++() {
    if (is_end()) { return *this; }
    if constexpr(FORWARD_ITER) {
      ++i_;
      if (i_ >= vec_->size()) {
        set_end();
      }
    } else {
      if (i_ == 0) {
        set_end();
      } else {
        --i_;
      }
    }
    return *this;
  }

  /** Decrement iterator in-place */
  vector_iterator_templ& operator--() {
    if (is_begin() || is_end()) { return *this; }
    if constexpr(FORWARD_ITER) {
      --i_;
    } else {
      ++i_;
    }
    return *this;
  }

  /** Create the next iterator */
  vector_iterator_templ operator++(int) const {
    vector_iterator_templ next_iter(*this);
    ++next_iter;
    return next_iter;
  }

  /** Create the prior iterator */
  vector_iterator_templ operator--(int) const {
    vector_iterator_templ prior_iter(*this);
    --prior_iter;
    return prior_iter;
  }

  /** Increment iterator by \a count and return */
  vector_iterator_templ operator+(size_t count) const {
    if (is_end()) { return end(); }
    if constexpr(FORWARD_ITER) {
      if (i_ + count > vec_->size()) {
        return end();
      }
      return vector_iterator_templ(vec_, i_ + count);
    } else {
      if (i_ < count - 1) {
        return end();
      }
      return vector_iterator_templ(vec_, i_ - count);
    }
  }

  /** Decrement iterator by \a count and return */
  vector_iterator_templ operator-(size_t count) const {
    if (is_end()) { return end(); }
    if constexpr(FORWARD_ITER) {
      if (i_ < count) {
        return begin();
      }
      return vector_iterator_templ(vec_, i_ - count);
    } else {
      if (i_ + count > vec_->size() - 1) {
        return begin();
      }
      return vector_iterator_templ(vec_, i_ + count);
    }
  }

  /** Increment iterator by \a count in-place */
  void operator+=(size_t count) {
    if (is_end()) { return end(); }
    if constexpr(FORWARD_ITER) {
      if (i_ + count > vec_->size()) {
        set_end();
        return;
      }
      i_ += count;
      return;
    } else {
      if (i_ < count - 1) {
        set_end();
        return;
      }
      i_ -= count;
      return;
    }
  }

  /** Decrement iterator by \a count in-place */
  void operator-=(size_t count) {
    if (is_end()) { return end(); }
    if constexpr(FORWARD_ITER) {
      if (i_ < count) {
        set_begin();
        return;
      }
      i_ -= count;
      return;
    } else {
      if (i_ + count > vec_->size() - 1) {
        set_begin();
        return;
      }
      i_ += count;
      return;
    }
  }

  /** Check if two iterators are equal */
  friend bool operator==(const vector_iterator_templ &a,
                         const vector_iterator_templ &b) {
    return (a.i_ == b.i_);
  }

  /** Check if two iterators are inequal */
  friend bool operator!=(const vector_iterator_templ &a,
                         const vector_iterator_templ &b) {
    return (a.i_ != b.i_);
  }

  /** Create the begin iterator */
  vector_iterator_templ const begin() {
    if constexpr(FORWARD_ITER) {
      return vector_iterator_templ(vec_, 0);
    } else {
      return vector_iterator_templ(vec_, vec_->size() - 1);
    }
  }

  /** Create the end iterator */
  static vector_iterator_templ const end() {
    static vector_iterator_templ end_iter(nullptr, -1);
    return end_iter;
  }

  /** Set this iterator to end */
  void set_end() {
    i_ = -1;
  }

  /** Set this iterator to begin */
  void set_begin() {
    if constexpr(FORWARD_ITER) {
      if (vec_->size() > 0) {
        i_ = 0;
      } else {
        set_end();
      }
    } else {
      i_ = vec_->size() - 1;
    }
  }

  /** Determine whether this iterator is the begin iterator */
  bool is_begin() const {
    if constexpr(FORWARD_ITER) {
      return (i_ == 0);
    } else {
      return (i_ == vec_->size() - 1);
    }
  }

  /** Determine whether this iterator is the end iterator */
  bool is_end() const {
    return i_ < 0;
  }
};

/** Forward iterator typedef */
template<typename T>
using vector_iterator = vector_iterator_templ<T, true, false>;

/** Backward iterator typedef */
template<typename T>
using vector_riterator = vector_iterator_templ<T, false, false>;

/** Constant forward iterator typedef */
template<typename T>
using vector_citerator = vector_iterator_templ<T, true, true>;

/** Backward iterator typedef */
template<typename T>
using vector_criterator = vector_iterator_templ<T, false, true>;

/**
 * MACROS used to simplify the vector namespace
 * Used as inputs to the SHM_CONTAINER_TEMPLATE
 * */
#define CLASS_NAME vector
#define TYPED_CLASS vector<T>

/**
 * The vector shared-memory header
 * */
template<typename T>
struct ShmHeader<TYPED_CLASS> : public ShmBaseHeader {
  Pointer vec_ptr_;
  size_t max_length_, length_;

  ShmHeader() = default;

  ShmHeader(const ShmHeader &other) {
    vec_ptr_ = other.vec_ptr_;
    max_length_ = other.max_length_;
    length_ = other.length_;
  }
};

/**
 * The vector class
 * */
template<typename T>
class vector : public SHM_CONTAINER(TYPED_CLASS) {
 public:
  BASIC_SHM_CONTAINER_TEMPLATE

 public:
  /** Default constructor */
  vector() = default;

  /** Construct the vector in shared memory */
  template<typename ...Args>
  void shm_init_main(ShmArchive<TYPED_CLASS> *ar,
                     Allocator *alloc, size_t length, Args&& ...args) {
    shm_init_header(ar, alloc);
    resize(length, std::forward<Args>(args)...);
  }

  /** Construct the vector in shared memory */
  void shm_init_main(ShmArchive<TYPED_CLASS> *ar,
                     Allocator *alloc) {
    shm_init_header(ar, alloc);
    header_->length_ = 0;
    header_->max_length_ = 0;
    header_->vec_ptr_.SetNull();
  }

  /** Copy from std::vector */
  void shm_init_main(ShmArchive<TYPED_CLASS> *ar,
                     Allocator *alloc, std::vector<T> &other) {
    shm_init_header(ar, alloc);
    reserve(other.size());
    for (auto &entry : other) {
      emplace_back(entry);
    }
  }

  /** Destroy all shared memory allocated by the vector */
  void shm_destroy(bool destroy_header = true) {
    SHM_DESTROY_DATA_START
    erase(begin(), end());
    alloc_->Free(header_->vec_ptr_);
    SHM_DESTROY_DATA_END
    SHM_DESTROY_END
  }

  /** Store into shared memory */
  void shm_serialize(ShmArchive<TYPED_CLASS> &ar) const {
    shm_serialize_header(ar.header_ptr_);
  }

  /** Load from shared memory */
  void shm_deserialize(const ShmArchive<TYPED_CLASS> &ar) {
    if(!shm_deserialize_header(ar.header_ptr_)) { return; }
  }

  /** Move constructor */
  void WeakMove(vector &other) {
    SHM_WEAK_MOVE_START(SHM_WEAK_MOVE_DEFAULT(TYPED_CLASS))
    *header_ = *(other.header_);
    SHM_WEAK_MOVE_END()
  }

  /** Copy a vector */
  void StrongCopy(const vector &other) {
    SHM_STRONG_COPY_START(SHM_STRONG_COPY_DEFAULT(TYPED_CLASS))
    reserve(other.size());
    for (auto iter = other.cbegin(); iter != other.cend(); ++iter) {
      emplace_back((**iter));
    }
    SHM_STRONG_COPY_END()
  }

  /**
   * Reserve space in the vector to emplace elements. Does not
   * change the size of the list.
   *
   * @param length the maximum size the vector can get before a growth occurs
   * @param args the arguments to construct
   * */
  template<typename ...Args>
  void reserve(size_t length, Args&& ...args) {
    if (IsNull()) { shm_init(); }
    if (length == 0) { return; }
    grow_vector(data_ar(), length, false, std::forward<Args>(args)...);
  }

  /**
   * Reserve space in the vector to emplace elements. Changes the
   * size of the list.
   *
   * @param length the maximum size the vector can get before a growth occurs
   * @param args the arguments used to construct the vector elements
   * */
  template<typename ...Args>
  void resize(size_t length, Args&& ...args) {
    if (IsNull()) { shm_init(); }
    if (length == 0) { return; }
    grow_vector(data_ar(), length, true, std::forward<Args>(args)...);
    header_->length_ = length;
  }

  /** Index the vector at position i */
  lipc::Ref<T> operator[](const size_t i) {
    shm_ar<T> *vec = data_ar();
    return lipc::Ref<T>(vec[i].internal_ref());
  }

  /** Index the vector at position i */
  const lipc::Ref<T> operator[](const size_t i) const {
    shm_ar<T> *vec = data_ar_const();
    return lipc::Ref<T>(vec[i].internal_ref());
  }

  /** Construct an element at the back of the vector */
  template<typename... Args>
  void emplace_back(Args&&... args) {
    shm_ar<T> *vec = data_ar();
    if (header_->length_ == header_->max_length_) {
      vec = grow_vector(vec, 0, false);
    }
    Allocator::ConstructObj<shm_ar<T>>(
      *(vec + header_->length_),
      std::forward<Args>(args)...);
    ++header_->length_;
  }

  /** Construct an element at an arbitrary position in the vector */
  template<typename ...Args>
  void emplace(vector_iterator<T> pos, Args&&... args) {
    if (pos.is_end()) {
      emplace_back(std::forward<Args>(args)...);
      return;
    }
    shm_ar<T> *vec = data_ar();
    if (header_->length_ == header_->max_length_) {
      vec = grow_vector(vec, 0, false);
    }
    shift_right(pos);
    Allocator::ConstructObj<shm_ar<T>>(
      *(vec + pos.i_),
      std::forward<Args>(args)...);
    ++header_->length_;
  }

  /** Delete the element at \a pos position */
  void erase(vector_iterator<T> pos) {
    if (pos.is_end()) return;
    shift_left(pos, 1);
    header_->length_ -= 1;
  }

  /** Delete elements between first and last  */
  void erase(vector_iterator<T> first, vector_iterator<T> last) {
    size_t last_i;
    if (first.is_end()) return;
    if (last.is_end()) {
      last_i = size();
    } else {
      last_i = last.i_;
    }
    size_t count = last_i - first.i_;
    if (count == 0) return;
    shift_left(first, count);
    header_->length_ -= count;
  }

  /** Delete all elements from the vector */
  void clear() {
    erase(begin(), end());
  }

  /** Get the size of the vector */
  size_t size() const {
    if (header_ == nullptr) {
      return 0;
    }
    return header_->length_;
  }

  /** Get the data in the vector */
  void* data() {
    if (header_ == nullptr) {
      return nullptr;
    }
    return alloc_->template
      Convert<void>(header_->vec_ptr_);
  }

  /** Get constant pointer to the data */
  void* data_const() const {
    if (header_ == nullptr) {
      return nullptr;
    }
    return alloc_->template
      Convert<void>(header_->vec_ptr_);
  }

  /**
   * Retreives a pointer to the array from the process-independent pointer.
   * */
  shm_ar<T>* data_ar() {
    return alloc_->template
      Convert<shm_ar<T>>(header_->vec_ptr_);
  }

  /**
   * Retreives a pointer to the array from the process-independent pointer.
   * */
  shm_ar<T>* data_ar_const() const {
    return alloc_->template
      Convert<shm_ar<T>>(header_->vec_ptr_);
  }

 private:
  /**
   * Grow a vector to a new size.
   *
   * @param vec the C-style array of elements to grow
   * @param max_length the new length of the vector. If 0, the current size
   * of the vector will be multiplied by a constant.
   * @param args the arguments used to construct the elements of the vector
   * */
  template<typename ...Args>
  shm_ar<T>* grow_vector(shm_ar<T> *vec, size_t max_length,
                    bool resize, Args&& ...args) {
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
    shm_ar<T> *new_vec;
    if (resize) {
      new_vec = alloc_->template
        ReallocateConstructObjs<shm_ar<T>>(
          header_->vec_ptr_,
          header_->length_,
          max_length,
          std::forward<Args>(args)...);
    } else {
      new_vec = alloc_->template
        ReallocateObjs<shm_ar<T>>(header_->vec_ptr_, max_length);
    }
    if (new_vec == nullptr) {
      throw OUT_OF_MEMORY.format("vector::emplace_back",
                                 max_length*sizeof(shm_ar<T>));
    }

    // Copy contents of old vector into new
    memcpy(new_vec, vec,
           header_->length_*sizeof(shm_ar<T>));

    // Update vector header
    header_->max_length_ = max_length;

    return new_vec;
  }

  /**
   * Shift every element starting at "pos" to the left by count. Any element
   * who would be shifted before "pos" will be deleted.
   *
   * @param pos the starting position
   * @param count the amount to shift left by
   * */
  void shift_left(const vector_iterator<T> pos, int count = 1) {
    shm_ar<T> *vec = data_ar();
    for (int i = 0; i < count; ++i) {
      Allocator::DestructObj<shm_ar<T>>(*(vec + pos.i_ + i));
    }
    auto dst = vec + pos.i_;
    auto src = dst + count;
    for (auto i = pos.i_ + count; i < size(); ++i) {
      memcpy(dst, src, sizeof(shm_ar<T>));
      dst += 1; src += 1;
    }
  }

  /**
   * Shift every element starting at "pos" to the right by count. Increases
   * the total number of elements of the vector by "count". Does not modify
   * the size parameter of the vector, this is done elsewhere.
   *
   * @param pos the starting position
   * @param count the amount to shift right by
   * */
  void shift_right(const vector_iterator<T> pos, int count = 1) {
    auto src = data_ar() + size() - 1;
    auto dst = src + count;
    auto sz = static_cast<off64_t>(size());
    for (auto i = sz - 1; i >= pos.i_; --i) {
      memcpy(dst, src, sizeof(shm_ar<T>));
      dst -= 1; src -= 1;
    }
  }


  /**
   * ITERATORS
   * */

 public:
  /** Beginning of the forward iterator */
  vector_iterator<T> begin() {
    if (size() == 0) { return end(); }
    vector_iterator<T> iter(this);
    iter.set_begin();
    return iter;
  }

  /** End of the forward iterator */
  static vector_iterator<T> const end() {
    return vector_iterator<T>::end();
  }

  /** Beginning of the constant forward iterator */
  vector_citerator<T> cbegin() const {
    if (size() == 0) { return cend(); }
    vector_citerator<T> iter(this);
    iter.set_begin();
    return iter;
  }

  /** End of the forward iterator */
  static const vector_citerator<T> cend() {
    return vector_citerator<T>::end();
  }

  /** Beginning of the reverse iterator */
  vector_riterator<T> rbegin() {
    if (size() == 0) { return rend(); }
    vector_riterator<T> iter(this);
    iter.set_begin();
    return iter;
  }

  /** End of the reverse iterator */
  static vector_riterator<T> const rend() {
    return vector_riterator<T>::end();
  }

  /** Beginning of the constant reverse iterator */
  vector_criterator<T> crbegin() {
    if (size() == 0) { return rend(); }
    vector_criterator<T> iter(this);
    iter.set_begin();
    return iter;
  }

  /** End of the constant reverse iterator */
  static vector_criterator<T> const crend() {
    return vector_criterator<T>::end();
  }
};

}  // namespace labstor::ipc

#undef CLASS_NAME
#undef TYPED_CLASS

#endif  // LABSTOR_DATA_STRUCTURES_LOCKLESS_VECTOR_H_
