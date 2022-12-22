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


#ifndef LABSTOR_MEMORY_MEMORY_H_
#define LABSTOR_MEMORY_MEMORY_H_

#include <labstor/types/basic.h>
#include <labstor/constants/data_structure_singleton_macros.h>
#include <labstor/introspect/system_info.h>

#include <atomic>

namespace labstor::ipc {

/**
 * The identifier for an allocator
 * */
union allocator_id_t {
  struct {
    uint32_t major_;  // Typically some sort of process id
    uint32_t minor_;  // Typically a process-local id
  } bits_;
  uint64_t int_;

  /**
   * Null allocator ID is -1 (for now)
   * */
  allocator_id_t() : int_(-1) {}

  /**
   * Constructor which sets major & minor
   * */
  explicit allocator_id_t(uint32_t major, uint32_t minor) {
    bits_.major_ = major;
    bits_.minor_ = minor;
  }

  /**
   * Set this allocator to null
   * */
  void set_null() {
    int_ = -1;
  }

  /**
   * Check if this is the null allocator
   * */
  bool is_null() const { return int_ == -1; }

  /** Equality check */
  bool operator==(const allocator_id_t &other) const {
    return other.int_ == int_;
  }

  /** Inequality check */
  bool operator!=(const allocator_id_t &other) const {
    return other.int_ != int_;
  }

  /** Get the null allocator */
  static allocator_id_t null() {
    allocator_id_t alloc;
    alloc.set_null();
    return alloc;
  }
};

typedef uint32_t slot_id_t;  // Uniquely ids a MemoryBackend slot

/**
 * A process-independent pointer
 * */
struct Pointer {
  allocator_id_t allocator_id_;   // allocator pointer is attached to
  std::atomic<size_t> off_;       // Offset within the allocator's slot

  /** Default constructor */
  Pointer() = default;

  /** Full constructor */
  explicit Pointer(allocator_id_t id, size_t off) :
    allocator_id_(id), off_(off) {}

  /** Copy constructor */
  Pointer(const Pointer &other)
  : allocator_id_(other.allocator_id_), off_(other.off_.load()) {
  }

  /** Move constructor */
  Pointer(Pointer &&other) noexcept
  : allocator_id_(other.allocator_id_), off_(other.off_.load()) {
    other.set_null();
  }

  /** Copy assignment operator */
  Pointer& operator=(const Pointer &other) {
    if (this != &other) {
      allocator_id_ = other.allocator_id_;
      off_ = other.off_.load();
    }
    return *this;
  }

  /** Move assignment operator */
  Pointer& operator=(Pointer &&other) {
    if (this != &other) {
      allocator_id_ = other.allocator_id_;
      off_ = other.off_.load();
      other.set_null();
    }
    return *this;
  }

  /** Set to null */
  void set_null() {
    allocator_id_.set_null();
  }

  /** Check if null */
  bool is_null() const {
    return allocator_id_.is_null();
  }

  /** Get the null pointer */
  static Pointer null() {
    Pointer p;
    p.set_null();
    return p;
  }

  /** Equality check */
  bool operator==(const Pointer &other) const {
    return (other.allocator_id_ == allocator_id_ && other.off_ == off_);
  }

  /** Inequality check */
  bool operator!=(const Pointer &other) const {
    return (other.allocator_id_ != allocator_id_ || other.off_ != off_);
  }
};

/** The null process-indepent pointer */
static const Pointer kNullPointer = Pointer::null();

/** Round up to the nearest multiple of the alignment */
static size_t NextAlignmentMultiple(size_t alignment, size_t size) {
  auto page_size = LABSTOR_SYSTEM_INFO->page_size_;
  size_t new_size = size;
  size_t page_off = size % alignment;
  if (page_off) {
    new_size = size + page_size - page_off;
  }
  return new_size;
}

/** Round up to the nearest multiple of page size */
static size_t NextPageSizeMultiple(size_t size) {
  auto page_size = LABSTOR_SYSTEM_INFO->page_size_;
  size_t new_size = NextAlignmentMultiple(page_size, size);
  return new_size;
}

}  // namespace labstor::ipc

namespace std {

/** Allocator ID hash */
template <>
struct hash<labstor::ipc::allocator_id_t> {
  std::size_t operator()(const labstor::ipc::allocator_id_t &key) const {
    return std::hash<uint64_t>{}(key.int_);
  }
};

}  // namespace std


#endif  // LABSTOR_MEMORY_MEMORY_H_
