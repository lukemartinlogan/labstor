//
// Created by lukemartinlogan on 11/28/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_LOCKLESS_LIST_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_LOCKLESS_LIST_H_

#include <labstor/data_structures/data_structure.h>

namespace labstor::ipc::lockless {
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
}  // namespace labstor::ipc::lockless

namespace labstor::ipc {

template<typename T>
struct ShmArchive<lockless::list<T>> {
  Pointer header_ptr_;
};

template<typename T>
struct ShmHeader<lockless::list<T>> {
  Pointer head_ptr_, tail_ptr_;
  size_t length_;
};

}  // namespace labstor::ipc

namespace labstor::ipc::lockless {

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
                         Pointer &entry_ptr) :
    list_(list), entry_(entry), entry_ptr_(entry_ptr) {}

  T_Ref operator*() const {
    return entry_->data();
  }

  list_iterator& operator++() {
    entry_ = LABSTOR_MEMORY_MANAGER->template
      Convert<list_entry<T>>(entry_->next_ptr_, entry_ptr_);
    return *this;
  }

  list_iterator& operator--() {
    return *this;
  }

  list_iterator operator++(int) const {
    auto next_entry = LABSTOR_MEMORY_MANAGER->template
      Convert<list_entry<T>>(entry_->next_ptr_);
    return list_iterator(list_, next_entry, entry_->next_ptr_);
  }

  list_iterator operator--(int) const {
    auto prior_entry = LABSTOR_MEMORY_MANAGER->template
      Convert<list_entry<T>>(entry_->next_ptr_);
    return list_iterator(list_, prior_entry, entry_->prior_ptr_);
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

template<typename T>
class list : public ShmDataStructure<list<T>> {
 SHM_DATA_STRUCTURE_TEMPLATE(list, list<T>)

 private:
  typedef SHM_T_OR_ARCHIVE(T) T_Ar;
  typedef SHM_T_OR_REF_T(T) T_Ref;

 public:
  list() = default;

  explicit list(Allocator *alloc) : ShmDataStructure<list<T>>(alloc) {}

  explicit list(Allocator *alloc, bool init) :
    ShmDataStructure<list<T>>(alloc) {
    if (init) {
      header_ = alloc_->template AllocateObjs<ShmHeader<list<T>>>(header_ptr_);
      memset(header_, 0, sizeof(header_));
    }
  }

  void shm_destroy() {
    erase(begin(), end());
    alloc_->Free(header_);
  }

  void shm_serialize(ShmArchive<list<T>> &ar) {
    ar.header_ptr_ = header_ptr_;
  }

  void shm_deserialize(ShmArchive<list<T>> &ar) {
    InitDataStructure(ar.header_ptr_);
  }

  template<typename... Args>
  void emplace_back(Args&&... args) {
    emplace(end(), args...);
  }

  template<typename... Args>
  void emplace_front(Args&&... args) {
    emplace(begin(), args...);
  }

  template<typename ...Args>
  void emplace(list_entry<T> pos, Args&&... args) {
    Pointer entry_ptr;
    auto entry = _create_entry(entry_ptr, args...);
    if (size() == 0) {
      entry->prior_ptr_ = kNullPointer;
      entry->next_ptr_ = kNullPointer;
      header_->head_ptr_ = entry_ptr;
      header_->tail_ptr_ = entry_ptr;
    } else if (pos.entry_ptr_ == header_->head_) {
      entry->prior_ptr_ = kNullPointer;
      entry->next_ptr_ = header_->head_ptr_;
      auto head = mem_mngr_->template
        Convert<list_entry<T>>(header_->Tail());
      head->prior_ptr_ = entry_ptr;
      header_->head_ptr_ = entry_ptr;
    } else if (pos.entry_ptr_ == header_->tail_) {
      entry->prior_ptr_ = header_->tail_ptr_;
      entry->next_ptr_ = kNullPointer;
      auto tail = mem_mngr_->template
        Convert<list_entry<T>>(header_->Tail());
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

  void erase(list_entry<T> first,
             list_entry<T> last) {
    auto first_prior_ptr = first.entry_->prior_ptr_;
    auto pos = first;
    list_entry<T> next;
    while (pos != last) {
      next = mem_mngr_->template
        Convert<list_entry<T>>(pos.entry_->next_ptr_);
      _destruct<T,T_Ar>(*pos.entry_);
      pos = next;
    }

    if (first_prior_ptr == kNullPointer) {
      header_->head_ = last.entry_ptr_;
    } else {
      auto first_prior = mem_mngr_->template
        Convert<list_entry<T>>(first_prior_ptr);
      first_prior->next_ptr_ = last.entry_ptr_;
    }

    if (last.entry_ptr_ == kNullPointer) {
      header_->tail_ = first_prior_ptr;
    } else {
      last.entry_->prior_ptr_ = first_prior_ptr;
    }
  }

  size_t size() const {
    return header_->length_;
  }

  /**
   * ITERATORS
   * */

 public:
  list_entry<T> begin() {
    return list_entry<T>(*this, 0);
  }

  list_entry<T> end() {
    return list_entry<T>(*this, size());
  }

 private:
  template<typename ...Args>
  inline list_entry<T>* _create_entry(Pointer &ptr, Args ...args) {
    auto entry = mem_mngr_->template
      AllocateObjs<list_entry<T>>(ptr);
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      entry->data_ << T(args...);
    } else {
      _construct<T,T_Ar>(entry->data_, args...);
    }
    return entry;
  }
};

}  // namespace labstor::ipc::lockless

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_LOCKLESS_LIST_H_
