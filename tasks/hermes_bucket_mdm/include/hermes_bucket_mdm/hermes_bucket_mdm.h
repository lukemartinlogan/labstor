//
// Created by lukemartinlogan on 6/29/23.
//

#ifndef LABSTOR_hermes_bucket_mdm_H_
#define LABSTOR_hermes_bucket_mdm_H_

#include "hermes_bucket_mdm_tasks.h"

namespace hermes::bucket_mdm {

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

  /** Put blob (fire & forget) */
  HSHM_ALWAYS_INLINE
  void PutBlob(const TaskNode &task_node,
               TagId tag_id, const BlobId &blob_id,
               size_t blob_off, size_t blob_size,
               bitfield32_t flags) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    u32 hash = tag_id.node_id_;
    LABSTOR_CLIENT->NewTask<PutBlobTask>(
        p, task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        tag_id, blob_id,
        blob_off, blob_size, flags);
    queue->Emplace(hash, p);
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
    HILOG(kInfo, "Creating a tag {}", tag_name.str());
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
