//
// Created by lukemartinlogan on 8/14/23.
//

#ifndef LABSTOR_TASKS_HERMES_BUCKET_MDM_INCLUDE_HERMES_BUCKET_MDM_HERMES_BUCKET_MDM_TASKS_H_
#define LABSTOR_TASKS_HERMES_BUCKET_MDM_INCLUDE_HERMES_BUCKET_MDM_HERMES_BUCKET_MDM_TASKS_H_

#include "labstor/api/labstor_client.h"
#include "labstor/task_registry/task_lib.h"
#include "labstor_admin/labstor_admin.h"
#include "labstor/queue_manager/queue_manager_client.h"
#include "hermes/hermes_types.h"
#include "bdev/bdev.h"
#include "hermes_blob_mdm/hermes_blob_mdm.h"
#include "labstor/api/labstor_client.h"
#include "labstor/labstor_namespace.h"
#include "proc_queue/proc_queue.h"

namespace hermes::bucket_mdm {

#include "hermes_bucket_mdm_methods.h"
#include "labstor/labstor_namespace.h"

/** Phases of the construct task */
using labstor::Admin::CreateTaskStatePhase;
class ConstructTaskPhase : public CreateTaskStatePhase {
 public:
  TASK_METHOD_T kInit = kLast + 0;
  TASK_METHOD_T kWait = kLast + 1;
};

/**
 * A task to create hermes_bucket_mdm
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
                            "hermes_bucket_mdm", id, max_lanes,
                            num_lanes, depth, flags) {
  }

  /** Create group */
  HSHM_ALWAYS_INLINE
  u32 GetGroup(hshm::charbuf &group) {
    return TASK_UNORDERED;
  }
};

/** A task to destroy hermes_bucket_mdm */
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
               TaskStateId &state_id)
      : DestroyTaskStateTask(alloc, task_node, domain_id, state_id) {}

  /** Create group */
  HSHM_ALWAYS_INLINE
  u32 GetGroup(hshm::charbuf &group) {
    return TASK_UNORDERED;
  }
};

/** Phases for the put task */
class PutBlobPhase {
 public:
  TASK_METHOD_T kUpdateMdm = 0;
  TASK_METHOD_T kWait = 1;
};

/** Put a blob in the bucket */
struct PutBlobTask : public Task, TaskFlags<TF_SRL_SYM> {
  IN TagId tag_id_;
  IN size_t blob_off_;
  IN size_t data_size_;
  IN bitfield32_t flags_;
  IN BlobId blob_id_;

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
              const BlobId &blob_id,
              size_t blob_off,
              size_t data_size,
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
    blob_id_ = blob_id;
    blob_off_ = blob_off;
    data_size_ = data_size;
    flags_ = flags;
  }

  /** Destructor */
  ~PutBlobTask() {}

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    task_serialize<Ar>(ar);
    // TODO(llogan): make it so data xfer doesn't happen here
    ar(tag_id_, blob_id_, blob_off_, data_size_, flags_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(u32 replica, Ar &ar) {
    ar(blob_id_);
  }

  /** Create group */
  HSHM_ALWAYS_INLINE
  u32 GetGroup(hshm::charbuf &group) {
    hshm::NodeThreadId tid;
    labstor::LocalSerialize srl(group);
    srl << tag_id_.unique_;
    srl << tag_id_.node_id_;
    srl << LABSTOR_CLIENT->node_id_;
    return 0;
  }
};

/** Phases for the append task */
class AppendBlobPhase {
 public:
  TASK_METHOD_T kGetBlobIds = 0;
  TASK_METHOD_T kWaitBlobIds = 1;
  TASK_METHOD_T kWaitPutBlobs = 2;
};

/** A struct to store the  */
struct AppendInfo {
  size_t blob_off_;
  size_t data_size_;
  hshm::charbuf blob_name_;
  blob_mdm::GetOrCreateBlobIdTask *blob_id_task_;
  blob_mdm::PutBlobTask *put_task_;

  template<typename Ar>
  void serialize(Ar &ar) {
    ar(blob_off_, data_size_, blob_name_);
  }
};

