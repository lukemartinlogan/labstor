//
// Created by lukemartinlogan on 8/14/23.
//

#ifndef LABSTOR_TASKS_HERMES_BLOB_MDM_INCLUDE_HERMES_BLOB_MDM_HERMES_BLOB_MDM_TASKS_H_
#define LABSTOR_TASKS_HERMES_BLOB_MDM_INCLUDE_HERMES_BLOB_MDM_HERMES_BLOB_MDM_TASKS_H_

#include "labstor/api/labstor_client.h"
#include "labstor/task_registry/task_lib.h"
#include "labstor_admin/labstor_admin.h"
#include "labstor/queue_manager/queue_manager_client.h"
#include "hermes/hermes_types.h"
#include "bdev/bdev.h"

namespace hermes::blob_mdm {

/** The set of methods in the hermes_blob_mdm task */
struct Method : public TaskMethod {
  TASK_METHOD_T kPutBlob = TaskMethod::kLast + 0;
  TASK_METHOD_T kGetBlob = TaskMethod::kLast + 1;
  TASK_METHOD_T kTruncateBlob = TaskMethod::kLast + 2;
  TASK_METHOD_T kDestroyBlob = TaskMethod::kLast + 3;
  TASK_METHOD_T kTagBlob = TaskMethod::kLast + 4;
  TASK_METHOD_T kUntagBlob = TaskMethod::kLast + 5;
  TASK_METHOD_T kBlobHasTag = TaskMethod::kLast + 6;
  TASK_METHOD_T kGetBlobTags = TaskMethod::kLast + 7;
  TASK_METHOD_T kGetBlobId = TaskMethod::kLast + 8;
  TASK_METHOD_T kGetBlobName = TaskMethod::kLast + 9;
  TASK_METHOD_T kGetBlobSize = TaskMethod::kLast + 10;
  TASK_METHOD_T kGetBlobScore = TaskMethod::kLast + 11;
  TASK_METHOD_T kGetBlobBuffers = TaskMethod::kLast + 12;
  TASK_METHOD_T kRenameBlob = TaskMethod::kLast + 13;
};


/** Phases of the construct task */
using labstor::Admin::CreateTaskStatePhase;
class ConstructTaskPhase : public CreateTaskStatePhase {
 public:
  TASK_METHOD_T kCreateTaskStates = kLast + 0;
  TASK_METHOD_T kWaitForTaskStates = kLast + 1;
};

/**
 * A task to create hermes_mdm
 * */
using labstor::Admin::CreateTaskStateTask;
struct ConstructTask : public CreateTaskStateTask {
  HSHM_ALWAYS_INLINE
  ConstructTask(hipc::Allocator *alloc,
                const TaskNode &task_node,
                const DomainId &domain_id,
                const std::string &state_name,
                const TaskStateId &id,
                u32 max_lanes, u32 num_lanes,
                u32 depth, bitfield32_t flags)
      : CreateTaskStateTask(alloc, task_node, domain_id, state_name,
                            "hermes_blob_mdm", id, max_lanes,
                            num_lanes, depth, flags) {
  }
};

/** A task to destroy hermes_mdm */
using labstor::Admin::DestroyTaskStateTask;
struct DestructTask : public DestroyTaskStateTask {
  HSHM_ALWAYS_INLINE
  DestructTask(hipc::Allocator *alloc,
               const TaskNode &task_node,
               const DomainId &domain_id,
               const TaskStateId &state_id)
  : DestroyTaskStateTask(alloc, task_node, domain_id, state_id) {}
};

/**====================================
 * Blob Operations
 * ===================================*/

/** Phases for the put task */
class PutBlobPhase {
 public:
  TASK_METHOD_T kCreate = 0;
  TASK_METHOD_T kAllocate = 1;
  TASK_METHOD_T kWaitAllocate = 2;
  TASK_METHOD_T kModify = 3;
  TASK_METHOD_T kWaitModify = 4;
};

#define HERMES_BLOB_REPLACE BIT_OPT(u32, 0)
#define HERMES_BLOB_APPEND BIT_OPT(u32, 1)

/** A task to put data in a blob */
struct PutBlobTask : public Task {
  IN TagId tag_id_;
  IN hipc::ShmArchive<hipc::charbuf> blob_name_;
  IN size_t blob_off_;
  IN size_t data_size_;
  IN hipc::Pointer data_;
  IN float score_;
  IN bitfield32_t flags_;
  INOUT BlobId blob_id_;
  TEMP bool did_create_;
  TEMP int phase_;
  TEMP int plcmnt_idx_;
  TEMP int sub_plcmnt_idx_;
  TEMP hermes::bdev::AllocTask *cur_bdev_alloc_;
  TEMP hipc::ShmArchive<std::vector<PlacementSchema>> schema_;
  TEMP hipc::ShmArchive<std::vector<hermes::bdev::WriteTask *>> bdev_writes_;

