//
// Created by lukemartinlogan on 11/28/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_LOCKLESS_LIST_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_LOCKLESS_LIST_H_

#include "labstor/data_structures/data_structure.h"

namespace labstor::ipc {

template<typename T>
class list;

template<typename T>
struct list_entry {
 private:
  typedef SHM_T_OR_ARCHIVE(T) T_Ar;
  typedef SHM_T_OR_REF_T(T) T_Ref;

 public:
  Pointer next_ptr_, prior_ptr_;
  T_Ar data_;

  T_Ref data() {
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      T obj;
      obj << data_;
      return obj;
    } else {
      return data_;
    }
  }
};

template<typename T>
struct list_header {
  Pointer head_ptr_, tail_ptr_;
  size_t length_;
};

template<typename T>
struct list_iterator {
 private:
  typedef SHM_T_OR_ARCHIVE(T) T_Ar;
  typedef SHM_T_OR_REF_T(T) T_Ref;

 public:
  list<T> &list_;
  list_entry<T> *entry_;
  Pointer entry_ptr_;

  explicit list_iterator(list<T> &list,
                         list_entry<T> *entry,
                         Pointer entry_ptr) :
    list_(list), entry_(entry), entry_ptr_(std::move(entry_ptr)) {}

  T_Ref operator*() const {
    return entry_->data();
  }

  list_iterator& operator++() {
    entry_ptr_ = entry_->next_ptr_;
    entry_ = list_.alloc_->template
      Convert<list_entry<T>>(entry_->next_ptr_);
    return *this;
  }

  list_iterator& operator--() {
    return *this;
  }

  list_iterator operator++(int) const {
    if (entry_ == nullptr) { return (*this); }
    auto next_entry = LABSTOR_MEMORY_MANAGER->template
      Convert<list_entry<T>>(entry_->next_ptr_);
    return list_iterator(list_, next_entry, entry_->next_ptr_);
  }

  list_iterator operator--(int) const {
    auto prior_entry = LABSTOR_MEMORY_MANAGER->template
      Convert<list_entry<T>>(entry_->next_ptr_);
    return list_iterator(list_, prior_entry, entry_->prior_ptr_);
  }

  list_iterator operator+(size_t count) const {
    list_iterator pos = *this;
    for (size_t i = 0; i < count; ++i) {
      ++pos;
    }
    return pos;
  }

  list_iterator operator-(size_t count) const {
    list_iterator pos = *this;
    for (size_t i = 0; i < count; ++i) {
      --pos;
    }
    return pos;
  }

  void operator+=(size_t count) {
    list_iterator pos = (*this) + count;
    entry_ = pos.entry_;
    entry_ptr_ = pos.entry_ptr_;
  }

  void operator-=(size_t count) {
    list_iterator pos = (*this) - count;
    entry_ = pos.entry_;
    entry_ptr_ = pos.entry_ptr_;
  }

  void operator=(list_iterator<T> &other) {
    list_ = other.list_;
    entry_ = other.entry_;
    entry_ptr_ = other.entry_ptr_;
  }

  friend bool operator==(const list_iterator &a,
                         const list_iterator &b) {
    return a.entry_ == b.entry_;
  }

  friend bool operator!=(const list_iterator &a,
                         const list_iterator &b) {
    return a.entry_ != b.entry_;
  }
};

#define CLASS_NAME list
#define TYPED_CLASS list<T>
#define TYPED_HEADER list_header<T>

template<typename T>
class list : public ShmDataStructure<TYPED_CLASS, TYPED_HEADER> {
 SHM_DATA_STRUCTURE_TEMPLATE(CLASS_NAME, TYPED_CLASS, TYPED_HEADER)
 friend list_iterator<T>;

 private:
  typedef SHM_T_OR_ARCHIVE(T) T_Ar;
  typedef SHM_T_OR_REF_T(T) T_Ref;

 public:
  list() = default;

  explicit list(Allocator *alloc) :
    ShmDataStructure<TYPED_CLASS, TYPED_HEADER>(alloc) {}

  void shm_init() {
    Pointer head_ptr;
    header_ = alloc_->template
      AllocateObjs<TYPED_HEADER>(1, header_ptr_);
    memset(header_, 0, sizeof(header_));
  }

