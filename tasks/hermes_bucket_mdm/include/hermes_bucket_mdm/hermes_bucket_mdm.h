//
// Created by lukemartinlogan on 6/29/23.
//

#ifndef LABSTOR_hermes_bucket_mdm_H_
#define LABSTOR_hermes_bucket_mdm_H_

#include "labstor/api/labstor_client.h"
#include "labstor/task_registry/task_lib.h"
#include "labstor_admin/labstor_admin.h"
#include "labstor/queue_manager/queue_manager_client.h"
#include "hermes/hermes_types.h"
#include "bdev/bdev.h"
#include "hermes_blob_mdm/hermes_blob_mdm.h"

namespace hermes::bucket_mdm {

/** The set of methods in the hermes_bucket_mdm task */
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
};

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
  HSHM_ALWAYS_INLINE
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

  HSHM_ALWAYS_INLINE
  ~ConstructTask() {
    // Custom params
  }
};

/** A task to destroy hermes_bucket_mdm */
using labstor::Admin::DestroyTaskStateTask;
struct DestructTask : public DestroyTaskStateTask {
  HSHM_ALWAYS_INLINE
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
struct PutBlobTask : public Task {
  IN TagId tag_id_;
  IN hipc::ShmArchive<hipc::charbuf> blob_name_;
  IN size_t blob_off_;
  IN size_t data_size_;
  IN hipc::Pointer data_;
  IN float score_;
  IN bitfield32_t flags_;
  INOUT BlobId blob_id_;
  TEMP int phase_;
  TEMP Task* blob_put_task_;

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
    phase_ = PutBlobPhase::kUpdateMdm;
  }

  ~PutBlobTask() {
    HSHM_DESTROY_AR(blob_name_);
    // LABSTOR_CLIENT->FreeBuffer(data_);
  }
};

/** A task to get or create a tag */
struct GetOrCreateTagTask : public Task {
  IN hipc::ShmArchive<hipc::string> tag_name_;
  IN bool blob_owner_;
  IN hipc::ShmArchive<hipc::vector<TraitId>> traits_;
  IN size_t backend_size_;
  OUT TagId tag_id_;

  HSHM_ALWAYS_INLINE
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

  ~GetOrCreateTagTask() {
    HSHM_DESTROY_AR(tag_name_)
    HSHM_DESTROY_AR(traits_)
  }
};

/** A task to get a tag id */
struct GetTagIdTask : public Task {
  IN hipc::ShmArchive<hipc::string> tag_name_;
  OUT TagId tag_id_;

  HSHM_ALWAYS_INLINE
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

  ~GetTagIdTask() {
    HSHM_DESTROY_AR(tag_name_)
  }
};

/** A task to get a tag name */
struct GetTagNameTask : public Task {
  IN TagId tag_id_;
  OUT hipc::ShmArchive<hipc::string> tag_name_;

  HSHM_ALWAYS_INLINE
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

  ~GetTagNameTask() {
    HSHM_DESTROY_AR(tag_name_)
  }
};

/** A task to rename a tag */
struct RenameTagTask : public Task {
  IN TagId tag_id_;
  IN hipc::ShmArchive<hipc::string> tag_name_;

  HSHM_ALWAYS_INLINE
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

  ~RenameTagTask() {
    HSHM_DESTROY_AR(tag_name_)
  }
};

/** A task to destroy a tag */
struct DestroyTagTask : public Task {
  IN TagId tag_id_;

  HSHM_ALWAYS_INLINE
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
};

/** A task to add a blob to the tag */
struct TagAddBlobTask : public Task {
  IN TagId tag_id_;
  BlobId blob_id_;

  HSHM_ALWAYS_INLINE
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
};

/** A task to remove a blob from a tag */
struct TagRemoveBlobTask : public Task {
  IN TagId tag_id_;
  BlobId blob_id_;

  HSHM_ALWAYS_INLINE
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
};

/** A task to get the group of blobs associated with a tag */
struct TagGroupByTask : public Task {};

/** A task to associate a tag with a trait */
struct TagAddTraitTask : public Task {};

/** A task to remove a trait from a tag */
struct TagRemoveTraitTask : public Task {};

/** A task to destroy all blobs in the tag */
struct TagClearBlobsTask : public Task {
  IN TagId tag_id_;

  HSHM_ALWAYS_INLINE
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
};

/** Create hermes_bucket_mdm requests */
class Client {
 public:
  TaskStateId id_;
  QueueId queue_id_;

 public:
  /** Default constructor */
  Client() = default;

  /** Destructor */
  ~Client() = default;

  /** Create a hermes_bucket_mdm */
  HSHM_ALWAYS_INLINE
  void Create(const TaskNode &task_node,
              const DomainId &domain_id,
              const std::string &state_name) {
    id_ = TaskStateId::GetNull();
    id_ = LABSTOR_ADMIN->CreateTaskState<ConstructTask>(
        task_node, domain_id, state_name, id_,
        LABSTOR_CLIENT->server_config_.queue_manager_.max_lanes_,
        LABSTOR_CLIENT->server_config_.queue_manager_.max_lanes_,
        LABSTOR_CLIENT->server_config_.queue_manager_.queue_depth_,
        bitfield32_t(0));
    queue_id_ = QueueId(id_);
  }
  LABSTOR_TASK_NODE_ROOT(Create);

  /** Destroy task state + queue */
  HSHM_ALWAYS_INLINE
  void Destroy(const TaskNode &task_node,
               const DomainId &domain_id) {
    LABSTOR_ADMIN->DestroyTaskState(task_node, domain_id, id_);
  }
  LABSTOR_TASK_NODE_ROOT(Destroy);

