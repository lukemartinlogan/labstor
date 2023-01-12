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
#include <labstor/types/bitfield.h>
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
  allocator_id_t() : int_(0) {}

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
  void SetNull() {
    int_ = 0;
  }

  /**
   * Check if this is the null allocator
   * */
  bool IsNull() const { return int_ == 0; }

  /** Equality check */
  bool operator==(const allocator_id_t &other) const {
    return other.int_ == int_;
  }

  /** Inequality check */
  bool operator!=(const allocator_id_t &other) const {
    return other.int_ != int_;
  }

  /** Get the null allocator */
  static allocator_id_t GetNull() {
    allocator_id_t alloc;
    alloc.SetNull();
    return alloc;
  }
};

typedef uint32_t slot_id_t;  // Uniquely ids a MemoryBackend slot

/**
 * Stores an offset into a memory region. Assumes the developer knows
 * which allocator the pointer comes from.
 * */
struct OffsetPointer {
  std::atomic<size_t> off_;       // Offset within the allocator's slot

  /** Default constructor */
  OffsetPointer() = default;

  /** Full constructor */
  explicit OffsetPointer(size_t off) : off_(off) {}

  /** Copy constructor */
  OffsetPointer(const OffsetPointer &other) : off_(other.off_.load()) {
  }

  /** Move constructor */
  OffsetPointer(OffsetPointer &&other) noexcept
    : off_(other.off_.load()) {
    other.SetNull();
  }

  /** Copy assignment operator */
  OffsetPointer& operator=(const OffsetPointer &other) {
    if (this != &other) {
      off_ = other.off_.load();
    }
    return *this;
  }

  /** Move assignment operator */
  OffsetPointer& operator=(OffsetPointer &&other) {
    if (this != &other) {
      off_ = other.off_.load();
      other.SetNull();
    }
    return *this;
  }

  /** Addition operator */
  OffsetPointer operator+(size_t size) const {
    OffsetPointer p;
    p.off_ = off_ + size;
    return p;
  }

  /** Subtraction operator */
  OffsetPointer operator-(size_t size) const {
    OffsetPointer p;
    p.off_ = off_ - size;
    return p;
  }

  /** Addition assignment operator */
  OffsetPointer& operator+=(size_t size) {
    off_ += size;
    return *this;
  }

  /** Subtraction assignment operator */
  OffsetPointer& operator-=(size_t size) {
    off_ -= size;
    return *this;
  }

  /** Set to null */
  void SetNull() {
    off_ = -1;
  }

  /** Check if null */
  bool IsNull() const {
    return off_ == -1;
  }

  /** Get the null pointer */
  static OffsetPointer GetNull() {
    OffsetPointer p;
    p.SetNull();
    return p;
  }

  /** Equality check */
  bool operator==(const OffsetPointer &other) const {
    return (other.off_ == off_);
  }

  /** Inequality check */
  bool operator!=(const OffsetPointer &other) const {
    return (other.off_ != off_);
  }
};

/**
 * A process-independent pointer, which stores both the allocator's
 * information and the offset within the allocator's region
 * */
struct Pointer {
  allocator_id_t allocator_id_;   // allocator pointer is attached to
  size_t off_;                    // Offset within the allocator's slot

  /** Default constructor */
  Pointer() = default;

  /** Full constructor */
  explicit Pointer(allocator_id_t id, size_t off) :
    allocator_id_(id), off_(off) {}

  /** Copy constructor */
  Pointer(const Pointer &other)
  : allocator_id_(other.allocator_id_), off_(other.off_) {
  }

  /** Move constructor */
  Pointer(Pointer &&other) noexcept
  : allocator_id_(other.allocator_id_), off_(other.off_) {
    other.SetNull();
  }

  /** Copy assignment operator */
  Pointer& operator=(const Pointer &other) {
    if (this != &other) {
      allocator_id_ = other.allocator_id_;
      off_ = other.off_;
    }
    return *this;
  }

  /** Move assignment operator */
  Pointer& operator=(Pointer &&other) {
    if (this != &other) {
      allocator_id_ = other.allocator_id_;
      off_ = other.off_;
      other.SetNull();
    }
    return *this;
  }

  /** Addition operator */
  Pointer operator+(size_t size) const {
    Pointer p;
    p.allocator_id_ = allocator_id_;
    p.off_ = off_ + size;
    return p;
  }

  /** Subtraction operator */
  Pointer operator-(size_t size) const {
    Pointer p;
    p.allocator_id_ = allocator_id_;
    p.off_ = off_ - size;
    return p;
  }

  /** Addition assignment operator */
  Pointer& operator+=(size_t size) {
    off_ += size;
    return *this;
  }

  /** Subtraction assignment operator */
  Pointer& operator-=(size_t size) {
    off_ -= size;
    return *this;
  }

  /** Set to null */
  void SetNull() {
    allocator_id_.SetNull();
  }

  /** Check if null */
  bool IsNull() const {
    return allocator_id_.IsNull();
  }

  /** Get the null pointer */
  static Pointer GetNull() {
    Pointer p;
    p.SetNull();
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
static const Pointer kNullPointer = Pointer::GetNull();

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
