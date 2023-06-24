//
// Created by lukemartinlogan on 6/22/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_LABSTOR_DATA_STRUCTURES_H_
#define LABSTOR_INCLUDE_LABSTOR_LABSTOR_DATA_STRUCTURES_H_

#include <hermes_shm/data_structures/ipc/unordered_map.h>
#include <hermes_shm/data_structures/ipc/vector.h>
#include <hermes_shm/data_structures/ipc/list.h>
#include <hermes_shm/data_structures/ipc/slist.h>
#include <hermes_shm/data_structures/data_structure.h>
#include <hermes_shm/data_structures/ipc/string.h>
#include <hermes_shm/data_structures/ipc/mpsc_queue.h>
#include <hermes_shm/data_structures/containers/charbuf.h>
#include <hermes_shm/data_structures/containers/converters.h>
#include <hermes_shm/util/auto_trace.h>
#include <hermes_shm/thread/lock.h>
#include <hermes_shm/thread/thread_model_manager.h>
#include <hermes_shm/types/atomic.h>
#include "hermes_shm/util/singleton.h"

namespace labstor {

using hshm::RwLock;
using hshm::Mutex;
using hshm::bitfield32_t;
using hshm::ScopedRwReadLock;
using hshm::ScopedRwWriteLock;

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

/** Represents unique ID for BlobId and TagId */
template<int TYPE>
struct UniqueId {
  u64 unique_;   /**< A unique id for the blob */
  u32 node_id_;  /**< The node the content is on */

  /** Default constructor */
  UniqueId() = default;

  /** Emplace constructor */
  UniqueId(u64 unique, i32 node_id) : unique_(unique), node_id_(node_id) {}

  /** Copy constructor */
  UniqueId(const UniqueId &other) {
    unique_ = other.unique_;
    node_id_ = other.node_id_;
  }

  /** Copy assignment */
  UniqueId& operator=(const UniqueId &other) {
    if (this != &other) {
      unique_ = other.unique_;
      node_id_ = other.node_id_;
    }
    return *this;
  }

  /** Move constructor */
  UniqueId(UniqueId &&other) noexcept {
    unique_ = other.unique_;
    node_id_ = other.node_id_;
  }

  /** Move assignment */
  UniqueId& operator=(UniqueId &&other) noexcept {
    if (this != &other) {
      unique_ = other.unique_;
      node_id_ = other.node_id_;
    }
    return *this;
  }

  /** Check if null */
  [[nodiscard]] bool IsNull() const { return unique_ == 0; }

  /** Get null id */
  static inline UniqueId GetNull() {
    static const UniqueId id(0, 0);
    return id;
  }

  /** Set to null id */
  void SetNull() {
    node_id_ = 0;
    unique_ = 0;
  }

  /** Get id of node from this id */
  [[nodiscard]] u32 GetNodeId() const { return node_id_; }

  /** Compare two ids for equality */
  bool operator==(const UniqueId &other) const {
    return unique_ == other.unique_ && node_id_ == other.node_id_;
  }

  /** Compare two ids for inequality */
  bool operator!=(const UniqueId &other) const {
    return unique_ != other.unique_ || node_id_ != other.node_id_;
  }

  /** Serialize a UniqueId
  template<class Archive>
  void serialize(Archive &ar) {
    ar(unique_, node_id_);
  }*/
};

/** Uniquely identify a distributed task */
using TaskExecId = UniqueId<1>;

}  // namespace labstor

namespace std {
template <int TYPE>
struct hash<labstor::UniqueId<TYPE>> {
  std::size_t operator()(const labstor::UniqueId<TYPE> &key) const {
    return
      std::hash<labstor::u64>{}(key.unique_) +
        std::hash<labstor::i32>{}(key.node_id_);
  }
};
}  // namespace std

#endif  // LABSTOR_INCLUDE_LABSTOR_LABSTOR_DATA_STRUCTURES_H_
