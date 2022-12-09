//
// Created by lukemartinlogan on 11/6/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_LOCKLESS_VECTOR_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_LOCKLESS_VECTOR_H_

#include "labstor/data_structures/data_structure.h"

namespace labstor::ipc {

/** forward pointer for vector */
template<typename T>
class vector;

/**
 * The vector shared-memory header
 * */
template<typename T>
struct vector_header {
  Pointer vec_ptr_;
  size_t max_length_, length_;
};

/**
 * The vector iterator implementation
 * */
template<typename T, bool FORWARD_ITER>
struct vector_iterator_templ {
 public:
  typedef SHM_T_OR_ARCHIVE(T) T_Ar;
  typedef SHM_T_OR_REF_T(T) T_Ref;

 public:
  vector<T> *vec_;
  off64_t i_;

  /** Default constructor */
  vector_iterator_templ() = default;

  /** Construct an iterator at \a i offset */
  explicit vector_iterator_templ(vector<T> &vec, size_t i)
    : vec_(&vec), i_(static_cast<off64_t>(i)) {}

  /** Dereference the iterator */
  T_Ref operator*() const { return (*vec_)[i_]; }

  /** Increment iterator in-place */
  vector_iterator_templ& operator++() {
    if constexpr(FORWARD_ITER) {
      ++i_;
    } else {
      --i_;
    }
    return *this;
  }

  /** Decrement iterator in-place */
  vector_iterator_templ& operator--() {
    if constexpr(FORWARD_ITER) {
      --i_;
    } else {
      ++i_;
    }
    return *this;
  }

  /** Create the next iterator */
  vector_iterator_templ operator++(int) const {
    if constexpr(FORWARD_ITER) {
      return vector_iterator_templ((*vec_), i_ + 1);
    } else {
      return vector_iterator_templ((*vec_), i_ - 1);
    }
  }

  /** Create the prior iterator */
  vector_iterator_templ operator--(int) const {
    if constexpr(FORWARD_ITER) {
      return vector_iterator_templ((*vec_), i_ - 1);
    } else {
      return vector_iterator_templ((*vec_), i_ + 1);
    }
  }

  /** Increment iterator by \a count and return */
  vector_iterator_templ operator+(size_t count) const {
    if constexpr(FORWARD_ITER) {
      if (i_ + count > (*vec_).size()) {
        return vector_iterator_templ((*vec_), (*vec_).size());
      }
      return vector_iterator_templ((*vec_), i_ + count);
    } else {
      if (i_ < count - 1) {
        return vector_iterator_templ((*vec_), -1);
      }
      return vector_iterator_templ((*vec_), i_ - count);
    }
  }

  /** Decrement iterator by \a count and return */
  vector_iterator_templ operator-(size_t count) const {
    if constexpr(FORWARD_ITER) {
      if (i_ < count) {
        return vector_iterator_templ((*vec_), 0);
      }
      return vector_iterator_templ((*vec_), i_ - count);
    } else {
      if (i_ + count > (*vec_).size() - 1) {
        return vector_iterator_templ((*vec_), (*vec_).size() - 1);
      }
      return vector_iterator_templ((*vec_), i_ + count);
    }
  }

