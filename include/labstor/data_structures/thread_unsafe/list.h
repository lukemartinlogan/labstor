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


#ifndef LABSTOR_DATA_STRUCTURES_LOCKLESS_LIST_H_
#define LABSTOR_DATA_STRUCTURES_LOCKLESS_LIST_H_

#include "labstor/data_structures/data_structure.h"
#include "labstor/data_structures/smart_ptr/manual_ptr.h"
#include "labstor/data_structures/internal/shm_ar.h"

#include <list>

namespace labstor::ipc {

/** forward pointer for list */
template<typename T>
class list;

/** represents an object within a list */
template<typename T>
struct list_entry {
 public:
  typedef SHM_T_OR_ARCHIVE(T) T_Ar;

 public:
  Pointer next_ptr_, prior_ptr_;
  shm_ar<T> data_;

  /**
   * Constructor.
   * */
  template<typename ...Args>
  explicit list_entry(Args ...args) : data_(std::forward<Args>(args)...) {}

  /**
   * Returns the element stored in the list
   * */
  Ref<T> internal_ref() {
    return Ref<T>(data_.internal_ref());
  }
};

/**
 * The list iterator
 * */
template<typename T, bool CONST_ITER>
struct list_iterator_templ {
 public:
  typedef SHM_T_OR_ARCHIVE(T) T_Ar;
  typedef SHM_T_OR_REF_T(T) T_Ref;
  typedef SHM_CONST_T_OR_T(T_Ar, CONST_ITER) T_Ar_Const;
  typedef SHM_CONST_T_OR_T(T_Ref, CONST_ITER) T_Ref_Const;
  typedef SHM_CONST_T_OR_T(list<T>, CONST_ITER) ListT_Const;

 public:
  ListT_Const *list_;
  list_entry<T> *entry_;
  Pointer entry_ptr_;

  /** Default constructor */
  list_iterator_templ() = default;

  /** Construct an iterator  */
  explicit list_iterator_templ(ListT_Const *list) : list_(list) {}

  /** Construct an iterator  */
  explicit list_iterator_templ(ListT_Const *list,
                               list_entry<T> *entry,
                               Pointer entry_ptr)
    : list_(list), entry_(entry), entry_ptr_(entry_ptr) {}

  /** Copy constructor */
  list_iterator_templ(const list_iterator_templ &other) {
    list_ = other.list_;
    entry_ = other.entry_;
    entry_ptr_ = other.entry_ptr_;
  }

  /** Assign this iterator from another iterator */
  list_iterator_templ& operator=(const list_iterator_templ &other) {
    if (this != &other) {
      list_ = other.list_;
      entry_ = other.entry_;
      entry_ptr_ = other.entry_ptr_;
    }
    return *this;
  }

  /** Change the list pointer */
  void change_pointer(ListT_Const *other) {
    list_ = other;
  }

  /** Get the object the iterator points to */
  Ref<T> operator*() {
    return entry_->internal_ref();
  }

  /** Get the object the iterator points to */
  const Ref<T> operator*() const {
    return entry_->internal_ref();
  }

  /** Get the next iterator (in place) */
  list_iterator_templ& operator++() {
    if (is_end()) { return *this; }
    entry_ptr_ = entry_->next_ptr_;
    entry_ = list_->alloc_->template
      Convert<list_entry<T>>(entry_->next_ptr_);
    return *this;
  }

  /** Get the prior iterator (in place) */
  list_iterator_templ& operator--() {
    if (is_end() || is_begin()) { return *this; }
    entry_ptr_ = entry_->prior_ptr_;
    entry_ = list_->alloc_->template
      Convert<list_entry<T>>(entry_->prior_ptr_);
    return *this;
  }

  /** Return the next iterator */
  list_iterator_templ operator++(int) const {
    list_iterator_templ next_iter(*this);
    ++next_iter;
    return next_iter;
  }

  /** Return the prior iterator */
  list_iterator_templ operator--(int) const {
    list_iterator_templ prior_iter(*this);
    --prior_iter;
    return prior_iter;
  }

  /** Return the iterator at count after this one */
  list_iterator_templ operator+(size_t count) const {
    list_iterator_templ pos(*this);
    for (size_t i = 0; i < count; ++i) {
      ++pos;
    }
    return pos;
  }

  /** Return the iterator at count before this one */
  list_iterator_templ operator-(size_t count) const {
    list_iterator_templ pos(*this);
    for (size_t i = 0; i < count; ++i) {
      --pos;
    }
    return pos;
  }

