//
// Created by lukemartinlogan on 8/14/23.
//

#ifndef LABSTOR_TASKS_HERMES_MDM_INCLUDE_HERMES_MDM_HERMES_MDM_TASKS_H_
#define LABSTOR_TASKS_HERMES_MDM_INCLUDE_HERMES_MDM_HERMES_MDM_TASKS_H_

#include "labstor/api/labstor_client.h"
#include "labstor/task_registry/task_lib.h"
#include "labstor_admin/labstor_admin.h"
#include "labstor/queue_manager/queue_manager_client.h"
#include "hermes/hermes_types.h"
#include "bdev/bdev.h"

namespace hermes::mdm {

using labstor::Task;
using labstor::SrlFlags;
using labstor::DataTransfer;

/** The set of methods in the admin task */
struct Method : public TaskMethod {
  TASK_METHOD_T kGetOrCreateTag = TaskMethod::kLast + 0;
  TASK_METHOD_T kGetTagId = TaskMethod::kLast + 1;
  TASK_METHOD_T kGetTagName = TaskMethod::kLast + 2;
  TASK_METHOD_T kRenameTag = TaskMethod::kLast + 3;
  TASK_METHOD_T kDestroyTag = TaskMethod::kLast + 4;
  TASK_METHOD_T kTagAddBlob = TaskMethod::kLast + 5;
  TASK_METHOD_T kTagRemoveBlob = TaskMethod::kLast + 6;
  TASK_METHOD_T kTagGroupBy = TaskMethod::kLast + 7;
  TASK_METHOD_T kTagAddTrait = TaskMethod::kLast + 8;
  TASK_METHOD_T kTagRemoveTrait = TaskMethod::kLast + 9;
  TASK_METHOD_T kTagClearBlobs = TaskMethod::kLast + 10;
  TASK_METHOD_T kPutBlob = TaskMethod::kLast + 11;
  TASK_METHOD_T kGetBlob = TaskMethod::kLast + 12;
  TASK_METHOD_T kTruncateBlob = TaskMethod::kLast + 13;
  TASK_METHOD_T kDestroyBlob = TaskMethod::kLast + 14;
  TASK_METHOD_T kTagBlob = TaskMethod::kLast + 15;
  TASK_METHOD_T kUntagBlob = TaskMethod::kLast + 16;
  TASK_METHOD_T kBlobHasTag = TaskMethod::kLast + 17;
  TASK_METHOD_T kGetBlobTags = TaskMethod::kLast + 18;
  TASK_METHOD_T kGetBlobId = TaskMethod::kLast + 19;
  TASK_METHOD_T kGetBlobName = TaskMethod::kLast + 20;
  TASK_METHOD_T kGetBlobSize = TaskMethod::kLast + 21;
  TASK_METHOD_T kGetBlobScore = TaskMethod::kLast + 22;
  TASK_METHOD_T kGetBlobBuffers = TaskMethod::kLast + 23;
  TASK_METHOD_T kRenameBlob = TaskMethod::kLast + 24;
};

/** Phases of the construct task */
using labstor::Admin::CreateTaskStatePhase;
class ConstructTaskPhase : public CreateTaskStatePhase {
 public:
  TASK_METHOD_T kLoadConfig = kLast;
  TASK_METHOD_T kCreateTaskStates = kLast + 1;
  TASK_METHOD_T kWaitForTaskStates = kLast + 2;
  TASK_METHOD_T kCreateQueues = kLast + 3;
  TASK_METHOD_T kWaitForQueues = kLast + 4;
};

/**
 * A task to create hermes_mdm
 * */
using labstor::Admin::CreateTaskStateTask;
struct ConstructTask : public CreateTaskStateTask {
  IN hipc::ShmArchive<hipc::string> server_config_path_;

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
  ConstructTask(hipc::Allocator *alloc,
                const TaskNode &task_node,
                const DomainId &domain_id,
                const std::string &state_name,
                const TaskStateId &id,
                u32 max_lanes, u32 num_lanes,
                u32 depth, bitfield32_t flags,
                const std::string &server_config_path = "")
      : CreateTaskStateTask(alloc, task_node, domain_id, state_name,
                            "hermes_mdm", id, max_lanes,
                            num_lanes, depth, flags) {
    // Custom params
    HSHM_MAKE_AR(server_config_path_, alloc, server_config_path);
  }