/** A task to append data to a bucket */
struct AppendBlobSchemaTask : public Task, TaskFlags<TF_SRL_SYM> {
  IN TagId tag_id_;
  IN size_t data_size_;
  IN size_t page_size_;
  TEMP int phase_ = AppendBlobPhase::kGetBlobIds;
  TEMP hipc::ShmArchive<std::vector<AppendInfo>> append_info_;

  /** SHM default constructor */
  HSHM_ALWAYS_INLINE explicit
  AppendBlobSchemaTask(hipc::Allocator *alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
  AppendBlobSchemaTask(hipc::Allocator *alloc,
                       const TaskNode &task_node,
                       const DomainId &domain_id,
                       const TaskStateId &state_id,
                       const TagId &tag_id,
                       size_t data_size,
                       size_t page_size) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = tag_id.unique_;
    task_state_ = state_id;
    method_ = Method::kAppendBlobSchema;
    task_flags_.SetBits(TASK_LOW_LATENCY);
    domain_id_ = domain_id;

    // Custom params
    tag_id_ = tag_id;
    data_size_ = data_size;
    page_size_ = page_size;
  }

  /** Destructor */
  ~AppendBlobSchemaTask() {}

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    ar(tag_id_, data_size_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(u32 replica, Ar &ar) {
    ar(append_info_);
  }

  /** Create group */
  HSHM_ALWAYS_INLINE
  u32 GetGroup(hshm::charbuf &group) {
    hshm::NodeThreadId tid;
    labstor::LocalSerialize srl(group);
    srl << tag_id_.unique_;
    srl << tag_id_.node_id_;
    srl << LABSTOR_CLIENT->node_id_;
    return 0;
  }
};

/** A task to append data to a bucket */
struct AppendBlobTask : public Task, TaskFlags<TF_LOCAL> {
  IN TagId tag_id_;
  IN size_t data_size_;
  IN hipc::Pointer data_;
  IN size_t page_size_;
  IN u32 node_id_;
  IN float score_;
  TEMP int phase_ = AppendBlobPhase::kGetBlobIds;
  TEMP AppendBlobSchemaTask *schema_;

  /** SHM default constructor */
  HSHM_ALWAYS_INLINE explicit
  AppendBlobTask(hipc::Allocator *alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
  AppendBlobTask(hipc::Allocator *alloc,
                 const TaskNode &task_node,
                 const DomainId &domain_id,
                 const TaskStateId &state_id,
                 const TagId &tag_id,
                 size_t data_size,
                 const hipc::Pointer &data,
                 size_t page_size,
                 float score,
                 u32 node_id) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = tag_id.unique_;
    task_state_ = state_id;
    method_ = Method::kAppendBlob;
    task_flags_.SetBits(TASK_LOW_LATENCY | TASK_FIRE_AND_FORGET | TASK_DATA_OWNER);
    domain_id_ = domain_id;

    // Custom params
    tag_id_ = tag_id;
    data_size_ = data_size;
    data_ = data;
    score_ = score;
    page_size_ = page_size;
    node_id_ = node_id;
  }

  /** Destructor */
  ~AppendBlobTask() {
    if (IsDataOwner()) {
      LABSTOR_CLIENT->FreeBuffer(data_);
    }
  }

  /** Create group */
  HSHM_ALWAYS_INLINE
  u32 GetGroup(hshm::charbuf &group) {
    hshm::NodeThreadId tid;
    labstor::LocalSerialize srl(group);
    srl << tag_id_.unique_;
    srl << tag_id_.node_id_;
    srl << LABSTOR_CLIENT->node_id_;
    return 0;
  }
};

/** A task to get or create a tag */
struct GetOrCreateTagTask : public Task, TaskFlags<TF_SRL_SYM> {
  IN hipc::ShmArchive<hipc::string> tag_name_;
  IN bool blob_owner_;
  IN hipc::ShmArchive<hipc::vector<TraitId>> traits_;
  IN size_t backend_size_;
  OUT TagId tag_id_;

