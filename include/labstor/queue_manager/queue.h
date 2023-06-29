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

typedef hipc::mpsc_queue<hipc::Pointer> Lane;

/**
 * The shared-memory representation of a Queue
 * */
struct MultiQueueShm : public hipc::ShmContainer {
 SHM_CONTAINER_TEMPLATE((MultiQueueShm), (MultiQueueShm))
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

  /** SHM constructor. */
  explicit MultiQueueShm(hipc::Allocator *alloc, QueueId id,
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
  explicit MultiQueueShm(hipc::Allocator *alloc, const MultiQueueShm &other) {
    shm_init_container(alloc);
    SetNull();
    shm_strong_copy_construct_and_op(other);
  }

  /** SHM copy assignment operator */
  MultiQueueShm& operator=(const MultiQueueShm &other) {
    if (this != &other) {
      shm_destroy();
      shm_strong_copy_construct_and_op(other);
    }
    return *this;
  }

  /** SHM copy constructor + operator main */
  void shm_strong_copy_construct_and_op(const MultiQueueShm &other) {
    (*lanes_) = (*other.lanes_);
  }

  /**====================================
   * Move Constructors
   * ===================================*/

  /** SHM move constructor. */
  MultiQueueShm(hipc::Allocator *alloc,
                MultiQueueShm &&other) noexcept {
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
  MultiQueueShm& operator=(MultiQueueShm &&other) noexcept {
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

  Lane& GetLane(u32 lane_id) {
    return (*lanes_)[lane_id];
  }
};

/**
 * A shared-memory, scalable concurrent queue
 * Update requests are stored in a single lane
 * Read & Write requests are stored in a separate pool of lanes
 *
 * Resize is assumed to be called sequentially on a single thread.
 * */
class MultiQueue {
 public:
  hipc::uptr<MultiQueueShm> queue_;  /**< SHM representation of multi-queue */
  std::vector<Lane*> lanes_;  /**< Lanes for storing TASK_READ & TASK_WRITE */
  hipc::uptr<Lane> update_lane_shm_;  /** SHM representation of update lane */
  Lane *update_lane_;  /**< Lane used for storing TASK_UPDATE */

 public:
  /** Construct queue */
  MultiQueue(hipc::Allocator *alloc, QueueId id,
             u32 max_lanes, u32 num_lanes,
             u32 depth, bitfield32_t flags) {
    queue_ = hipc::make_uptr<MultiQueueShm>(
      alloc, id, max_lanes, num_lanes, depth, flags);
    CacheLanes();
  }

  /** Attach to existing queue */
  MultiQueue(hipc::Pointer p) {
    queue_ << p;
    CacheLanes();
  }

 private:
  /** Cache lanes */
  void CacheLanes() {
    lanes_.reserve(queue_->max_lanes_);
    for (u32 lane_id = 0; lane_id < queue_->num_lanes_; ++lane_id) {
      lanes_.emplace_back(&queue_->GetLane(lane_id));
    }
  }

 public:
  /**
   * Change the number of active lanes
   * This assumes that PlugForResize and UnplugForResize are called externally.
   * */
  void Resize(u32 num_lanes) {
  }

  /** Emplace a SHM pointer to a task */
  bool Emplace(u32 hash, hipc::Pointer &p) {
    if (IsEmplacePlugged()) {
      WaitForEmplacePlug();
    }
    u32 lane_id = hash % lanes_.size();
    Lane *lane = lanes_[lane_id];
    hshm::qtok_t ret = lane->emplace(p);
    return !ret.IsNull();
  }

  /** Pop a regular pointer to a task */
  bool Pop(u32 lane_id, Task *&task) {
    Lane *lane = lanes_[lane_id];
    hipc::Pointer p;
    hshm::qtok_t ret = lane->pop(p);
    if (ret.IsNull()) {
      return false;
    }
    task = HERMES_MEMORY_MANAGER->Convert<Task>(p);
    return true;
  }

  /** Begin plugging the queue for resize */
  bool PlugForResize() {
    // Mark this queue as QUEUE_RESIZE
    if (!queue_->flags_.Any(QUEUE_RESIZE)) {
      queue_->flags_.SetBits(QUEUE_RESIZE);
    }
    // Check if all lanes have been marked QUEUE_RESIZE
    for (Lane *lane : lanes_) {
      if (!lane->flags_.Any(QUEUE_RESIZE)) {
        return false;
      }
    }
    return true;
  }

  /** Begin plugging the queue for update tasks */
  bool PlugForUpdateTask() {
    // Mark this queue as QUEUE_UPDATE
    queue_->flags_.SetBits(QUEUE_UPDATE);
    // Check if all lanes have been marked QUEUE_UPDATE
    for (Lane *lane : lanes_) {
      if (!lane->flags_.Any(QUEUE_UPDATE)) {
        return false;
      }
    }
    return true;
  }

  /** Check if emplace operations are plugged */
  bool IsEmplacePlugged() {
    return queue_->flags_.Any(QUEUE_RESIZE);
  }

  /** Check if pop operations are plugged */
  bool IsPopPlugged() {
    return queue_->flags_.Any(QUEUE_UPDATE | QUEUE_RESIZE);
  }

  /** Wait for emplace plug to complete */
  void WaitForEmplacePlug() {
    // NOTE(llogan): will this infinite loop due to CPU caching?
    while (queue_->flags_.Any(QUEUE_UPDATE)) {
      HERMES_THREAD_MODEL->Yield();
    }
  }

  /** Enable emplace & pop */
  void UnplugForResize() {
    queue_->flags_.UnsetBits(QUEUE_RESIZE);
  }

  /** Enable pop */
  void UnplugForUpdateTask() {
    queue_->flags_.UnsetBits(QUEUE_UPDATE);
  }
};

}  // namespace labstor

#endif  // LABSTOR_INCLUDE_LABSTOR_QUEUE_MANAGER_QUEUE_H_