  void shm_destroy() {
    erase(begin(), end());
    alloc_->Free(header_ptr_);
  }

  template<typename... Args>
  inline void emplace_back(Args&&... args) {
    emplace(end(), args...);
  }

  template<typename... Args>
  inline void emplace_front(Args&&... args) {
    emplace(begin(), args...);
  }

  template<typename ...Args>
  void emplace(list_iterator<T> pos, Args&&... args) {
    Pointer entry_ptr;
    auto entry = _create_entry(entry_ptr, args...);
    if (size() == 0) {
      entry->prior_ptr_ = kNullPointer;
      entry->next_ptr_ = kNullPointer;
      header_->head_ptr_ = entry_ptr;
      header_->tail_ptr_ = entry_ptr;
    } else if (pos == begin()) {
      entry->prior_ptr_ = kNullPointer;
      entry->next_ptr_ = header_->head_ptr_;
      auto head = mem_mngr_->template
        Convert<list_entry<T>>(header_->tail_ptr_);
      head->prior_ptr_ = entry_ptr;
      header_->head_ptr_ = entry_ptr;
    } else if (pos == end()) {
      entry->prior_ptr_ = header_->tail_ptr_;
      entry->next_ptr_ = kNullPointer;
      auto tail = mem_mngr_->template
        Convert<list_entry<T>>(header_->tail_ptr_);
      tail->next_ptr_ = entry_ptr;
      header_->tail_ptr_ = entry_ptr;
    } else {
      auto next = mem_mngr_->template
        Convert<list_entry<T>>(pos.entry_->next_ptr_);
      auto prior = mem_mngr_->template
        Convert<list_entry<T>>(pos.entry_->prior_ptr_);
      entry->next_ptr_ = pos.entry_->next_ptr_;
      entry->prior_ptr_ = pos.entry_->prior_ptr_;
      next->prior_ptr_ = entry_ptr;
      prior->next_ptr_ = entry_ptr;
    }
    ++header_->length_;
  }

  void erase(list_iterator<T> first,
             list_iterator<T> last) {
    if (first == end()) { return; }
    auto first_prior_ptr = first.entry_->prior_ptr_;
    auto pos = first;
    while (pos != last) {
      auto next = pos + 1;
      _destruct<T,T_Ar>(pos.entry_->data_);
      alloc_->Free(pos.entry_ptr_);
      --header_->length_;
      pos = next;
    }

    if (first_prior_ptr.is_null()) {
      header_->head_ptr_ = last.entry_ptr_;
    } else {
      auto first_prior = mem_mngr_->template
        Convert<list_entry<T>>(first_prior_ptr);
      first_prior->next_ptr_ = last.entry_ptr_;
    }

    if (last.entry_ptr_.is_null()) {
      header_->tail_ptr_ = first_prior_ptr;
    } else {
      last.entry_->prior_ptr_ = first_prior_ptr;
    }
  }

  inline T_Ref front() {
    return *begin();
  }

  inline T_Ref back() {
    return *end();
  }

  inline size_t size() const {
    if (header_ == nullptr) {
      return 0;
    }
    return header_->length_;
  }

  /**
   * ITERATORS
   * */

  inline list_iterator<T> begin() {
    auto head = alloc_->template
      Convert<list_entry<T>>(header_->head_ptr_);
    return list_iterator<T>(*this, head, header_->head_ptr_);
  }

  inline list_iterator<T> end() {
    return list_iterator<T>(*this, nullptr, kNullPointer);
  }

 private:
  template<typename ...Args>
  inline list_entry<T>* _create_entry(Pointer &ptr, Args ...args) {
    auto entry = alloc_->template
      AllocateObjs<list_entry<T>>(1, ptr);
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      T obj(args...);
      obj >> entry->data_;
    } else {
      _construct<T,T_Ar>(entry->data_, args...);
    }
    return entry;
  }
};

}  // namespace labstor::ipc

#undef CLASS_NAME
#undef TYPED_CLASS
#undef TYPED_HEADER

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_LOCKLESS_LIST_H_
