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
#include "labstor/api/labstor_client.h"
#include "labstor/labstor_namespace.h"

namespace hermes::blob_mdm {

#include "hermes_blob_mdm_methods.h"

using labstor::Task;
using labstor::TaskFlags;
using labstor::DataTransfer;

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
  /** SHM default constructor */
  HSHM_ALWAYS_INLINE explicit
  ConstructTask(hipc::Allocator *alloc) : CreateTaskStateTask(alloc) {}

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
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
  /** SHM default constructor */
  HSHM_ALWAYS_INLINE explicit
  DestructTask(hipc::Allocator *alloc) : DestroyTaskStateTask(alloc) {}

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
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

/**
 * Get \a blob_name BLOB from \a bkt_id bucket
 * */
struct GetOrCreateBlobIdTask : public Task, TaskFlags<TF_SRL_SYM> {
  IN TagId tag_id_;
  IN hipc::ShmArchive<hipc::charbuf> blob_name_;
  OUT BlobId blob_id_;

  /** SHM default constructor */
  HSHM_ALWAYS_INLINE explicit
  GetOrCreateBlobIdTask(hipc::Allocator *alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
  GetOrCreateBlobIdTask(hipc::Allocator *alloc,
                        const TaskNode &task_node,
                        const DomainId &domain_id,
                        const TaskStateId &state_id,
                        const TagId &tag_id,
                        const hshm::charbuf &blob_name) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = std::hash<hshm::charbuf>{}(blob_name);
    task_state_ = state_id;
    method_ = Method::kGetOrCreateBlobId;
    task_flags_.SetBits(TASK_LOW_LATENCY);
    domain_id_ = domain_id;

    // Custom
    tag_id_ = tag_id;
    HSHM_MAKE_AR(blob_name_, LABSTOR_CLIENT->main_alloc_, blob_name)
  }

  /** Destructor */
  ~GetOrCreateBlobIdTask() {
    HSHM_DESTROY_AR(blob_name_)
  }

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    task_serialize<Ar>(ar);
    ar(tag_id_, blob_name_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(u32 replica, Ar &ar) {
    ar(blob_id_);
  }
};

/** A task to put data in a blob */
struct PutBlobTask : public Task, TaskFlags<TF_SRL_ASYM_START | TF_SRL_SYM_END> {
  IN TagId tag_id_;
  IN hipc::ShmArchive<hipc::charbuf> blob_name_;
  IN size_t blob_off_;
  IN size_t data_size_;
  IN hipc::Pointer data_;
  IN float score_;
  IN bitfield32_t flags_;
  IN BlobId blob_id_;
  TEMP bool did_create_;
  TEMP int phase_;
  TEMP int plcmnt_idx_;
  TEMP int sub_plcmnt_idx_;
  TEMP hermes::bdev::AllocTask *cur_bdev_alloc_;
  TEMP hipc::ShmArchive<std::vector<PlacementSchema>> schema_;
  TEMP hipc::ShmArchive<std::vector<hermes::bdev::WriteTask*>> bdev_writes_;

  /** SHM default constructor */
  HSHM_ALWAYS_INLINE explicit
  PutBlobTask(hipc::Allocator *alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
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
    lane_hash_ = blob_id_.unique_;
    task_state_ = state_id;
    method_ = Method::kPutBlob;
    task_flags_.SetBits(TASK_LOW_LATENCY | TASK_FIRE_AND_FORGET);
    domain_id_ = domain_id;

    HILOG(kInfo, "PUT BLOB START: {} {} bytes", blob_name_->str(), data_size);

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
    sub_plcmnt_idx_ = 0;
  }

  /** Destructor */
  ~PutBlobTask() {
    HSHM_DESTROY_AR(blob_name_);
    // TODO(llogan): add this back. Double free because remote_queue owns the data. Need to add a flag indicating
    // data ownership
    // LABSTOR_CLIENT->FreeBuffer(data_);
  }

  /** (De)serialize message call */
  template<typename Ar>
  void SaveStart(Ar &ar) {
    HILOG(kInfo, "PUT BLOB SERIALIZE: {}", blob_name_->str());
    DataTransfer xfer(DT_RECEIVER_READ,
                      HERMES_MEMORY_MANAGER->Convert<char>(data_),
                      data_size_, domain_id_);
    task_serialize<Ar>(ar);
    ar & xfer;
    ar(tag_id_, blob_name_, blob_id_, blob_off_, data_size_, score_, flags_);
  }

