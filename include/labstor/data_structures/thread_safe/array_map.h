//
// Created by lukemartinlogan on 11/6/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_LOCKLESS_ARRAY_QUEUE_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_LOCKLESS_ARRAY_QUEUE_H_

#include <labstor/data_structures/data_structure.h>
#include <labstor/data_structures/thread_unsafe/vector.h>

namespace labstor::ipc {

/**
 * forward declaration for the array_queue:
 *
 * Key must be an atomic-compatible type
 * */
template<typename Key, typename T>
class array_queue;

/**
 * An array_queue slot
 * */
template<typename Key, typename T>
struct array_queue_entry {
 public:
  typedef SHM_T_OR_ARCHIVE(T) T_Ar;

 public:
  bool correct_;
  Key key_;
};

/** array_queue shared-memory header */
template<typename Key, typename T>
struct array_queue_header {
  std::atomic<Key> enqueued_;
  std::atomic<Key> dequeued_;
  std::atomic<Key> size_;
  ShmArchive<vector<T>> vec_;
};

/**
 * MACROS to simplify the string namespace
 * */
#define CLASS_NAME array_queue
#define TYPED_CLASS array_queue<Key, T>
#define TYPED_HEADER array_queue<Key, T>

/** The array_queue implementation */
template<typename Key, typename T>
class array_queue {
 public:
  SHM_DATA_STRUCTURE_TEMPLATE(CLASS_NAME, TYPED_CLASS, TYPED_HEADER)

 public:
  typedef SHM_T_OR_ARCHIVE(T) T_Ar;
  typedef SHM_T_OR_REF_T(T) T_Ref;

 public:
  array_queue() = default;

  /** Default shared-memory constructor */
  explicit array_queue(Allocator *alloc)
  : ShmDataStructure<TYPED_CLASS, TYPED_HEADER>(alloc) {
    shm_init();
  }

  /**
   * Construct a array_queue of a certain length in shared memory
   *
   * @param length the size the array_queue should be
   * @param alloc the allocator to reserve memory from
   * @param args the parameters of the elements to construct
   * */
  template<typename ...Args>
  explicit array_queue(Allocator *alloc, size_t length, Args&& ...args)
  : ShmDataStructure<TYPED_CLASS, TYPED_HEADER>(alloc) {
    shm_init(length);
  }

  /**
   * Construct a array_queue of a certain length in shared memory
   *
   * @param length the size the array_queue should be
   * @param alloc_id the id of the allocator to reserve memory from
   * @param args the parameters of the elements to construct
   * */
  template<typename ...Args>
  explicit array_queue(allocator_id_t alloc_id, size_t length, Args&& ...args)
  : ShmDataStructure<TYPED_CLASS, TYPED_HEADER>(alloc_id) {
    resize(length, std::forward<Args>(args)...);
  }

  /** Moves one array_queue into another */
  array_queue(array_queue&& source) {
    header_ptr_ = source.header_ptr_;
    header_ = source.header_;
    source.header_ptr_.set_null();
  }

  /** Disable copying  */
  array_queue(const array_queue &other) = delete;

  /** Assign one array_queue into another */
  array_queue&
  operator=(const array_queue &other) {
    if (this != &other) {
      header_ptr_ = other.header_ptr_;
      header_ = other.header_;
    }
    return *this;
  }

  /** Construct the array_queue in shared memory */
  void shm_init(size_t depth = 32) {
    header_ = alloc_->template
      AllocateObjs<TYPED_HEADER>(1, header_ptr_);
    vector<> header_->vec_;
  }

  /** Destroy all shared memory allocated by the array_queue */
  void shm_destroy() {
    if (header_ptr_.is_null()) { return; }
    erase(begin(), end());
    alloc_->Free(header_->vec_ptr_);
    alloc_->Free(header_ptr_);
  }

  /** Emplace a request into the array queue */
  void emplace() {
  }
};

}

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_ARRAY_QUEUE_H_
