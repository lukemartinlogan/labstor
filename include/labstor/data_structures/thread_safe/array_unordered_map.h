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


#ifndef LABSTOR_DATA_STRUCTURES_ARRAY_UNORDERED_MAP_H_
#define LABSTOR_DATA_STRUCTURES_ARRAY_UNORDERED_MAP_H_

#include <labstor/data_structures/data_structure.h>
#include <labstor/data_structures/thread_unsafe/vector.h>

namespace labstor::ipc {

/**
 * forward declaration for the array_unordered_map:
 *
 * Key must be an atomic-compatible type
 * */
template<typename Key, typename T>
class array_unordered_map;

/**
 * An array_unordered_map slot
 * */
template<typename Key, typename T>
struct array_unordered_map_entry {
 public:
  typedef SHM_T_OR_ARCHIVE(T) T_Ar;

 public:
  bool correct_;
  Key key_;
};

/** array_unordered_map shared-memory header */
template<typename Key, typename T>
struct array_unordered_map_header {
  std::atomic<Key> enqueued_;
  std::atomic<Key> dequeued_;
  std::atomic<Key> size_;
  ShmArchive<vector<T>> vec_;
};

/**
 * MACROS to simplify the string namespace
 * */
#define CLASS_NAME array_unordered_map
#define TYPED_CLASS array_unordered_map<Key, T>
#define TYPED_HEADER array_unordered_map<Key, T>

/** The array_unordered_map implementation */
template<typename Key, typename T>
class array_unordered_map {
 public:
  BASIC_SHM_CONTAINER_TEMPLATE

 public:
  typedef SHM_T_OR_ARCHIVE(T) T_Ar;
  typedef SHM_T_OR_REF_T(T) T_Ref;

 public:
  array_unordered_map() = default;

  /** Default shared-memory constructor */
  explicit array_unordered_map(Allocator *alloc)
  : ShmContainer<TYPED_CLASS>(alloc) {
    shm_init();
  }

  /**
   * Construct a array_unordered_map of a certain length in shared memory
   *
   * @param length the size the array_unordered_map should be
   * @param alloc the allocator to reserve memory from
   * @param args the parameters of the elements to construct
   * */
  template<typename ...Args>
  explicit array_unordered_map(Allocator *alloc, size_t length, Args&& ...args)
  : ShmContainer<TYPED_CLASS>(alloc) {
    shm_init(length);
  }

  /**
   * Construct a array_unordered_map of a certain length in shared memory
   *
   * @param length the size the array_unordered_map should be
   * @param alloc_id the id of the allocator to reserve memory from
   * @param args the parameters of the elements to construct
   * */
  template<typename ...Args>
  explicit array_unordered_map(allocator_id_t alloc_id, size_t length, Args&& ...args)
  : ShmContainer<TYPED_CLASS>(alloc_id) {
    resize(length, std::forward<Args>(args)...);
  }

  /** Moves one array_unordered_map into another */
  array_unordered_map(array_unordered_map&& source) {
    header_ptr_ = source.header_ptr_;
    header_ = source.header_;
    source.header_ptr_.SetNull();
  }

  /** Disable copying  */
  array_unordered_map(const array_unordered_map &other) = delete;

  /** Assign one array_unordered_map into another */
  array_unordered_map&
  operator=(const array_unordered_map &other) {
    if (this != &other) {
      header_ptr_ = other.header_ptr_;
      header_ = other.header_;
    }
    return *this;
  }

  /** Construct the array_unordered_map in shared memory */
  void shm_init(size_t depth = 32) {
    header_ = alloc_->template
      AllocateObjs<TYPED_HEADER>(1, header_ptr_);
    vector<> header_->vec_;
  }

  /** Destroy all shared memory allocated by the array_unordered_map */
  void shm_destroy() {
    if (header_ptr_.IsNull()) { return; }
    erase(begin(), end());
    alloc_->Free(header_->vec_ptr_);
    alloc_->Free(header_ptr_);
  }

  /** Emplace a request into the array queue */
  void emplace() {
  }
};

}  // namespace labstor::ipc

#endif  // LABSTOR_DATA_STRUCTURES_array_unordered_map_H_
