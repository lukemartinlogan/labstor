//
// Created by lukemartinlogan on 11/1/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_MEMORY_MEMORY_H_
#define LABSTOR_INCLUDE_LABSTOR_MEMORY_MEMORY_H_

#include <labstor/types/basic.h>
#include <labstor/constants/singleton_macros.h>
#include <labstor/introspect/system_info.h>
#include "shm_macros.h"

namespace labstor::ipc {

/**
 * The identifier for an allocator
 * */
union allocator_id_t {
  struct {
    uint32_t major_;  // Typically some sort of process id
    uint32_t minor_;  // Typically a process-local id
  } bits;
  uint64_t int_;

  /**
   * Null allocator ID is -1 (for now)
   * */
  allocator_id_t() : int_(-1) {}

  /**
   * Constructor which sets major & minor
   * */
  explicit allocator_id_t(uint32_t major, uint32_t minor) {
    bits.major_ = major;
    bits.minor_ = minor;
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
};

typedef uint32_t slot_id_t;  // Uniquely ids a MemoryBackend slot

/**
 * A process-independent pointer
 * */
struct Pointer {
  allocator_id_t allocator_id_;   // allocator pointer is attached to
  size_t off_;                    // Offset within the allocator's slot

  /** Default constructor */
  Pointer() : off_(0), allocator_id_() {}

  /** Full constructor */
  explicit Pointer(allocator_id_t id, size_t off) :
    allocator_id_(id), off_(off) {}

  /** Set to null */
  void set_null() {
    allocator_id_.set_null();
  }

  /** Check if null */
  bool is_null() {
    return allocator_id_.is_null();
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
static const Pointer kNullPointer;

/**
 * Indicates that a data structure supports shared-memory storage by
 * implementing the interfaces shown in the comments
 * */
class ShmSerializeable {
 public:
  // virtual void shm_init(args...) = 0;
  // virtual void shm_destroy() = 0;
  // virtual void shm_serialize(ShmArchive &ar) = 0;
  // virtual void shm_deserialize(ShmArchive &ar) = 0;
  // void operator>>(ShmArchive &ar);
  // void operator<<(ShmArchive &r);
};

/**
 * A wrapper around a process-independent pointer for storing
 * a single complex shared-memory data structure
 * */
template<typename T>
struct ShmArchive {
  Pointer header_ptr_;

  /** Default constructor */
  ShmArchive() = default;

  /** Get the process-independent pointer */
  inline Pointer& Get() {
    return header_ptr_;
  }

  /** Archives a shm-serializeable object */
  explicit ShmArchive(const T &obj) {
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      obj >> (*this);
    }
  }

  /** Constructs and archives a shm-serializeable object */
  template<typename ...Args>
  explicit ShmArchive(Args ...args) {
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      T(args...) >> (*this);
    }
  }
};

/**
 * A wrapper around a process-independent pointer for
 * storing a C-style array of a simple type
 * */
template<typename T>
struct ShmPointer {
  Pointer ptr_;

  /** Default constructor */
  ShmPointer() = default;

  /** Get the process-independent pointer */
  inline Pointer& Get() {
    return ptr_;
  }
};

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


#endif  // LABSTOR_INCLUDE_LABSTOR_MEMORY_MEMORY_H_
