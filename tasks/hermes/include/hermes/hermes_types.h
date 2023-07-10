//
// Created by lukemartinlogan on 7/8/23.
//

#ifndef LABSTOR_TASKS_HERMES_INCLUDE_HERMES_HERMES_TYPES_H_
#define LABSTOR_TASKS_HERMES_INCLUDE_HERMES_HERMES_TYPES_H_

#include "labstor/labstor_types.h"
#include "labstor/api/labstor_client.h"
#include "status.h"

namespace hermes {

using labstor::TaskMethod;
using labstor::UniqueId;
using labstor::TaskStateId;
using labstor::DomainId;
using labstor::Task;

/** Queue id */
using labstor::QueueId;

/** Queue for interprocess-communication */
using labstor::MultiQueue;

/** Unique blob id */
typedef UniqueId<3> BlobId;

/** Unique bucket id */
typedef UniqueId<4> BucketId;

/** Represents a tag */
typedef UniqueId<5> TagId;

/** Represetnts a storage target */
typedef TaskStateId TargetId;

/** Represents a trait */
typedef TaskStateId TraitId;

/** Represents a blob  */
typedef hshm::charbuf Blob;

/** Represents an allocated fraction of a target */
struct BufferInfo {
  TargetId tid_;        /**< The destination target */
  size_t t_slab_;          /**< The index of the slab in the target */
  size_t t_off_;        /**< Offset in the target */
  size_t t_size_;       /**< Size in the target */
  size_t blob_off_;     /**< Offset in the blob */
  size_t blob_size_;    /**< The amount of the blob being placed */

  /** Default constructor */
  BufferInfo() = default;

  /** Primary constructor */
  BufferInfo(TargetId tid, size_t t_off, size_t t_size,
             size_t blob_off, size_t blob_size)
      : tid_(tid), t_off_(t_off), t_size_(t_size),
        blob_off_(blob_off), blob_size_(blob_size) {}

  /** Copy constructor */
  BufferInfo(const BufferInfo &other) {
    Copy(other);
  }

  /** Move constructor */
  BufferInfo(BufferInfo &&other) {
    Copy(other);
  }

  /** Copy assignment */
  BufferInfo& operator=(const BufferInfo &other) {
    Copy(other);
    return *this;
  }

  /** Move assignment */
  BufferInfo& operator=(BufferInfo &&other) {
    Copy(other);
    return *this;
  }

  /** Performs move/copy */
  void Copy(const BufferInfo &other) {
    tid_ = other.tid_;
    t_slab_ = other.t_slab_;
    t_off_ = other.t_off_;
    t_size_ = other.t_size_;
    blob_off_ = other.blob_off_;
    blob_size_ = other.blob_size_;
  }
};

/** Hermes API call context */
struct Context {
  /** The blob's score */
  float blob_score_;

  Context() {}
};

/** Supported data placement policies */
enum class PlacementPolicy {
  kRandom,         /**< Random blob placement */
  kRoundRobin,     /**< Round-Robin (around devices) blob placement */
  kMinimizeIoTime, /**< LP-based blob placement, minimize I/O time */
  kNone,           /**< No DPE for cases we want it disabled */
};

/** A class to convert placement policy enum value to string */
class PlacementPolicyConv {
 public:
  /** A function to return string representation of \a policy */
  static std::string to_str(PlacementPolicy policy) {
    switch (policy) {
      case PlacementPolicy::kRandom: {
        return "PlacementPolicy::kRandom";
      }
      case PlacementPolicy::kRoundRobin: {
        return "PlacementPolicy::kRoundRobin";
      }
      case PlacementPolicy::kMinimizeIoTime: {
        return "PlacementPolicy::kMinimizeIoTime";
      }
      case PlacementPolicy::kNone: {
        return "PlacementPolicy::kNone";
      }
    }
    return "PlacementPolicy::Invalid";
  }

  /** return enum value of \a policy  */
  static PlacementPolicy to_enum(const std::string &policy) {
    if (policy.find("Random") != std::string::npos) {
      return PlacementPolicy::kRandom;
    } else if (policy.find("RoundRobin") != std::string::npos) {
      return PlacementPolicy::kRoundRobin;
    } else if (policy.find("MinimizeIoTime") != std::string::npos) {
      return PlacementPolicy::kMinimizeIoTime;
    } else if (policy.find("None") != std::string::npos) {
      return PlacementPolicy::kNone;
    }
    return PlacementPolicy::kNone;
  }
};

}  // namespace hermes

#endif  // LABSTOR_TASKS_HERMES_INCLUDE_HERMES_HERMES_TYPES_H_
