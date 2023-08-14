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

namespace hermes::bucket_mdm {

using labstor::Task;
using labstor::SrlFlags;
using labstor::DataTransfer;

#include "hermes_bucket_mdm_methods.h"

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
};

/** Phases for the put task */
class PutBlobPhase {
 public:
  TASK_METHOD_T kUpdateMdm = 0;
  TASK_METHOD_T kWait = 1;
};

/** Put a blob in the bucket */
struct PutBlobTask : public Task, SrlFlags<false, true> {
  IN TagId tag_id_;
  IN hipc::ShmArchive<hipc::charbuf> blob_name_;
  IN size_t blob_off_;
  IN size_t data_size_;
  IN hipc::Pointer data_;
  IN float score_;
  IN bitfield32_t flags_;
  INOUT BlobId blob_id_;
  TEMP int phase_;
  TEMP Task *blob_put_task_;

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
    phase_ = PutBlobPhase::kUpdateMdm;
  }

  /** Destructor */
  ~PutBlobTask() {
    HSHM_DESTROY_AR(blob_name_);
    // LABSTOR_CLIENT->FreeBuffer(data_);
  }

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    task_serialize<Ar>(ar);
    // TODO(llogan): make it so data xfer doesn't happen here
    ar(tag_id_, blob_name_, blob_id_, blob_off_, data_size_, score_, flags_, phase_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(Ar &ar) {
    ar(blob_id_);
  }
};

/** A task to get or create a tag */
struct GetOrCreateTagTask : public Task, SrlFlags<true, true> {
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
  void SerializeEnd(Ar &ar) {
    ar(tag_id_);
  }
};

/** A task to get a tag id */
struct GetTagIdTask : public Task, SrlFlags<true, true> {
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
  void SerializeEnd(Ar &ar) {
    ar(tag_id_);
  }
};

/** A task to get a tag name */
struct GetTagNameTask : public Task, SrlFlags<true, true> {
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
  void SerializeEnd(Ar &ar) {
    ar(tag_name_);
  }
};

/** A task to rename a tag */
struct RenameTagTask : public Task, SrlFlags<true, true> {
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
  void SerializeEnd(Ar &ar) {
    ar(tag_name_);
  }
};

/** A task to destroy a tag */
struct DestroyTagTask : public Task, SrlFlags<true, true> {
  IN TagId tag_id_;

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
  void SerializeEnd(Ar &ar) {}
};

/** A task to add a blob to the tag */
struct TagAddBlobTask : public Task, SrlFlags<true, true> {
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
  void SerializeEnd(Ar &ar) {}
};

/** A task to remove a blob from a tag */
struct TagRemoveBlobTask : public Task, SrlFlags<true, true> {
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
  void SerializeEnd(Ar &ar) {}
};

/** A task to get the group of blobs associated with a tag */
struct TagGroupByTask : public Task, SrlFlags<true, true> {};

/** A task to associate a tag with a trait */
struct TagAddTraitTask : public Task, SrlFlags<true, true> {};

/** A task to remove a trait from a tag */
struct TagRemoveTraitTask : public Task, SrlFlags<true, true> {};

/** A task to destroy all blobs in the tag */
struct TagClearBlobsTask : public Task, SrlFlags<true, true> {
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
  void SerializeEnd(Ar &ar) {}
};

}  // namespace hermes::bucket_mdm

#endif  // LABSTOR_TASKS_HERMES_BUCKET_MDM_INCLUDE_HERMES_BUCKET_MDM_HERMES_BUCKET_MDM_TASKS_H_