  /** SHM default constructor */
  HSHM_ALWAYS_INLINE explicit
  GetOrCreateTagTask(hipc::Allocator *alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
  GetOrCreateTagTask(hipc::Allocator *alloc,
                     const TaskNode &task_node,
                     const DomainId &domain_id,
                     const TaskStateId &state_id,
                     const hshm::charbuf &tag_name,
                     bool blob_owner,
                     const std::vector<TraitId> &traits,
                     size_t backend_size) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = std::hash<hshm::charbuf>{}(tag_name);
    task_state_ = state_id;
    method_ = Method::kGetOrCreateTag;
    task_flags_.SetBits(TASK_LOW_LATENCY);
    domain_id_ = domain_id;

    // Custom params
    blob_owner_ = blob_owner;
    backend_size_ = backend_size;
    HSHM_MAKE_AR(tag_name_, LABSTOR_CLIENT->main_alloc_, tag_name)
    HSHM_MAKE_AR(traits_, LABSTOR_CLIENT->main_alloc_, traits)
  }

  /** Destructor */
  ~GetOrCreateTagTask() {
    HSHM_DESTROY_AR(tag_name_)
    HSHM_DESTROY_AR(traits_)
  }

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    task_serialize<Ar>(ar);
    ar(tag_name_, blob_owner_, traits_, backend_size_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(u32 replica, Ar &ar) {
    ar(tag_id_);
  }

  /** Create group */
  HSHM_ALWAYS_INLINE
  u32 GetGroup(hshm::charbuf &group) {
    group.resize(tag_name_->size());
    memcpy(group.data(), tag_name_->data(), tag_name_->size());
    return 0;
  }
};

/** A task to get a tag id */
struct GetTagIdTask : public Task, TaskFlags<TF_SRL_SYM> {
  IN hipc::ShmArchive<hipc::string> tag_name_;
  OUT TagId tag_id_;

  /** SHM default constructor */
  HSHM_ALWAYS_INLINE explicit
  GetTagIdTask(hipc::Allocator *alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
  GetTagIdTask(hipc::Allocator *alloc,
               const TaskNode &task_node,
               const DomainId &domain_id,
               const TaskStateId &state_id,
               const hshm::charbuf &tag_name) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = std::hash<hshm::charbuf>{}(tag_name);
    task_state_ = state_id;
    method_ = Method::kGetTagId;
    task_flags_.SetBits(TASK_LOW_LATENCY);
    domain_id_ = domain_id;

    // Custom params
    HSHM_MAKE_AR(tag_name_, LABSTOR_CLIENT->main_alloc_, tag_name)
  }

  /** Destructor */
  ~GetTagIdTask() {
    HSHM_DESTROY_AR(tag_name_)
  }

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    task_serialize<Ar>(ar);
    ar(tag_name_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(u32 replica, Ar &ar) {
    ar(tag_id_);
  }

  /** Create group */
  HSHM_ALWAYS_INLINE
  u32 GetGroup(hshm::charbuf &group) {
    group.resize(tag_name_->size());
    memcpy(group.data(), tag_name_->data(), tag_name_->size());
    return 0;
  }
};

/** A task to get a tag name */
struct GetTagNameTask : public Task, TaskFlags<TF_SRL_SYM> {
  IN TagId tag_id_;
  OUT hipc::ShmArchive<hipc::string> tag_name_;

  /** SHM default constructor */
  HSHM_ALWAYS_INLINE explicit
  GetTagNameTask(hipc::Allocator *alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
  GetTagNameTask(hipc::Allocator *alloc,
                 const TaskNode &task_node,
                 const DomainId &domain_id,
                 const TaskStateId &state_id,
                 const TagId &tag_id) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = tag_id.unique_;
    task_state_ = state_id;
    method_ = Method::kGetTagName;
    task_flags_.SetBits(TASK_LOW_LATENCY);
    domain_id_ = domain_id;

    // Custom params
    tag_id_ = tag_id;
  }