  /** Get the iterator at count after this one (in-place) */
  void operator+=(size_t count) {
    list_iterator_templ pos = (*this) + count;
    entry_ = pos.entry_;
    entry_ptr_ = pos.entry_ptr_;
  }

  /** Get the iterator at count before this one (in-place) */
  void operator-=(size_t count) {
    list_iterator_templ pos = (*this) - count;
    entry_ = pos.entry_;
    entry_ptr_ = pos.entry_ptr_;
  }

  /** Determine if two iterators are equal */
  friend bool operator==(const list_iterator_templ &a,
                         const list_iterator_templ &b) {
    return (a.is_end() && b.is_end()) || (a.entry_ == b.entry_);
  }

  /** Determine if two iterators are inequal */
  friend bool operator!=(const list_iterator_templ &a,
                         const list_iterator_templ &b) {
    return !(a.is_end() && b.is_end()) && (a.entry_ != b.entry_);
  }

  /** Create the end iterator */
  static list_iterator_templ const end() {
    static list_iterator_templ end_iter(nullptr);
    return end_iter;
  }

  /** Determine whether this iterator is the end iterator */
  bool is_end() const {
    return list_ == nullptr;
  }

  /** Determine whether this iterator is the begin iterator */
  bool is_begin() const {
    if (list_ && entry_) {
      return entry_->prior_ptr_.IsNull();
    } else {
      return false;
    }
  }
};

/** forward iterator typedef */
template<typename T>
using list_iterator = list_iterator_templ<T, false>;

/** const forward iterator typedef */
template<typename T>
using list_citerator = list_iterator_templ<T, true>;


/**
 * MACROS used to simplify the list namespace
 * Used as inputs to the SHM_CONTAINER_TEMPLATE
 * */
#define CLASS_NAME list
#define TYPED_CLASS list<T>

/**
 * The list shared-memory header
 * */
template<typename T>
struct ShmHeader<TYPED_CLASS> : public ShmBaseHeader {
  Pointer head_ptr_, tail_ptr_;
  size_t length_;

  ShmHeader() = default;

  ShmHeader(const ShmHeader &other) {
    head_ptr_ = other.head_ptr_;
    tail_ptr_ = other.tail_ptr_;
    length_ = other.length_;
  }
};

/**
 * Doubly linked list implementation
 * */
template<typename T>
class list : public SHM_CONTAINER(TYPED_CLASS) {
 public:
  BASIC_SHM_CONTAINER_TEMPLATE

 public:
  typedef SHM_T_OR_ARCHIVE(T) T_Ar;
  typedef SHM_T_OR_REF_T(T) T_Ref;

 public:
  /** Default constructor */
  list() = default;

  /** Initialize list in shared memory */
  void shm_init_main(ShmArchive<TYPED_CLASS> *ar,
                     Allocator *alloc) {
    shm_init_header(ar, alloc);
    header_->length_ = 0;
    header_->head_ptr_.SetNull();
    header_->tail_ptr_.SetNull();
  }

  /** Copy from std::list */
  void shm_init_main(ShmArchive<TYPED_CLASS> *ar,
                     Allocator *alloc, std::list<T> &other) {
    shm_init_header(ar, alloc);
    for (auto &entry : other) {
      emplace_back(entry);
    }
  }

