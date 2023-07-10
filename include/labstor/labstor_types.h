//
// Created by lukemartinlogan on 6/22/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_LABSTOR_TYPES_H_
#define LABSTOR_INCLUDE_LABSTOR_LABSTOR_TYPES_H_

#include <hermes_shm/data_structures/ipc/unordered_map.h>
#include <hermes_shm/data_structures/ipc/vector.h>
#include <hermes_shm/data_structures/ipc/list.h>
#include <hermes_shm/data_structures/ipc/slist.h>
#include <hermes_shm/data_structures/data_structure.h>
#include <hermes_shm/data_structures/ipc/string.h>
#include <hermes_shm/data_structures/ipc/mpsc_queue.h>
#include <hermes_shm/data_structures/ipc/mpsc_ptr_queue.h>
#include <hermes_shm/data_structures/ipc/ticket_queue.h>
#include <hermes_shm/data_structures/containers/charbuf.h>
#include <hermes_shm/data_structures/containers/spsc_queue.h>
#include <hermes_shm/data_structures/containers/converters.h>
#include "hermes_shm/data_structures/serialization/shm_serialize.h"
#include <hermes_shm/util/auto_trace.h>
#include <hermes_shm/thread/lock.h>
#include <hermes_shm/thread/thread_model_manager.h>
#include <hermes_shm/types/atomic.h>
#include "hermes_shm/util/singleton.h"
#include "hermes_shm/constants/macros.h"

typedef uint8_t u8;   /**< 8-bit unsigned integer */
typedef uint16_t u16; /**< 16-bit unsigned integer */
typedef uint32_t u32; /**< 32-bit unsigned integer */
typedef uint64_t u64; /**< 64-bit unsigned integer */
typedef int8_t i8;    /**< 8-bit signed integer */
typedef int16_t i16;  /**< 16-bit signed integer */
typedef int32_t i32;  /**< 32-bit signed integer */
typedef int64_t i64;  /**< 64-bit signed integer */
typedef float f32;    /**< 32-bit float */
typedef double f64;   /**< 64-bit float */

namespace labstor {

using hshm::RwLock;
using hshm::Mutex;
using hshm::bitfield32_t;
using hshm::ScopedRwReadLock;
using hshm::ScopedRwWriteLock;
typedef hshm::bitfield<uint64_t> bitfield64_t;

/** Determine the mode that LabStor is initialized for */
enum class LabstorMode {
  kNone,
  kClient,
  kServer
};

#define DOMAIN_FLAG_T static inline const int

/**
 * Represents a unique ID for a scheduling domain
 * There are a few domain types:
 * 1. A node domain, which is a single node
 * 2. A global domain, which is all nodes
 * 3. A specific domain, which is a subset of nodes
 * 4. A specific domain + node, temporarily includes this node in a domain
 * */
struct DomainId {
  bitfield32_t flags_;  /**< Flags indicating how to interpret id */
  u32 id_;              /**< The domain id, 0 is NULL */
  DOMAIN_FLAG_T kLocal = (1 << 0);   /**< Include local node in scheduling decision */
  DOMAIN_FLAG_T kGlobal = (1 << 1);  /**< Use all nodes in scheduling decision */
  DOMAIN_FLAG_T kSet = (1 << 2);     /**< ID represents a set of nodes, not a single node */

  /** Default constructor. */
  HSHM_ALWAYS_INLINE
  DomainId() : id_(0) {}

  /** DomainId representing the local node */
  HSHM_ALWAYS_INLINE
  static DomainId GetLocal() {
      DomainId id;
      id.id_ = 0;
      id.flags_.SetBits(kLocal);
      return id;
  }

  /** DomainId representing a specific node */
  HSHM_ALWAYS_INLINE
  static DomainId GetNode(u32 node_id) {
    DomainId id;
    id.id_ = node_id;
    return id;
  }

  /** DomainId representing a specific node + local node */
  HSHM_ALWAYS_INLINE
  static DomainId GetNodeWithLocal(u32 node_id) {
    DomainId id;
    id.id_ = node_id;
    id.flags_.SetBits(kLocal);
    return id;
  }

  /** DomainId representing all nodes */
  HSHM_ALWAYS_INLINE
  static DomainId GetGlobal() {
      DomainId id;
      id.id_ = 0;
      id.flags_.SetBits(kGlobal);
      return id;
  }

  /** DomainId representing a node set */
  HSHM_ALWAYS_INLINE
  static DomainId GetSet(u32 domain_id) {
    DomainId id;
    id.id_ = domain_id;
    id.flags_.SetBits(kSet);
    return id;
  }