  /**====================================
   * Tag Operations
   * ===================================*/

  /** Put blob */
  HSHM_ALWAYS_INLINE
  void PutBlob(const TaskNode &task_node,
               TagId tag_id, const hshm::charbuf &blob_name,
               BlobId &blob_id, size_t blob_off, size_t blob_size,
               const hipc::Pointer &blob, float score,
               bitfield32_t flags) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    u32 hash = tag_id.node_id_;
    auto *task = LABSTOR_CLIENT->NewTask<PutBlobTask>(
        p, task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        tag_id, blob_name, blob_id,
        blob_off, blob_size,
        blob, score, flags);
    queue->Emplace(hash, p);
    task->Wait();
    // LABSTOR_CLIENT->DelTask(task);
  }
  LABSTOR_TASK_NODE_ROOT(PutBlob);

  /** Create a tag or get the ID of existing tag */
  HSHM_ALWAYS_INLINE
  void GetOrCreateTag(const TaskNode &task_node,
                      const hshm::charbuf &tag_name,
                      bool blob_owner,
                      const std::vector<TraitId> &traits,
                      size_t backend_size) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    u32 hash = std::hash<hshm::charbuf>{}(tag_name);
    auto *task = LABSTOR_CLIENT->NewTask<GetOrCreateTagTask>(
        p, task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        tag_name, blob_owner, traits, backend_size);
    queue->Emplace(hash, p);
    task->Wait();
    LABSTOR_CLIENT->DelTask(task);
  }
  LABSTOR_TASK_NODE_ROOT(GetOrCreateTag);

  /** Get tag ID */
  TagId GetTagId(const TaskNode &task_node,
                 const hshm::charbuf &tag_name) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    u32 hash = std::hash<hshm::charbuf>{}(tag_name);
    auto *task = LABSTOR_CLIENT->NewTask<GetTagIdTask>(
        p,
        task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        tag_name);
    queue->Emplace(hash, p);
    task->Wait();
    TagId tag_id = task->tag_id_;
    LABSTOR_CLIENT->DelTask(task);
    return tag_id;
  }
  LABSTOR_TASK_NODE_ROOT(GetTagId);

  /** Get tag name */
  hshm::string GetTagName(const TaskNode &task_node,
                          const TagId &tag_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    u32 hash = tag_id.unique_;
    auto *task = LABSTOR_CLIENT->NewTask<GetTagNameTask>(
        p,
        task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        tag_id);
    queue->Emplace(hash, p);
    task->Wait();
    hshm::string tag_name = hshm::to_charbuf<hipc::string>(*task->tag_name_.get());
    LABSTOR_CLIENT->DelTask(task);
    return tag_name;
  }
  LABSTOR_TASK_NODE_ROOT(GetTagName);

  /** Rename tag */
  void RenameTag(const TaskNode &task_node,
                 const TagId &tag_id, const hshm::charbuf &new_tag_name) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    u32 hash = tag_id.unique_;
    auto *task = LABSTOR_CLIENT->NewTask<RenameTagTask>(
        p,
        task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        tag_id, new_tag_name);
    queue->Emplace(hash, p);
    task->Wait();
    LABSTOR_CLIENT->DelTask(task);
  }
  LABSTOR_TASK_NODE_ROOT(RenameTag);

  /** Destroy tag */
  void DestroyTag(const TaskNode &task_node,
                  const TagId &tag_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    u32 hash = tag_id.unique_;
    auto *task = LABSTOR_CLIENT->NewTask<DestroyTagTask>(
        p,
        task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        tag_id);
    queue->Emplace(hash, p);
    task->Wait();
    LABSTOR_CLIENT->DelTask(task);
  }
  LABSTOR_TASK_NODE_ROOT(DestroyTag);

  /** Add a blob to a tag */
  void TagAddBlob(const TaskNode &task_node,
                  const TagId &tag_id, const BlobId &blob_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    u32 hash = tag_id.unique_;
    auto *task = LABSTOR_CLIENT->NewTask<TagAddBlobTask>(
        p,
        task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        tag_id, blob_id);
    queue->Emplace(hash, p);
    task->Wait();
    LABSTOR_CLIENT->DelTask(task);
  }
  LABSTOR_TASK_NODE_ROOT(TagAddBlob);

  /** Remove a blob from a tag */
  void TagRemoveBlob(const TaskNode &task_node,
                     const TagId &tag_id, const BlobId &blob_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    u32 hash = tag_id.unique_;
    auto *task = LABSTOR_CLIENT->NewTask<TagRemoveBlobTask>(
        p,
        task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        tag_id, blob_id);
    queue->Emplace(hash, p);
    task->Wait();
    LABSTOR_CLIENT->DelTask(task);
  }
  LABSTOR_TASK_NODE_ROOT(TagRemoveBlob);

  /** Clear blobs from a tag */
  void TagClearBlobs(const TaskNode &task_node,
                     const TagId &tag_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    u32 hash = tag_id.unique_;
    auto *task = LABSTOR_CLIENT->NewTask<TagClearBlobsTask>(
        p,
        task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        tag_id);
    queue->Emplace(hash, p);
    task->Wait();
    LABSTOR_CLIENT->DelTask(task);
  }
  LABSTOR_TASK_NODE_ROOT(TagClearBlobs);
};

}  // namespace labstor

#endif  // LABSTOR_hermes_bucket_mdm_H_