  /** Destroy all shared memory allocated by the list */
  void shm_destroy(bool destroy_header = true) {
    SHM_DESTROY_DATA_START
    clear();
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
  void WeakMove(list &other) {
    SHM_WEAK_MOVE_START(SHM_WEAK_MOVE_DEFAULT(TYPED_CLASS))
    *header_ = *(other.header_);
    SHM_WEAK_MOVE_END()
  }

  /** Copy constructor */
  void StrongCopy(const list &other) {
    SHM_STRONG_COPY_START(SHM_STRONG_COPY_DEFAULT(TYPED_CLASS))
    for (auto iter = other.cbegin(); iter != other.cend(); ++iter) {
      emplace_back(**iter);
    }
    SHM_STRONG_COPY_END();
  }

  /** Construct an element at the back of the list */
  template<typename... Args>
  inline void emplace_back(Args&&... args) {
    emplace(end(), std::forward<Args>(args)...);
  }

  /** Construct an element at the beginning of the list */
  template<typename... Args>
  inline void emplace_front(Args&&... args) {
    emplace(begin(), std::forward<Args>(args)...);
  }

  /** Construct an element at \a pos position in the list */
  template<typename ...Args>
  void emplace(list_iterator<T> pos, Args&&... args) {
    Pointer entry_ptr;
    auto entry = _create_entry(entry_ptr, std::forward<Args>(args)...);
    if (size() == 0) {
      entry->prior_ptr_.SetNull();
      entry->next_ptr_.SetNull();
      header_->head_ptr_ = entry_ptr;
      header_->tail_ptr_ = entry_ptr;
    } else if (pos.is_begin()) {
      entry->prior_ptr_.SetNull();
      entry->next_ptr_ = header_->head_ptr_;
      auto head = alloc_->template
        Convert<list_entry<T>>(header_->tail_ptr_);
      head->prior_ptr_ = entry_ptr;
      header_->head_ptr_ = entry_ptr;
    } else if (pos.is_end()) {
      entry->prior_ptr_ = header_->tail_ptr_;
      entry->next_ptr_.SetNull();
      auto tail = alloc_->template
        Convert<list_entry<T>>(header_->tail_ptr_);
      tail->next_ptr_ = entry_ptr;
      header_->tail_ptr_ = entry_ptr;
    } else {
      auto next = alloc_->template
        Convert<list_entry<T>>(pos.entry_->next_ptr_);
      auto prior = alloc_->template
        Convert<list_entry<T>>(pos.entry_->prior_ptr_);
      entry->next_ptr_ = pos.entry_->next_ptr_;
      entry->prior_ptr_ = pos.entry_->prior_ptr_;
      next->prior_ptr_ = entry_ptr;
      prior->next_ptr_ = entry_ptr;
    }
    ++header_->length_;
  }

  /** Erase the element at pos */
  void erase(list_iterator<T> pos) {
    erase(pos, pos+1);
  }

  /** Erase all elements between first and last */
  void erase(list_iterator<T> first,
             list_iterator<T> last) {
    if (first.is_end()) { return; }
    auto first_prior_ptr = first.entry_->prior_ptr_;
    auto pos = first;
    while (pos != last) {
      auto next = pos + 1;
      Allocator::DestructObj<list_entry<T>>(*pos.entry_);
      alloc_->Free(pos.entry_ptr_);
      --header_->length_;
      pos = next;
    }

    if (first_prior_ptr.IsNull()) {
      header_->head_ptr_ = last.entry_ptr_;
    } else {
      auto first_prior = alloc_->template
        Convert<list_entry<T>>(first_prior_ptr);
      first_prior->next_ptr_ = last.entry_ptr_;
    }

    if (last.entry_ptr_.IsNull()) {
      header_->tail_ptr_ = first_prior_ptr;
    } else {
      last.entry_->prior_ptr_ = first_prior_ptr;
    }
  }

  /** Destroy all elements in the list */
  void clear() {
    erase(begin(), end());
  }

  /** Get the object at the front of the list */
  inline Ref<T> front() {
    return *begin();
  }

  /** Get the object at the back of the list */
  inline Ref<T> back() {
    return *end();
  }

  /** Get the number of elements in the list */
  inline size_t size() const {
    if (!IsNull()) {
      return header_->length_;
    }
    return 0;
  }

  /**
   * ITERATORS
   * */

  /** Forward iterator begin */
  inline list_iterator<T> begin() {
    if (size() == 0) { return end(); }
    auto head = alloc_->template
      Convert<list_entry<T>>(header_->head_ptr_);
    return list_iterator<T>(this, head, header_->head_ptr_);
  }

  /** Forward iterator end */
  static list_iterator<T> const end() {
    return list_iterator<T>::end();
  }

  /** Constant forward iterator begin */
  inline list_citerator<T> cbegin() const {
    if (size() == 0) { return cend(); }
    auto head = alloc_->template
      Convert<list_entry<T>>(header_->head_ptr_);
    return list_citerator<T>(this, head, header_->head_ptr_);
  }

  /** Constant forward iterator end */
  static list_citerator<T> const cend() {
    return list_citerator<T>::end();
  }

 private:
  template<typename ...Args>
  inline list_entry<T>* _create_entry(Pointer &ptr, Args&& ...args) {
    auto entry = alloc_->template
      AllocateConstructObjs<list_entry<T>>(1, ptr, std::forward<Args>(args)...);
    return entry;
  }
};

}  // namespace labstor::ipc

#undef CLASS_NAME
#undef TYPED_CLASS

#endif  // LABSTOR_DATA_STRUCTURES_LOCKLESS_LIST_H_
