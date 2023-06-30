//
// Created by lukemartinlogan on 6/27/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_QUEUE_MANAGER_QUEUE_H_
#define LABSTOR_INCLUDE_LABSTOR_QUEUE_MANAGER_QUEUE_H_

#include "labstor/labstor_types.h"
#include "labstor/task_registry/task.h"
#include <vector>

/** This queue contains only latency-sensitive tasks */
#define QUEUE_LOW_LATENCY (1 << 0)
/** This queue is currently being resized */
#define QUEUE_RESIZE (1 << 1)
/** This queue is currently processing updates */
#define QUEUE_UPDATE (1 << 2)

namespace labstor {

/** Represents a lane tasks can be stored */
typedef hipc::mpsc_queue<hipc::Pointer> Lane;

/**
 * The shared-memory representation of a Queue
 * */
struct MultiQueue : public hipc::ShmContainer {
 SHM_CONTAINER_TEMPLATE((MultiQueue), (MultiQueue))
  QueueId id_;
  bitfield32_t flags_;
  u32 max_lanes_;
  u32 num_lanes_;
  u32 depth_;
  hipc::ShmArchive<hipc::vector<Lane>> lanes_;

 public:
  /**====================================
   * Constructor
   * ===================================*/

  /** SHM constructor. Default. */
  explicit MultiQueue(hipc::Allocator *alloc) {
    shm_init_container(alloc);
    SetNull();
  }

  /** SHM constructor. */
  explicit MultiQueue(hipc::Allocator *alloc, const QueueId &id,
                      u32 max_lanes, u32 num_lanes,
                      u32 depth, bitfield32_t flags) {
    shm_init_container(alloc);
    id_ = id;
    max_lanes_ = max_lanes;
    num_lanes_ = num_lanes;
    depth_ = depth;
    flags_ = flags;
    HSHM_MAKE_AR0(lanes_, GetAllocator());
    lanes_->reserve(max_lanes_);
    for (u32 lane_id = 0; lane_id < num_lanes; ++lane_id) {
      lanes_->emplace_back(depth);
    }
    flags_.Clear();
    SetNull();
  }

  /**====================================
   * Copy Constructors
   * ===================================*/

  /** SHM copy constructor */
  explicit MultiQueue(hipc::Allocator *alloc, const MultiQueue &other) {
    shm_init_container(alloc);
    SetNull();
    shm_strong_copy_construct_and_op(other);
  }

  /** SHM copy assignment operator */
  MultiQueue& operator=(const MultiQueue &other) {
    if (this != &other) {
      shm_destroy();
      shm_strong_copy_construct_and_op(other);
    }
    return *this;
  }

  /** SHM copy constructor + operator main */
  void shm_strong_copy_construct_and_op(const MultiQueue &other) {
    (*lanes_) = (*other.lanes_);
  }

  /**====================================
   * Move Constructors
   * ===================================*/

  /** SHM move constructor. */
  MultiQueue(hipc::Allocator *alloc,
             MultiQueue &&other) noexcept {
    shm_init_container(alloc);
    if (GetAllocator() == other.GetAllocator()) {
      (*lanes_) = std::move(*other.lanes_);
      other.SetNull();
    } else {
      shm_strong_copy_construct_and_op(other);
      other.shm_destroy();
    }
  }

  /** SHM move assignment operator. */
  MultiQueue& operator=(MultiQueue &&other) noexcept {
    if (this != &other) {
      shm_destroy();
      if (GetAllocator() == other.GetAllocator()) {
        (*lanes_) = std::move(*other.lanes_);
        other.SetNull();
      } else {
        shm_strong_copy_construct_and_op(other);
        other.shm_destroy();
      }
    }
    return *this;
  }

  /**====================================
   * Destructor
   * ===================================*/

  /** SHM destructor.  */
  void shm_destroy_main() {
    (*lanes_).shm_destroy();
  }

  /** Check if the list is empty */
  bool IsNull() const {
    return (*lanes_).IsNull();
  }

  /** Sets this list as empty */
  void SetNull() {}

  /**====================================
   * Helpers
   * ===================================*/