  HSHM_ALWAYS_INLINE
  PutBlobTask(hipc::Allocator *alloc,
              const TaskNode &task_node,
              const DomainId &domain_id,
              const TaskStateId &state_id,
              const TagId &tag_id,
              const hshm::charbuf &blob_name,
              const BlobId &blob_id,
              size_t blob_off,
              size_t data_size,
              const hipc::Pointer &data,
              float score,
              bitfield32_t flags) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = blob_id.unique_;
    task_state_ = state_id;
    method_ = Method::kPutBlob;
    task_flags_.SetBits(TASK_LOW_LATENCY | TASK_FIRE_AND_FORGET);
    domain_id_ = domain_id;

    // Custom params
    tag_id_ = tag_id;
    HSHM_MAKE_AR(blob_name_, LABSTOR_CLIENT->main_alloc_, blob_name);
    blob_id_ = blob_id;
    blob_off_ = blob_off;
    data_size_ = data_size;
    data_ = data;
    score_ = score;
    flags_ = flags;
    phase_ = PutBlobPhase::kCreate;
    plcmnt_idx_ = 0;
  }

  ~PutBlobTask() {
    HSHM_DESTROY_AR(blob_name_);
    LABSTOR_CLIENT->FreeBuffer(data_);
  }
};

/** Phases for the get task */
class GetBlobPhase {
 public:
  TASK_METHOD_T kStart = 0;
  TASK_METHOD_T kWait = 1;
};

/** A task to get data from a blob */
struct GetBlobTask : public Task {
  IN BlobId blob_id_;
  IN size_t blob_off_;
  INOUT ssize_t data_size_;
  OUT int phase_;
  OUT hipc::ShmArchive<std::vector<bdev::ReadTask *>> bdev_reads_;
  OUT hipc::Pointer data_;

  HSHM_ALWAYS_INLINE
  GetBlobTask(hipc::Allocator *alloc,
              const TaskNode &task_node,
              const DomainId &domain_id,
              const TaskStateId &state_id,
              const BlobId &blob_id,
              size_t off,
              ssize_t size) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = blob_id.unique_;
    task_state_ = state_id;
    method_ = Method::kGetBlob;
    task_flags_.SetBits(TASK_LOW_LATENCY);
    domain_id_ = domain_id;

    // Custom params
    phase_ = GetBlobPhase::kStart;
    blob_id_ = blob_id;
    blob_off_ = off;
    data_size_ = size;
  }
};

/** A task to tag a blob */
struct TagBlobTask : public Task {
  IN BlobId blob_id_;
  IN TagId tag_;

  HSHM_ALWAYS_INLINE
  TagBlobTask(hipc::Allocator *alloc,
              const TaskNode &task_node,
              const DomainId &domain_id,
              const TaskStateId &state_id,
              const BlobId &blob_id,
              const TagId &tag) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = blob_id.unique_;
    task_state_ = state_id;
    method_ = Method::kTagBlob;
    task_flags_.SetBits(TASK_LOW_LATENCY);
    domain_id_ = domain_id;