  /** Destructor */
  HSHM_ALWAYS_INLINE
  ~ConstructTask() {
    HSHM_DESTROY_AR(server_config_path_);
  }

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    CreateTaskStateTask::SerializeStart(ar);
    ar(server_config_path_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(Ar &ar) {}
};

/** A task to destroy hermes_mdm */
using labstor::Admin::DestroyTaskStateTask;
struct DestructTask : public DestroyTaskStateTask {
  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
  DestructTask(hipc::Allocator *alloc,
               const TaskNode &task_node,
               const DomainId &domain_id,
               const TaskStateId &state_id)
      : DestroyTaskStateTask(alloc, task_node, domain_id, state_id) {}
};

/**====================================
 * Tag Operations
 * ===================================*/

/** A task to get or create a tag */
struct GetOrCreateTagTask : public Task, SrlFlags<true, true> {
  IN hipc::ShmArchive<hipc::string> tag_name_;
  IN bool blob_owner_;
  IN hipc::ShmArchive<hipc::vector<TraitId>> traits_;
  IN size_t backend_size_;
  OUT TagId tag_id_;

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
  void SerializeEnd(Ar &ar) {}
};

/** A task to get a tag id */
struct GetTagIdTask : public Task, SrlFlags<true, true> {
  IN hipc::ShmArchive<hipc::string> tag_name_;
  OUT TagId tag_id_;

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
    ar(tag_id_, tag_name_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(Ar &ar) {
  }
};

/** A task to destroy a tag */
struct DestroyTagTask : public Task, SrlFlags<true, true> {
  IN TagId tag_id_;

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
  void SerializeEnd(Ar &ar) {
  }
};

/** A task to add a blob to the tag */
struct TagAddBlobTask : public Task, SrlFlags<true, true> {
  IN TagId tag_id_;
  IN BlobId blob_id_;

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
  void SerializeEnd(Ar &ar) {
  }
};

/** A task to remove a blob from a tag */
struct TagRemoveBlobTask : public Task, SrlFlags<true, true> {
  IN TagId tag_id_;
  IN BlobId blob_id_;

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
  void SerializeEnd(Ar &ar) {
  }
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
  void SerializeEnd(Ar &ar) {
  }
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

/** A task to put data in a blob */
struct PutBlobTask : public Task, SrlFlags<true, true> {
  IN TagId tag_id_;
  IN hipc::ShmArchive<hipc::charbuf> blob_name_;
  IN size_t blob_off_;
  IN size_t data_size_;
  IN hipc::Pointer data_;
  IN float score_;
  IN bool replace_;
  INOUT BlobId blob_id_;
  OUT bool did_create_;
  TEMP int phase_;
  TEMP int plcmnt_idx_;
  TEMP int sub_plcmnt_idx_;
  TEMP hermes::bdev::AllocTask *cur_bdev_alloc_;
  TEMP hipc::ShmArchive<std::vector<PlacementSchema>> schema_;
  TEMP hipc::ShmArchive<std::vector<hermes::bdev::WriteTask *>> bdev_writes_;

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
              bool replace) : Task(alloc) {
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
    replace_ = replace;
    phase_ = PutBlobPhase::kCreate;
    plcmnt_idx_ = 0;
  }

  /** Destructor */
  ~PutBlobTask() {
    HSHM_DESTROY_AR(blob_name_);
    LABSTOR_CLIENT->FreeBuffer(data_);
  }

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    task_serialize<Ar>(ar);
    ar(tag_id_, blob_name_, blob_id_, blob_off_, data_size_, data_, score_, replace_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(Ar &ar) {
    ar(blob_id_, did_create_);
  }
};

/** Phases for the get task */
class GetBlobPhase {
 public:
  TASK_METHOD_T kStart = 0;
  TASK_METHOD_T kWait = 1;
};

/** A task to get data from a blob */
struct GetBlobTask : public Task, SrlFlags<true, true> {
  IN BlobId blob_id_;
  IN size_t blob_off_;
  INOUT ssize_t data_size_;
  OUT hipc::Pointer data_;
  TEMP int phase_;
  TEMP hipc::ShmArchive<std::vector<bdev::ReadTask *>> bdev_reads_;

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
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