  /** Deserialize message call */
  template<typename Ar>
  void LoadStart(Ar &ar) {
    DataTransfer xfer;
    task_serialize<Ar>(ar);
    ar & xfer;
    data_ = HERMES_MEMORY_MANAGER->Convert<void, hipc::Pointer>(xfer.data_);
    ar(tag_id_, blob_name_, blob_id_, blob_off_, data_size_, score_, flags_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(u32 replica, Ar &ar) {}
};

/** Phases for the get task */
class GetBlobPhase {
 public:
  TASK_METHOD_T kStart = 0;
  TASK_METHOD_T kWait = 1;
};

/** A task to get data from a blob */
struct GetBlobTask : public Task, TaskFlags<TF_SRL_ASYM_START | TF_SRL_SYM_END> {
  IN BlobId blob_id_;
  IN size_t blob_off_;
  IN hipc::Pointer data_;
  INOUT ssize_t data_size_;
  TEMP int phase_;
  TEMP hipc::ShmArchive<std::vector<bdev::ReadTask*>> bdev_reads_;

  /** SHM default constructor */
  HSHM_ALWAYS_INLINE explicit
  GetBlobTask(hipc::Allocator *alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
  GetBlobTask(hipc::Allocator *alloc,
              const TaskNode &task_node,
              const DomainId &domain_id,
              const TaskStateId &state_id,
              const BlobId &blob_id,
              size_t off,
              ssize_t data_size,
              hipc::Pointer &data) : Task(alloc) {
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
    data_size_ = data_size;
    data_ = data;
  }

  /** (De)serialize message call */
  template<typename Ar>
  void SaveStart(Ar &ar) {
    // TODO(llogan): Make it so Get takes as input a buffer, instead of returning one
    DataTransfer xfer(DT_RECEIVER_WRITE,
                      HERMES_MEMORY_MANAGER->Convert<char>(data_),
                      data_size_, domain_id_);
    task_serialize<Ar>(ar);
    ar & xfer;
    ar(blob_id_, blob_off_, data_size_);
  }

  /** Deserialize message call */
  template<typename Ar>
  void LoadStart(Ar &ar) {
    DataTransfer xfer;
    task_serialize<Ar>(ar);
    ar & xfer;
    data_ = HERMES_MEMORY_MANAGER->Convert<void, hipc::Pointer>(xfer.data_);
    ar(blob_id_, blob_off_, data_size_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(u32 replica, Ar &ar) {
    ar(blob_id_);
  }
};

/** A task to tag a blob */
struct TagBlobTask : public Task, TaskFlags<TF_SRL_SYM> {
  IN BlobId blob_id_;
  IN TagId tag_;

  /** SHM default constructor */
  HSHM_ALWAYS_INLINE explicit
  TagBlobTask(hipc::Allocator *alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
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

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    task_serialize<Ar>(ar);
    ar(blob_id_, tag_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(u32 replica, Ar &ar) {
  }
};

/**
 * Check if blob has a tag
 * */
struct BlobHasTagTask : public Task, TaskFlags<TF_SRL_SYM> {
  IN BlobId blob_id_;
  IN TagId tag_;
  OUT bool has_tag_;

  /** SHM default constructor */
  HSHM_ALWAYS_INLINE explicit
  BlobHasTagTask(hipc::Allocator *alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
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

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    task_serialize<Ar>(ar);
    ar(blob_id_, tag_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(u32 replica, Ar &ar) {
    ar(has_tag_);
  }
};

/**
 * Get \a blob_name BLOB from \a bkt_id bucket
 * */
struct GetBlobIdTask : public Task, TaskFlags<TF_SRL_SYM> {
  IN TagId tag_id_;
  IN hipc::ShmArchive<hipc::charbuf> blob_name_;
  OUT BlobId blob_id_;

  /** SHM default constructor */
  HSHM_ALWAYS_INLINE explicit
  GetBlobIdTask(hipc::Allocator *alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
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

  /** Destructor */
  ~GetBlobIdTask() {
    HSHM_DESTROY_AR(blob_name_)
  }

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    task_serialize<Ar>(ar);
    ar(blob_id_, blob_name_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(u32 replica, Ar &ar) {
    ar(blob_id_);
  }
};

/**
 * Get \a blob_name BLOB name from \a blob_id BLOB id
 * */
struct GetBlobNameTask : public Task, TaskFlags<TF_SRL_SYM> {
  IN BlobId blob_id_;
  OUT hipc::ShmArchive<hipc::string> blob_name_;

  /** SHM default constructor */
  HSHM_ALWAYS_INLINE explicit
  GetBlobNameTask(hipc::Allocator *alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
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

  /** Destructor */
  ~GetBlobNameTask() {
    HSHM_DESTROY_AR(blob_name_)
  };

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    task_serialize<Ar>(ar);
    ar(blob_id_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(u32 replica, Ar &ar) {
    ar(blob_name_);
  }
};

/** Get \a score from \a blob_id BLOB id */
struct GetBlobSizeTask : public Task, TaskFlags<TF_SRL_SYM> {
  IN BlobId blob_id_;
  OUT size_t size_;

  /** SHM default constructor */
  HSHM_ALWAYS_INLINE explicit
  GetBlobSizeTask(hipc::Allocator *alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
  GetBlobSizeTask(hipc::Allocator *alloc,
                   const TaskNode &task_node,
                   const DomainId &domain_id,
                   const TaskStateId &state_id,
                   const BlobId &blob_id) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = blob_id.unique_;
    task_state_ = state_id;
    method_ = Method::kGetBlobSize;
    task_flags_.SetBits(TASK_LOW_LATENCY);
    domain_id_ = domain_id;

    // Custom
    blob_id_ = blob_id;
  }

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    task_serialize<Ar>(ar);
    ar(blob_id_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(u32 replica, Ar &ar) {
    ar(size_);
  }
};

/** Get \a score from \a blob_id BLOB id */
struct GetBlobScoreTask : public Task, TaskFlags<TF_SRL_SYM> {
  IN BlobId blob_id_;
  OUT float score_;

  /** SHM default constructor */
  HSHM_ALWAYS_INLINE explicit
  GetBlobScoreTask(hipc::Allocator *alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
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

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    task_serialize<Ar>(ar);
    ar(blob_id_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(u32 replica, Ar &ar) {
    ar(score_);
  }
};

/** Get \a blob_id blob's buffers */
struct GetBlobBuffersTask : public Task, TaskFlags<TF_SRL_SYM> {
  IN BlobId blob_id_;
  OUT hipc::ShmArchive<hipc::vector<BufferInfo>> buffers_;

  /** SHM default constructor */
  HSHM_ALWAYS_INLINE explicit
  GetBlobBuffersTask(hipc::Allocator *alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
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

  /** Destructor */
  ~GetBlobBuffersTask() {
    HSHM_DESTROY_AR(buffers_)
  }

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    task_serialize<Ar>(ar);
    ar(blob_id_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(u32 replica, Ar &ar) {
    ar(buffers_);
  }
};

/**
 * Rename \a blob_id blob to \a new_blob_name new blob name
 * in \a bkt_id bucket.
 * */
struct RenameBlobTask : public Task, TaskFlags<TF_SRL_SYM> {
  IN BlobId blob_id_;
  IN hipc::ShmArchive<hipc::charbuf> new_blob_name_;

  /** SHM default constructor */
  HSHM_ALWAYS_INLINE explicit
  RenameBlobTask(hipc::Allocator *alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
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

  /** Destructor */
  ~RenameBlobTask() {
    HSHM_DESTROY_AR(new_blob_name_)
  }

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    task_serialize<Ar>(ar);
    ar(blob_id_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(u32 replica, Ar &ar) {
    ar(new_blob_name_);
  }
};

/** A task to truncate a blob */
struct TruncateBlobTask : public Task, TaskFlags<TF_SRL_SYM> {
  IN BlobId blob_id_;
  IN u64 size_;

  /** SHM default constructor */
  HSHM_ALWAYS_INLINE explicit
  TruncateBlobTask(hipc::Allocator *alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
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

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    task_serialize<Ar>(ar);
    ar(blob_id_, size_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(u32 replica, Ar &ar) {
  }
};

/** Phases of the destroy blob task */
struct DestroyBlobPhase {
  TASK_METHOD_T kFreeBuffers = 0;
  TASK_METHOD_T kWaitFreeBuffers = 1;
};

/** A task to destroy a blob */
struct DestroyBlobTask : public Task, TaskFlags<TF_SRL_SYM> {
  IN BlobId blob_id_;
  TEMP int phase_;
  TEMP hipc::ShmArchive<std::vector<bdev::FreeTask *>> free_tasks_;
  TEMP BlobInfo *blob_info_;

  /** SHM default constructor */
  HSHM_ALWAYS_INLINE explicit
  DestroyBlobTask(hipc::Allocator *alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
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

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    task_serialize<Ar>(ar);
    ar(blob_id_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(u32 replica, Ar &ar) {
  }
};

}  // namespace hermes::blob_mdm

#endif //LABSTOR_TASKS_HERMES_BLOB_MDM_INCLUDE_HERMES_BLOB_MDM_HERMES_BLOB_MDM_TASKS_H_