  /** Destructor */
  ~GetTagNameTask() {
    HSHM_DESTROY_AR(tag_name_)
  }

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    task_serialize<Ar>(ar);
    ar(tag_id_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(u32 replica, Ar &ar) {
    ar(tag_name_);
  }

  /** Create group */
  HSHM_ALWAYS_INLINE
  u32 GetGroup(hshm::charbuf &group) {
    hshm::NodeThreadId tid;
    labstor::LocalSerialize srl(group);
    srl << tag_id_.unique_;
    srl << tag_id_.node_id_;
    srl << LABSTOR_CLIENT->node_id_;
    return 0;
  }
};

/** A task to rename a tag */
struct RenameTagTask : public Task, TaskFlags<TF_SRL_SYM> {
  IN TagId tag_id_;
  IN hipc::ShmArchive<hipc::string> tag_name_;

  /** SHM default constructor */
  HSHM_ALWAYS_INLINE explicit
  RenameTagTask(hipc::Allocator *alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
  RenameTagTask(hipc::Allocator *alloc,
                const TaskNode &task_node,
                const DomainId &domain_id,
                const TaskStateId &state_id,
                const TagId &tag_id,
                const hshm::charbuf &tag_name) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = tag_id.unique_;
    task_state_ = state_id;
    method_ = Method::kRenameTag;
    task_flags_.SetBits(TASK_LOW_LATENCY);
    domain_id_ = domain_id;

    // Custom params
    tag_id_ = tag_id;
    HSHM_MAKE_AR(tag_name_, LABSTOR_CLIENT->main_alloc_, tag_name)
  }

  /** Destructor */
  ~RenameTagTask() {
    HSHM_DESTROY_AR(tag_name_)
  }

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    task_serialize<Ar>(ar);
    ar(tag_id_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(u32 replica, Ar &ar) {
    ar(tag_name_);
  }

  /** Create group */
  HSHM_ALWAYS_INLINE
  u32 GetGroup(hshm::charbuf &group) {
    hshm::NodeThreadId tid;
    labstor::LocalSerialize srl(group);
    srl << tag_id_.unique_;
    srl << tag_id_.node_id_;
    srl << LABSTOR_CLIENT->node_id_;
    return 0;
  }
};

class DestroyTagPhase {
 public:
  TASK_METHOD_T kDestroyBlobs = 0;
  TASK_METHOD_T kWaitDestroyBlobs = 1;
};

/** A task to destroy a tag */
struct DestroyTagTask : public Task, TaskFlags<TF_SRL_SYM> {
  IN TagId tag_id_;
  TEMP int phase_ = DestroyTagPhase::kDestroyBlobs;
  TEMP hipc::ShmArchive<std::vector<blob_mdm::DestroyBlobTask*>> destroy_blob_tasks_;

  /** SHM default constructor */
  HSHM_ALWAYS_INLINE explicit
  DestroyTagTask(hipc::Allocator *alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
  DestroyTagTask(hipc::Allocator *alloc,
                 const TaskNode &task_node,
                 const DomainId &domain_id,
                 const TaskStateId &state_id,
                 TagId tag_id) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = tag_id.unique_;
    task_state_ = state_id;
    method_ = Method::kDestroyTag;
    task_flags_.SetBits(TASK_LOW_LATENCY);
    domain_id_ = domain_id;

    // Custom params
    tag_id_ = tag_id;
  }

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    task_serialize<Ar>(ar);
    ar(tag_id_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(u32 replica, Ar &ar) {}

  /** Create group */
  HSHM_ALWAYS_INLINE
  u32 GetGroup(hshm::charbuf &group) {
    hshm::NodeThreadId tid;
    labstor::LocalSerialize srl(group);
    srl << tag_id_.unique_;
    srl << tag_id_.node_id_;
    srl << LABSTOR_CLIENT->node_id_;
    return 0;
  }
};

/** A task to add a blob to the tag */
struct TagAddBlobTask : public Task, TaskFlags<TF_SRL_SYM> {
  IN TagId tag_id_;
  IN BlobId blob_id_;