    // Custom
    blob_id_ = blob_id;
    tag_ = tag;
  }
};

/**
 * Check if blob has a tag
 * */
struct BlobHasTagTask : public Task {
  IN BlobId blob_id_;
  IN TagId tag_;
  OUT bool has_tag_;

  HSHM_ALWAYS_INLINE
  BlobHasTagTask(hipc::Allocator *alloc,
                 const TaskNode &task_node,
                 const DomainId &domain_id,
                 const TaskStateId &state_id,
                 const BlobId &blob_id,
                 const TagId &tag) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    task_node_ = task_node;
    lane_hash_ = blob_id.unique_;
    task_state_ = state_id;
    method_ = Method::kBlobHasTag;
    task_flags_.SetBits(TASK_LOW_LATENCY);
    domain_id_ = domain_id;

    // Custom
    blob_id_ = blob_id;
    tag_ = tag;
  }
};

/**
 * Get \a blob_name BLOB from \a bkt_id bucket
 * */
struct GetBlobIdTask : public Task {
  IN TagId tag_id_;
  IN hipc::ShmArchive<hipc::charbuf> blob_name_;
  OUT BlobId blob_id_;

  HSHM_ALWAYS_INLINE
  GetBlobIdTask(hipc::Allocator *alloc,
                const TaskNode &task_node,
                const DomainId &domain_id,
                const TaskStateId &state_id,
                const TagId &tag_id,
                const hshm::charbuf &blob_name) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = std::hash<hshm::charbuf>{}(blob_name);
    task_state_ = state_id;
    method_ = Method::kGetBlobId;
    task_flags_.SetBits(TASK_LOW_LATENCY);
    domain_id_ = domain_id;

    // Custom
    tag_id_ = tag_id;
    HSHM_MAKE_AR(blob_name_, LABSTOR_CLIENT->main_alloc_, blob_name)
  }

  ~GetBlobIdTask() {
    HSHM_DESTROY_AR(blob_name_)
  }
};

/**
 * Get \a blob_name BLOB name from \a blob_id BLOB id
 * */
struct GetBlobNameTask : public Task {
  IN BlobId blob_id_;
  OUT hipc::ShmArchive<hipc::string> blob_name_;

  HSHM_ALWAYS_INLINE
  GetBlobNameTask(hipc::Allocator *alloc,
                  const TaskNode &task_node,
                  const DomainId &domain_id,
                  const TaskStateId &state_id,
                  const BlobId &blob_id) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = blob_id.unique_;
    task_state_ = state_id;
    method_ = Method::kGetBlobName;
    task_flags_.SetBits(TASK_LOW_LATENCY);
    domain_id_ = domain_id;

    // Custom
    blob_id_ = blob_id;
    HSHM_MAKE_AR0(blob_name_, LABSTOR_CLIENT->main_alloc_)
  }

  ~GetBlobNameTask() {
    HSHM_DESTROY_AR(blob_name_)
  };
};

/** Get \a score from \a blob_id BLOB id */
struct GetBlobScoreTask : public Task {
  IN BlobId blob_id_;
  OUT float score_;

  HSHM_ALWAYS_INLINE
  GetBlobScoreTask(hipc::Allocator *alloc,
                   const TaskNode &task_node,
                   const DomainId &domain_id,
                   const TaskStateId &state_id,
                   const BlobId &blob_id) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = blob_id.unique_;
    task_state_ = state_id;
    method_ = Method::kGetBlobScore;
    task_flags_.SetBits(TASK_LOW_LATENCY);
    domain_id_ = domain_id;

    // Custom
    blob_id_ = blob_id;
  }
};

/** Get \a blob_id blob's buffers */
struct GetBlobBuffersTask : public Task {
  IN BlobId blob_id_;
  OUT hipc::ShmArchive<hipc::vector<BufferInfo>> buffers_;

