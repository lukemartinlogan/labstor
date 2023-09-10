//
// Created by lukemartinlogan on 6/29/23.
//

#ifndef LABSTOR_hermes_bucket_mdm_H_
#define LABSTOR_hermes_bucket_mdm_H_

#include "hermes_bucket_mdm_tasks.h"

namespace hermes::bucket_mdm {

/** Create hermes_bucket_mdm requests */
class Client : public TaskLibClient {
 public:
  /** Default constructor */
  Client() = default;

  /** Destructor */
  ~Client() = default;

  /** Init from existing ID */
  void Init(const TaskStateId &id) {
    id_ = id;
    queue_id_ = QueueId(id_);
  }

  /** Create a hermes_bucket_mdm */
  HSHM_ALWAYS_INLINE
  void CreateRoot(const DomainId &domain_id,
                  const std::string &state_name) {
    id_ = TaskStateId::GetNull();
    id_ = LABSTOR_ADMIN->CreateTaskStateRoot<ConstructTask>(
        domain_id, state_name, id_,
        LABSTOR_CLIENT->server_config_.queue_manager_.max_lanes_,
        LABSTOR_CLIENT->server_config_.queue_manager_.max_lanes_,
        LABSTOR_CLIENT->server_config_.queue_manager_.queue_depth_,
        bitfield32_t(0));
    queue_id_ = QueueId(id_);
  }

  /** Destroy task state + queue */
  HSHM_ALWAYS_INLINE
  void DestroyRoot(const DomainId &domain_id) {
    LABSTOR_ADMIN->DestroyTaskStateRoot(domain_id, id_);
  }

  /**====================================
   * Tag Operations
   * ===================================*/