  /** SHM default constructor */
  HSHM_ALWAYS_INLINE explicit
  TagAddBlobTask(hipc::Allocator *alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
  TagAddBlobTask(hipc::Allocator *alloc,
                 const TaskNode &task_node,
                 const DomainId &domain_id,
                 const TaskStateId &state_id,
                 TagId tag_id,
                 const BlobId &blob_id) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = tag_id.unique_;
    task_state_ = state_id;
    method_ = Method::kTagAddBlob;
    task_flags_.SetBits(TASK_LOW_LATENCY);
    domain_id_ = domain_id;

    // Custom params
    tag_id_ = tag_id;
    blob_id_ = blob_id;
  }

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    task_serialize<Ar>(ar);
    ar(tag_id_, blob_id_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(u32 replica, Ar &ar) {}

  /** Create group */
  HSHM_ALWAYS_INLINE
  u32 GetGroup(hshm::charbuf &group) {
    hshm::NodeThreadId tid;
    labstor::LocalSerialize srl(group);
    srl << tag_id_.unique_;
    srl << tag_id_.node_id_;
    srl << LABSTOR_CLIENT->node_id_;
    return 0;
  }
};

/** A task to remove a blob from a tag */
struct TagRemoveBlobTask : public Task, TaskFlags<TF_SRL_SYM> {
  IN TagId tag_id_;
  IN BlobId blob_id_;

  /** SHM default constructor */
  HSHM_ALWAYS_INLINE explicit
  TagRemoveBlobTask(hipc::Allocator *alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
  TagRemoveBlobTask(hipc::Allocator *alloc,
                    const TaskNode &task_node,
                    const DomainId &domain_id,
                    const TaskStateId &state_id,
                    TagId tag_id,
                    const BlobId &blob_id) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = tag_id.unique_;
    task_state_ = state_id;
    method_ = Method::kTagRemoveBlob;
    task_flags_.SetBits(TASK_LOW_LATENCY);
    domain_id_ = domain_id;

    // Custom params
    tag_id_ = tag_id;
    blob_id_ = blob_id;
  }

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    task_serialize<Ar>(ar);
    ar(tag_id_, blob_id_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(u32 replica, Ar &ar) {}

  /** Create group */
  HSHM_ALWAYS_INLINE
  u32 GetGroup(hshm::charbuf &group) {
    hshm::NodeThreadId tid;
    labstor::LocalSerialize srl(group);
    srl << tag_id_.unique_;
    srl << tag_id_.node_id_;
    srl << LABSTOR_CLIENT->node_id_;
    return 0;
  }
};

/** A task to get the group of blobs associated with a tag */
struct TagGroupByTask : public Task, TaskFlags<TF_SRL_SYM> {};

/** A task to associate a tag with a trait */
struct TagAddTraitTask : public Task, TaskFlags<TF_SRL_SYM> {};

/** A task to remove a trait from a tag */
struct TagRemoveTraitTask : public Task, TaskFlags<TF_SRL_SYM> {};

/** A task to destroy all blobs in the tag */
struct TagClearBlobsTask : public Task, TaskFlags<TF_SRL_SYM> {
  IN TagId tag_id_;

  /** SHM default constructor */
  HSHM_ALWAYS_INLINE explicit
  TagClearBlobsTask(hipc::Allocator *alloc) : Task(alloc) {}

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
  TagClearBlobsTask(hipc::Allocator *alloc,
                    const TaskNode &task_node,
                    const DomainId &domain_id,
                    const TaskStateId &state_id,
                    TagId tag_id) : Task(alloc) {
    // Initialize task
    task_node_ = task_node;
    lane_hash_ = tag_id.unique_;
    task_state_ = state_id;
    method_ = Method::kTagClearBlobs;
    task_flags_.SetBits(TASK_LOW_LATENCY);
    domain_id_ = domain_id;

    // Custom params
    tag_id_ = tag_id;
  }

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    task_serialize<Ar>(ar);
    ar(tag_id_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(u32 replica, Ar &ar) {}

  /** Create group */
  HSHM_ALWAYS_INLINE
  u32 GetGroup(hshm::charbuf &group) {
    hshm::NodeThreadId tid;
    labstor::LocalSerialize srl(group);
    srl << tag_id_.unique_;
    srl << tag_id_.node_id_;
    srl << LABSTOR_CLIENT->node_id_;
    return 0;
  }
};

}  // namespace hermes::bucket_mdm

#endif  // LABSTOR_TASKS_HERMES_BUCKET_MDM_INCLUDE_HERMES_BUCKET_MDM_HERMES_BUCKET_MDM_TASKS_H_