  /** (De)serialize message call */
  template<typename Ar>
  void SerializeStart(Ar &ar) {
    // TODO(llogan): make it so that blob gets take as input a buffer
    task_serialize<Ar>(ar);
    ar(blob_id_, blob_off_, data_size_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(Ar &ar) {
    ar(data_);
  }
};

/** A task to tag a blob */
struct TagBlobTask : public Task, SrlFlags<true, true> {
  IN BlobId blob_id_;
  IN TagId tag_;

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
  void SerializeEnd(Ar &ar) {
  }
};

/**
 * Check if blob has a tag
 * */
struct BlobHasTagTask : public Task, SrlFlags<true, true> {
  IN BlobId blob_id_;
  IN TagId tag_;
  OUT bool has_tag_;

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
  void SerializeEnd(Ar &ar) {
    ar(has_tag_);
  }
};

/**
 * Get \a blob_name BLOB from \a bkt_id bucket
 * */
struct GetBlobIdTask : public Task, SrlFlags<true, true> {
  IN TagId tag_id_;
  IN hipc::ShmArchive<hipc::charbuf> blob_name_;
  OUT BlobId blob_id_;

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
    ar(tag_id_, blob_name_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(Ar &ar) {
    ar(blob_id_);
  }
};

/**
 * Get \a blob_name BLOB name from \a blob_id BLOB id
 * */
struct GetBlobNameTask : public Task, SrlFlags<true, true> {
  IN BlobId blob_id_;
  OUT hipc::ShmArchive<hipc::string> blob_name_;

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
  void SerializeEnd(Ar &ar) {
    ar(blob_name_);
  }
};

/** Get \a score from \a blob_id BLOB id */
struct GetBlobScoreTask : public Task, SrlFlags<true, true> {
  IN BlobId blob_id_;
  OUT float score_;

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
  void SerializeEnd(Ar &ar) {
    ar(score_);
  }
};

/** Get \a blob_id blob's buffers */
struct GetBlobBuffersTask : public Task, SrlFlags<true, true> {
  IN BlobId blob_id_;
  OUT hipc::ShmArchive<hipc::vector<BufferInfo>> buffers_;

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
  void SerializeEnd(Ar &ar) {
    ar(buffers_);
  }
};

/**
 * Rename \a blob_id blob to \a new_blob_name new blob name
 * in \a bkt_id bucket.
 * */
struct RenameBlobTask : public Task, SrlFlags<true, true> {
  IN BlobId blob_id_;
  IN hipc::ShmArchive<hipc::charbuf> new_blob_name_;

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
  void SerializeEnd(Ar &ar) {
    ar(new_blob_name_);
  }
};

/** A task to truncate a blob */
struct TruncateBlobTask : public Task, SrlFlags<true, true> {
  BlobId blob_id_;
  IN u64 size_;

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
    ar(blob_id_);
  }

  /** (De)serialize message return */
  template<typename Ar>
  void SerializeEnd(Ar &ar) {
    ar(size_);
  }
};

/** Phases of the destroy blob task */
struct DestroyBlobPhase {
  TASK_METHOD_T kFreeBuffers = 0;
  TASK_METHOD_T kWaitFreeBuffers = 1;
};

/** A task to destroy a blob */
struct DestroyBlobTask : public Task, SrlFlags<true, true> {
  IN BlobId blob_id_;
  TEMP int phase_;
  TEMP hipc::ShmArchive<std::vector<bdev::FreeTask *>> free_tasks_;
  TEMP BlobInfo *blob_info_;

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
  void SerializeEnd(Ar &ar) {
  }
};

}  // namespace hermes::mdm

#endif  // LABSTOR_TASKS_HERMES_MDM_INCLUDE_HERMES_MDM_HERMES_MDM_TASKS_H_