  /** DomainId representing a node set + local node */
  HSHM_ALWAYS_INLINE
  static DomainId GetSetWithLocal(u32 domain_id) {
    DomainId id;
    id.id_ = domain_id;
    id.flags_.SetBits(kSet | kLocal);
    return id;
  }

  /** Copy constructor */
  HSHM_ALWAYS_INLINE
  DomainId(const DomainId &other) {
    id_ = other.id_;
    flags_ = other.flags_;
  }

  /** Copy operator */
  HSHM_ALWAYS_INLINE
  DomainId& operator=(const DomainId &other) {
    if (this != &other) {
      id_ = other.id_;
      flags_ = other.flags_;
    }
    return *this;
  }

  /** Move constructor */
  HSHM_ALWAYS_INLINE
  DomainId(DomainId &&other) noexcept {
    id_ = other.id_;
    flags_ = other.flags_;
  }

  /** Move operator */
  HSHM_ALWAYS_INLINE
  DomainId& operator=(DomainId &&other) noexcept {
    if (this != &other) {
      id_ = other.id_;
      flags_ = other.flags_;
    }
    return *this;
  }
};

/** Represents unique ID for states + queues */
template<int TYPE>
struct UniqueId {
  u32 node_id_;  /**< The node the content is on */
  u64 unique_;   /**< A unique id for the blob */

  /** Default constructor */
  HSHM_ALWAYS_INLINE
  UniqueId() = default;

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE
  UniqueId(u32 domain_id, u64 unique) : node_id_(node_id), unique_(unique) {}

  /** Copy constructor */
  HSHM_ALWAYS_INLINE
  UniqueId(const UniqueId &other) {
    node_id_ = other.node_id_;
    unique_ = other.unique_;
  }

  /** Copy constructor */
  template<int OTHER_TYPE=TYPE>
  HSHM_ALWAYS_INLINE
  UniqueId(const UniqueId<OTHER_TYPE> &other) {
    node_id_ = other.node_id_;
    unique_ = other.unique_;
  }

  /** Copy assignment */
  HSHM_ALWAYS_INLINE
  UniqueId& operator=(const UniqueId &other) {
    if (this != &other) {
      node_id_ = other.node_id_;
      unique_ = other.unique_;
    }
    return *this;
  }

  /** Move constructor */
  HSHM_ALWAYS_INLINE
  UniqueId(UniqueId &&other) noexcept {
    node_id_ = other.node_id_;
    unique_ = other.unique_;
  }

  /** Move assignment */
  HSHM_ALWAYS_INLINE
  UniqueId& operator=(UniqueId &&other) noexcept {
    if (this != &other) {
      node_id_ = other.node_id_;
      unique_ = other.unique_;
    }
    return *this;
  }

  /** Check if null */
  [[nodiscard]]
  HSHM_ALWAYS_INLINE bool IsNull() const { return unique_ == 0; }

  /** Get null id */
  HSHM_ALWAYS_INLINE
  static UniqueId GetNull() {
    static const UniqueId id(0, 0);
    return id;
  }

  /** Set to null id */
  HSHM_ALWAYS_INLINE
  void SetNull() {
    node_id_ = 0;
    unique_ = 0;
  }

  /** Get id of node from this id */
  [[nodiscard]]
  HSHM_ALWAYS_INLINE
  u32 GetNodeId() const { return node_id_; }

  /** Compare two ids for equality */
  HSHM_ALWAYS_INLINE
  bool operator==(const UniqueId &other) const {
    return unique_ == other.unique_ && node_id_ == other.node_id_;
  }

  /** Compare two ids for inequality */
  HSHM_ALWAYS_INLINE
  bool operator!=(const UniqueId &other) const {
    return unique_ != other.unique_ || node_id_ != other.node_id_;
  }

  /** Serialize a UniqueId
  template<class Archive>
  void serialize(Archive &ar) {
    ar(unique_, domain_id_);
  }*/
};

/** Uniquely identify a task state */
using TaskStateId = UniqueId<1>;
/** Uniquely identify a queue */
using QueueId = UniqueId<2>;

/** Allow unique ids to be printed as strings */
template<int num>
HSHM_ALWAYS_INLINE
std::ostream &operator<<(std::ostream &os, UniqueId<num> const &obj) {
  return os << (std::to_string(obj.node_id_) + "."
      + std::to_string(obj.unique_));
}

}  // namespace labstor

namespace std {
template <int TYPE>
struct hash<labstor::UniqueId<TYPE>> {
  HSHM_ALWAYS_INLINE
  std::size_t operator()(const labstor::UniqueId<TYPE> &key) const {
    return
      std::hash<labstor::u64>{}(key.unique_) +
        std::hash<labstor::u32>{}(key.node_id_);
  }
};
}  // namespace std

#endif  // LABSTOR_INCLUDE_LABSTOR_LABSTOR_TYPES_H_