  /** Update statistics after blob PUT (fire & forget) */
  HSHM_ALWAYS_INLINE
  void AsyncPutBlob(const TaskNode &task_node,
                    TagId tag_id, const BlobId &blob_id,
                    size_t blob_off, size_t blob_size,
                    bitfield32_t flags) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);
    u32 hash = tag_id.unique_;
    LABSTOR_CLIENT->NewTask<PutBlobTask>(
        p, task_node, DomainId::GetNode(tag_id.node_id_), id_,
        tag_id, blob_id,
        blob_off, blob_size, flags);
    queue->Emplace(hash, p);
  }
  LABSTOR_TASK_NODE_ROOT(AsyncPutBlob);

  /** Append data to the bucket (fire & forget) */
  HSHM_ALWAYS_INLINE
  AppendBlobSchemaTask* AsyncAppendBlobSchema(const TaskNode &task_node,
                                              TagId tag_id,
                                              size_t data_size,
                                              size_t page_size) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);
    u32 hash = tag_id.unique_;
    auto *task = LABSTOR_CLIENT->NewTask<AppendBlobSchemaTask>(
        p, task_node, DomainId::GetNode(tag_id.node_id_), id_,
        tag_id, data_size, page_size);
    queue->Emplace(hash, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncAppendBlobSchema);

  /** Append data to the bucket (fire & forget) */
  HSHM_ALWAYS_INLINE
  void AppendBlob(const TaskNode &task_node,
                  TagId tag_id,
                  size_t data_size,
                  const hipc::Pointer &data,
                  size_t page_size,
                  float score,
                  u32 node_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);
    u32 hash = tag_id.unique_;
    LABSTOR_CLIENT->NewTask<AppendBlobTask>(
        p, task_node, DomainId::GetNode(tag_id.node_id_), id_,
        tag_id, data_size, data, page_size, score, node_id);
    queue->Emplace(hash, p);
  }
  LABSTOR_TASK_NODE_ROOT(AppendBlob);

  /** Create a tag or get the ID of existing tag */
  HSHM_ALWAYS_INLINE
  GetOrCreateTagTask* AsyncGetOrCreateTag(const TaskNode &task_node,
                                          const hshm::charbuf &tag_name,
                                          bool blob_owner,
                                          const std::vector<TraitId> &traits,
                                          size_t backend_size) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);
    HILOG(kDebug, "Creating a tag {}", tag_name.str());
    u32 hash = std::hash<hshm::charbuf>{}(tag_name);
    auto *task = LABSTOR_CLIENT->NewTask<GetOrCreateTagTask>(
        p, task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        tag_name, blob_owner, traits, backend_size);
    queue->Emplace(hash, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncGetOrCreateTag);
  HSHM_ALWAYS_INLINE
  TagId GetOrCreateTagRoot(const hshm::charbuf &tag_name,
                           bool blob_owner,
                           const std::vector<TraitId> &traits,
                           size_t backend_size) {
    GetOrCreateTagTask *task = AsyncGetOrCreateTagRoot(tag_name,
                                                       blob_owner, traits,
                                                       backend_size);
    task->Wait();
    TagId tag_id = task->tag_id_;
    LABSTOR_CLIENT->DelTask(task);
    return tag_id;
  }

  /** Get tag ID */
  GetTagIdTask* AsyncGetTagId(const TaskNode &task_node,
                              const hshm::charbuf &tag_name) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);
    u32 hash = std::hash<hshm::charbuf>{}(tag_name);
    auto *task = LABSTOR_CLIENT->NewTask<GetTagIdTask>(
        p,
        task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        tag_name);
    queue->Emplace(hash, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncGetTagId);
  TagId GetTagIdRoot(const hshm::charbuf &tag_name) {
    GetTagIdTask *task = AsyncGetTagIdRoot(tag_name);
    task->Wait();
    TagId tag_id = task->tag_id_;
    LABSTOR_CLIENT->DelTask(task);
    return tag_id;
  }

  /** Get tag name */
  GetTagNameTask* AsyncGetTagName(const TaskNode &task_node,
                                  const TagId &tag_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);
    u32 hash = tag_id.unique_;
    auto *task = LABSTOR_CLIENT->NewTask<GetTagNameTask>(
        p,
        task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        tag_id);
    queue->Emplace(hash, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncGetTagName);
  hshm::string GetTagNameRoot(const TagId &tag_id) {
    GetTagNameTask *task = AsyncGetTagNameRoot(tag_id);
    task->Wait();
    hshm::string tag_name = hshm::to_charbuf<hipc::string>(*task->tag_name_.get());
    LABSTOR_CLIENT->DelTask(task);
    return tag_name;
  }

  /** Rename tag */
  RenameTagTask* AsyncRenameTag(const TaskNode &task_node,
                                const TagId &tag_id,
                                const hshm::charbuf &new_tag_name) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);
    u32 hash = tag_id.unique_;
    auto *task = LABSTOR_CLIENT->NewTask<RenameTagTask>(
        p,
        task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        tag_id, new_tag_name);
    queue->Emplace(hash, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncRenameTag);
  void RenameTagRoot(const TagId &tag_id, const hshm::charbuf &new_tag_name) {
    RenameTagTask *task = AsyncRenameTagRoot(tag_id, new_tag_name);
    task->Wait();
    LABSTOR_CLIENT->DelTask(task);
  }

  /** Destroy tag */
  DestroyTagTask* AsyncDestroyTag(const TaskNode &task_node,
                                  const TagId &tag_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);
    u32 hash = tag_id.unique_;
    auto *task = LABSTOR_CLIENT->NewTask<DestroyTagTask>(
        p,
        task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        tag_id);
    queue->Emplace(hash, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncDestroyTag);
  void DestroyTagRoot(const TagId &tag_id) {
    DestroyTagTask *task = AsyncDestroyTagRoot(tag_id);
    task->Wait();
    LABSTOR_CLIENT->DelTask(task);
  }

  /** Add a blob to a tag */
  TagAddBlobTask* AsyncTagAddBlob(const TaskNode &task_node,
                                  const TagId &tag_id,
                                  const BlobId &blob_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);
    u32 hash = tag_id.unique_;
    auto *task = LABSTOR_CLIENT->NewTask<TagAddBlobTask>(
        p,
        task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        tag_id, blob_id);
    queue->Emplace(hash, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncTagAddBlob);
  void TagAddBlobRoot(const TagId &tag_id, const BlobId &blob_id) {
    TagAddBlobTask *task = AsyncTagAddBlobRoot(tag_id, blob_id);
    task->Wait();
    LABSTOR_CLIENT->DelTask(task);
  }

  /** Remove a blob from a tag */
  TagRemoveBlobTask* AsyncTagRemoveBlob(const TaskNode &task_node,
                                        const TagId &tag_id, const BlobId &blob_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);
    u32 hash = tag_id.unique_;
    auto *task = LABSTOR_CLIENT->NewTask<TagRemoveBlobTask>(
        p,
        task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        tag_id, blob_id);
    queue->Emplace(hash, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncTagRemoveBlob);
  void TagRemoveBlobRoot(const TagId &tag_id, const BlobId &blob_id) {
    TagRemoveBlobTask *task = AsyncTagRemoveBlobRoot(tag_id, blob_id);
    task->Wait();
    LABSTOR_CLIENT->DelTask(task);
  }

  /** Clear blobs from a tag */
  TagClearBlobsTask* AsyncTagClearBlobs(const TaskNode &task_node,
                                        const TagId &tag_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);
    u32 hash = tag_id.unique_;
    auto *task = LABSTOR_CLIENT->NewTask<TagClearBlobsTask>(
        p,
        task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        tag_id);
    queue->Emplace(hash, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncTagClearBlobs);
  void TagClearBlobsRoot(const TagId &tag_id) {
    TagClearBlobsTask *task = AsyncTagClearBlobsRoot(tag_id);
    task->Wait();
    LABSTOR_CLIENT->DelTask(task);
  }
};

}  // namespace labstor

#endif  // LABSTOR_hermes_bucket_mdm_H_