  /** Get a lane of the queue */
  Lane& GetLane(u32 lane_id) {
    return (*lanes_)[lane_id];
  }

  /** Allocate a task */
  template<typename T, typename ...Args>
  static T* Allocate(hipc::Allocator *alloc, hipc::Pointer &p, Args&& ...args) {
    return alloc->AllocateConstructObjs<T>(1, p, alloc, std::forward<Args>(args)...);
  }

  /** Free a task */
  void Free(hipc::Allocator *alloc, hipc::Pointer &p) {
    alloc->Free(p);
  }

  /** Emplace a SHM pointer to a task */
  bool Emplace(u32 hash, hipc::Pointer &p) {
    if (IsEmplacePlugged()) {
      WaitForEmplacePlug();
    }
    hipc::vector<Lane> *lanes = lanes_.get();
    u32 lane_id = hash % lanes->size();
    Lane &lane = (*lanes)[lane_id];
    hshm::qtok_t ret = lane.emplace(p);
    return !ret.IsNull();
  }

  /** Pop a regular pointer to a task */
  bool Pop(u32 lane_id, Task *&task) {
    Lane &lane = (*lanes_)[lane_id];
    hipc::Pointer p;
    hshm::qtok_t ret = lane.pop(p);
    if (ret.IsNull()) {
      return false;
    }
    task = HERMES_MEMORY_MANAGER->Convert<Task>(p);
    return true;
  }

  /**
   * Change the number of active lanes
   * This assumes that PlugForResize and UnplugForResize are called externally.
   * */
  void Resize(u32 num_lanes) {
    hipc::vector<Lane> *lanes = lanes_.get();
    if (num_lanes > max_lanes_) {
      num_lanes = max_lanes_;
    }
    if (num_lanes < num_lanes_) {
      // Remove lanes
      for (u32 lane_id = num_lanes; lane_id < num_lanes_; ++lane_id) {
        lanes->erase(lanes->begin() + lane_id);
      }
    } else if (num_lanes > num_lanes_) {
      // Add lanes
      for (u32 lane_id = num_lanes_; lane_id < num_lanes; ++lane_id) {
        lanes->emplace_back(depth_);
      }
    }
    num_lanes_ = num_lanes;
  }

  /** Begin plugging the queue for resize */
  bool PlugForResize() {
    // Mark this queue as QUEUE_RESIZE
    if (!flags_.Any(QUEUE_RESIZE)) {
      flags_.SetBits(QUEUE_RESIZE);
    }
    // Check if all lanes have been marked QUEUE_RESIZE
    for (Lane &lane : *lanes_) {
      if (!lane.flags_.Any(QUEUE_RESIZE)) {
        return false;
      }
    }
    return true;
  }

  /** Begin plugging the queue for update tasks */
  bool PlugForUpdateTask() {
    // Mark this queue as QUEUE_UPDATE
    flags_.SetBits(QUEUE_UPDATE);
    // Check if all lanes have been marked QUEUE_UPDATE
    for (Lane &lane : *lanes_) {
      if (!lane.flags_.Any(QUEUE_UPDATE)) {
        return false;
      }
    }
    return true;
  }

  /** Check if emplace operations are plugged */
  bool IsEmplacePlugged() {
    return flags_.Any(QUEUE_RESIZE);
  }

  /** Check if pop operations are plugged */
  bool IsPopPlugged() {
    return flags_.Any(QUEUE_UPDATE | QUEUE_RESIZE);
  }

  /** Wait for emplace plug to complete */
  void WaitForEmplacePlug() {
    // NOTE(llogan): will this infinite loop due to CPU caching?
    while (flags_.Any(QUEUE_UPDATE)) {
      HERMES_THREAD_MODEL->Yield();
    }
  }

  /** Enable emplace & pop */
  void UnplugForResize() {
    flags_.UnsetBits(QUEUE_RESIZE);
  }

  /** Enable pop */
  void UnplugForUpdateTask() {
    flags_.UnsetBits(QUEUE_UPDATE);
  }
};

}  // namespace labstor

#endif  // LABSTOR_INCLUDE_LABSTOR_QUEUE_MANAGER_QUEUE_H_