  /** Increment iterator by \a count in-place */
  void operator+=(size_t count) {
    if constexpr(FORWARD_ITER) {
      if (i_ + count > (*vec_).size()) {
        i_ = (*vec_).size();
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

  /** Decrement iterator by \a count in-place */
  void operator-=(size_t count) {
    if constexpr(FORWARD_ITER) {
      if (i_ < count) {
        i_ = 0;
        return;
      }
      i_ -= count;
      return;
    } else {
      if (i_ + count > (*vec_).size() - 1) {
        i_ = (*vec_).size();
        return;
      }
      i_ += count;
      return;
    }
  }

  /** Assign one iterator into another  */
  vector_iterator_templ&
  operator=(const vector_iterator_templ &other) {
    if (this != &other) {
      vec_ = other.vec_;
      i_ = other.i_;
    }
    return *this;
  }

  /** Check if two iterators are equal */
  friend bool operator==(const vector_iterator_templ &a,
                         const vector_iterator_templ &b) {
    return a.i_ == b.i_;
  }

  /** Check if two iterators are inequal */
  friend bool operator!=(const vector_iterator_templ &a,
                         const vector_iterator_templ &b) {
    return a.i_ != b.i_;
  }
};

/** Forward iterator typedef */
template<typename T>
using vector_iterator = vector_iterator_templ<T, true>;

/** Backward iterator typedef */
template<typename T>
using vector_riterator = vector_iterator_templ<T, false>;

/**
 * MACROS to simplify the vector namespace
 * */
#define CLASS_NAME vector
#define TYPED_CLASS vector<T>
#define TYPED_HEADER vector_header<T>

/**
 * The vector class
 * */
template<typename T>
class vector : public ShmDataStructure<TYPED_CLASS, TYPED_HEADER> {
 public:
  SHM_DATA_STRUCTURE_TEMPLATE(CLASS_NAME, TYPED_CLASS, TYPED_HEADER)

 public:
  typedef SHM_T_OR_ARCHIVE(T) T_Ar;
  typedef SHM_T_OR_REF_T(T) T_Ref;

 public:
  vector() = default;

  /** Default shared-memory constructor */
  explicit vector(Allocator *alloc)
  : ShmDataStructure<TYPED_CLASS, TYPED_HEADER>(alloc) {}

  /**
   * Construct a vector of a certain length in shared memory
   *
   * @param length the size the vector should be
   * @param alloc the allocator to reserve memory from
   * @param args the parameters of the elements to construct
   * */
  template<typename ...Args>
  explicit vector(size_t length, Allocator *alloc = nullptr, Args ...args)
  : ShmDataStructure<TYPED_CLASS, TYPED_HEADER>(alloc) {
    resize(length, args...);
  }

  /**
   * Construct a vector of a certain length in shared memory
   *
   * @param length the size the vector should be
   * @param alloc_id the id of the allocator to reserve memory from
   * @param args the parameters of the elements to construct
   * */
  template<typename ...Args>
  explicit vector(size_t length, allocator_id_t alloc_id, Args ...args)
  : ShmDataStructure<TYPED_CLASS, TYPED_HEADER>(alloc_id) {
    resize(length, args...);
  }

  /** Construct the vector in shared memory */
  void shm_init() {
    header_ = alloc_->template
      AllocateObjs<TYPED_HEADER>(1, header_ptr_);
    header_->length_ = 0;
    header_->max_length_ = 0;
    header_->vec_ptr_.set_null();
  }

  /** Destroy all shared memory allocated by the vector */
  void shm_destroy() {
    erase(begin(), end());
    alloc_->Free(header_->vec_ptr_);
    alloc_->Free(header_ptr_);
  }

  /**
   * Reserve space in the vector to emplace elements. Does not
   * change the size of the list.
   *
   * @param length the maximum size the vector can get before a growth occurs
   * @param args the arguments to construct
   * */
  template<typename ...Args>
  void reserve(size_t length, Args ...args) {
    if (header_ == nullptr) {
      shm_init();
    }
    grow_vector(_data(), length, args...);
  }

  /**
   * Reserve space in the vector to emplace elements. Changes the
   * size of the list.
   *
   * @param length the maximum size the vector can get before a growth occurs
   * @param args the arguments used to construct the vector elements
   * */
  template<typename ...Args>
  void resize(size_t length, Args ...args) {
    reserve(length, args...);
    header_->length_ = length;
  }

  /** Index the vector at position i */
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

  /** Construct an element at the back of the vector */
  template<typename... Args>
  void emplace_back(Args&&... args) {
    T_Ar *vec = _data();
    if (header_->length_ == header_->max_length_) {
      vec = grow_vector(vec, 0);
    }
    Allocator::ConstructObj<T>(*(vec + header_->length_), args...);
    ++header_->length_;
  }

  /** Construct an element at an arbitrary position in the vector */
  template<typename ...Args>
  void emplace(vector_iterator<T> pos, Args&&... args) {
    if (pos == end()) {
      emplace_back(args...);
      return;
    }
    T_Ar *vec = _data();
    if (header_->length_ == header_->max_length_) {
      vec = grow_vector(vec, 0);
    }
    shift_right(pos);
    Allocator::ConstructObj<T, T_Ar>(*(vec + pos.i_), args...);
    ++header_->length_;
  }

  /** Delete elements between first and last  */
  void erase(vector_iterator<T> first, vector_iterator<T> last) {
    size_t count = last.i_ - first.i_;
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
    return mem_mngr_->template
      Convert<void>(header_->vec_ptr_);
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
  T_Ar* grow_vector(T_Ar *vec, size_t max_length, Args ...args) {
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
      ReallocateConstructObjs<T, T_Ar>(header_->vec_ptr_,
                              header_->max_length_,
                              max_length,
                              args...);
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

  /**
   * Shift every element starting at "pos" to the left by count. Any element
   * who would be shifted before "pos" will be deleted.
   *
   * @param pos the starting position
   * @param count the amount to shift left by
   * */
  void shift_left(const vector_iterator<T> pos, int count = 1) {
    T_Ar *vec = _data();
    for (int i = 0; i < count; ++i) {
      Allocator::DestructObj<T, T_Ar>(*(vec + pos.i_ + i));
    }
    auto dst = vec + pos.i_;
    auto src = dst + count;
    for (auto i = pos.i_ + count; i < size(); ++i) {
      memcpy(dst, src, sizeof(T_Ar));
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
    auto src = _data() + size() - 1;
    auto dst = src + count;
    auto sz = static_cast<off64_t>(size());
    for (auto i = sz - 1; i >= pos.i_; --i) {
      memcpy(dst, src, sizeof(T_Ar));
      dst -= 1; src -= 1;
    }
  }


  /**
   * Retreives a pointer to the array from the process-independent pointer.
   * */
  T_Ar* _data() {
    return alloc_->template
      Convert<T_Ar>(header_->vec_ptr_);
  }


  /**
   * ITERATORS
   * */

 public:

  /** Beginning of the forward iterator */
  vector_iterator<T> begin() {
    return vector_iterator<T>(*this, 0);
  }

  /** End of the forward iterator */
  vector_iterator<T> end() {
    return vector_iterator<T>(*this, size());
  }

  /** Beginning of the reverse iterator */
  vector_riterator<T> rbegin() {
    return vector_riterator<T>(*this, size() - 1);
  }

  /** End of the reverse iterator */
  vector_riterator<T> rend() {
    return vector_riterator<T>(*this, -1);
  }
};

}  // namespace labstor::ipc

#undef CLASS_NAME
#undef TYPED_CLASS
#undef TYPED_HEADER

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_LOCKLESS_VECTOR_H_