  HSHM_ALWAYS_INLINE
  GetBlobBuffersTask(hipc::Allocator *alloc,
                     const TaskNode &task_node,
                     const DomainId &domain_id,
                     const TaskStateId &state_id,
                     const BlobId &blob_id) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = blob_id.unique_;
    task_state_ = state_id;
    method_ = Method::kGetBlobBuffers;
    task_flags_.SetBits(TASK_LOW_LATENCY);
    domain_id_ = domain_id;

    // Custom
    blob_id_ = blob_id;
    HSHM_MAKE_AR0(buffers_, LABSTOR_CLIENT->main_alloc_)
  }

  ~GetBlobBuffersTask() {
    HSHM_DESTROY_AR(buffers_)
  }
};

/**
 * Rename \a blob_id blob to \a new_blob_name new blob name
 * in \a bkt_id bucket.
 * */
struct RenameBlobTask : public Task {
  IN BlobId blob_id_;
  IN hipc::ShmArchive<hipc::charbuf> new_blob_name_;

  HSHM_ALWAYS_INLINE
  RenameBlobTask(hipc::Allocator *alloc,
                 const TaskNode &task_node,
                 const DomainId &domain_id,
                 const TaskStateId &state_id,
                 const BlobId &blob_id,
                 const hshm::charbuf &new_blob_name) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = blob_id.unique_;
    task_state_ = state_id;
    method_ = Method::kRenameBlob;
    task_flags_.SetBits(TASK_LOW_LATENCY);
    domain_id_ = domain_id;

    // Custom
    blob_id_ = blob_id;
    HSHM_MAKE_AR(new_blob_name_, LABSTOR_CLIENT->main_alloc_, new_blob_name)
  }

  ~RenameBlobTask() {
    HSHM_DESTROY_AR(new_blob_name_)
  }
};

/** A task to truncate a blob */
struct TruncateBlobTask : public Task {
  BlobId blob_id_;
  IN u64 size_;

  HSHM_ALWAYS_INLINE
  TruncateBlobTask(hipc::Allocator *alloc,
                   const TaskNode &task_node,
                   const DomainId &domain_id,
                   const TaskStateId &state_id,
                   const BlobId &blob_id,
                   u64 size) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = blob_id.unique_;
    task_state_ = state_id;
    method_ = Method::kTruncateBlob;
    task_flags_.SetBits(TASK_LOW_LATENCY);
    domain_id_ = domain_id;

    // Custom params
    blob_id_ = blob_id;
    size_ = size;
  }
};

/** Phases of the destroy blob task */
struct DestroyBlobPhase {
  TASK_METHOD_T kFreeBuffers = 0;
  TASK_METHOD_T kWaitFreeBuffers = 1;
};

/** A task to destroy a blob */
struct DestroyBlobTask : public Task {
  IN BlobId blob_id_;
  TEMP int phase_;
  TEMP hipc::ShmArchive<std::vector<bdev::FreeTask *>> free_tasks_;
  TEMP BlobInfo *blob_info_;

  HSHM_ALWAYS_INLINE
  DestroyBlobTask(hipc::Allocator *alloc,
                  const TaskNode &task_node,
                  const DomainId &domain_id,
                  const TaskStateId &state_id,
                  const BlobId &blob_id) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = blob_id.unique_;
    task_state_ = state_id;
    method_ = Method::kDestroyBlob;
    task_flags_.SetBits(TASK_LOW_LATENCY);
    domain_id_ = domain_id;

    // Custom params
    blob_id_ = blob_id;
    phase_ = DestroyBlobPhase::kFreeBuffers;
  }
};

}  // namespace hermes::blob_mdm

#endif //LABSTOR_TASKS_HERMES_BLOB_MDM_INCLUDE_HERMES_BLOB_MDM_HERMES_BLOB_MDM_TASKS_H_
