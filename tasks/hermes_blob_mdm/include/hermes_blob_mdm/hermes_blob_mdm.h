//
// Created by lukemartinlogan on 6/29/23.
//

#ifndef LABSTOR_hermes_blob_mdm_H_
#define LABSTOR_hermes_blob_mdm_H_

#include "hermes_blob_mdm_tasks.h"

namespace hermes::blob_mdm {

/** Create hermes_blob_mdm requests */
class Client {
 public:
  TaskStateId id_;
  QueueId queue_id_;

 public:
  /** Default constructor */
  Client() = default;

  /** Destructor */
  ~Client() = default;

  /** Initialize directly using TaskStateId */
  void Init(const TaskStateId &id) {
    id_ = id;
    queue_id_ = QueueId(id_);
  }

  /** Create a hermes_blob_mdm */
  HSHM_ALWAYS_INLINE
  ConstructTask* AsyncCreate(const TaskNode &task_node,
                             const DomainId &domain_id,
                             const std::string &state_name) {
    id_ = TaskStateId::GetNull();
    return LABSTOR_ADMIN->AsyncCreateTaskState<ConstructTask>(
        task_node, domain_id, state_name, id_,
        LABSTOR_CLIENT->server_config_.queue_manager_.max_lanes_,
        LABSTOR_CLIENT->server_config_.queue_manager_.max_lanes_,
        LABSTOR_CLIENT->server_config_.queue_manager_.queue_depth_,
        bitfield32_t(0));
  }
  LABSTOR_TASK_NODE_ROOT(AsyncCreate);

  /** Complete async create task */
  void AsyncCreateComplete(ConstructTask *task) {
    if (task->IsComplete()) {
      id_ = task->id_;
      queue_id_ = QueueId(id_);
      LABSTOR_CLIENT->DelTask(task);
    }
  }

  template<typename ...Args>
  HSHM_ALWAYS_INLINE
  void CreateRoot(Args&& ...args) {
    auto *task = AsyncCreateRoot(std::forward<Args>(args)...);
    task->Wait();
    AsyncCreateComplete(task);
  }

  /** Destroy task state + queue */
  HSHM_ALWAYS_INLINE
  void DestroyRoot(const DomainId &domain_id) {
    LABSTOR_ADMIN->DestroyTaskStateRoot(domain_id, id_);
  }

  /**
   * Get \a blob_name BLOB from \a bkt_id bucket
   * */
  GetOrCreateBlobIdTask* AsyncGetOrCreateBlobId(const TaskNode &task_node,
                                                TagId tag_id, const hshm::charbuf &blob_name) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);
    u32 hash = std::hash<hshm::charbuf>{}(blob_name);
    auto *task = LABSTOR_CLIENT->NewTask<GetOrCreateBlobIdTask>(
        p,
        task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        tag_id, blob_name);
    queue->Emplace(hash, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncGetOrCreateBlobId);
  BlobId GetOrCreateBlobIdRoot(TagId tag_id, const hshm::charbuf &blob_name) {
    GetOrCreateBlobIdTask *task = AsyncGetOrCreateBlobIdRoot(tag_id, blob_name);
    task->Wait();
    BlobId blob_id = task->blob_id_;
    LABSTOR_CLIENT->DelTask(task);
    return blob_id;
  }

  /**
  * Create a blob's metadata
  *
  * @param tag_id id of the bucket
  * @param blob_name semantic blob name
  * @param blob_id the id of the blob
  * @param blob_off the offset of the data placed in existing blob
  * @param blob_size the amount of data being placed
  * @param blob a SHM pointer to the data to place
  * @param score the current score of the blob
  * @param replace whether to replace the blob if it exists
  * @param[OUT] did_create whether the blob was created or not
  * */
  PutBlobTask* AsyncPutBlob(
      const TaskNode &task_node,
      TagId tag_id, const hshm::charbuf &blob_name,
      BlobId &blob_id, size_t blob_off, size_t blob_size,
      const hipc::Pointer &blob, float score,
      bitfield32_t flags,
      bitfield32_t task_flags = bitfield32_t(TASK_FIRE_AND_FORGET | TASK_OWNS_DATA)) {
    HILOG(kDebug, "Beginning PUT (task_node={})", task_node);
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);
    u32 hash = blob_id.unique_;
    auto *task = LABSTOR_CLIENT->NewTask<PutBlobTask>(
        p,
        task_node, DomainId::GetNode(blob_id.node_id_), id_,
        tag_id, blob_name, blob_id,
        blob_off, blob_size,
        blob, score, flags);
    task->task_flags_.SetBits(task_flags.bits_);
    queue->Emplace(hash, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncPutBlob);

  /** Get a blob's data */
  GetBlobTask* AsyncGetBlob(const TaskNode &task_node,
                            const TagId &tag_id,
                            const BlobId &blob_id,
                            size_t off,
                            ssize_t data_size,
                            hipc::Pointer &data) {
    HILOG(kDebug, "Beginning GET (task_node={})", task_node);
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);
    u32 hash = blob_id.unique_;
    auto *task = LABSTOR_CLIENT->NewTask<GetBlobTask>(
        p,
        task_node, DomainId::GetNode(blob_id.node_id_), id_,
        tag_id, blob_id, off, data_size, data);
    queue->Emplace(hash, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncGetBlob);
  size_t GetBlobRoot(const TagId &tag_id,
                     const BlobId &blob_id,
                     size_t off,
                     ssize_t data_size,
                     hipc::Pointer &data) {
    GetBlobTask *task = AsyncGetBlobRoot(tag_id, blob_id, off, data_size, data);
    task->Wait();
    data = task->data_;
    size_t true_size = task->data_size_;
    LABSTOR_CLIENT->DelTask(task);
    return true_size;
  }

  /**
   * Reorganize a blob
   *
   * @param blob_id id of the blob being reorganized
   * @param score the new score of the blob
   * @param node_id the node to reorganize the blob to
   * */
  ReorganizeBlobTask* AsyncReorganizeBlob(const TaskNode &task_node,
                                          const TagId &tag_id,
                                          const BlobId &blob_id,
                                          float score,
                                          u32 node_id) {
    HILOG(kDebug, "Beginning REORGANIZE (task_node={})", task_node);
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);
    u32 hash = blob_id.unique_;
    auto *task = LABSTOR_CLIENT->NewTask<ReorganizeBlobTask>(
        p,
        task_node, DomainId::GetNode(blob_id.node_id_), id_,
        tag_id, blob_id, score, node_id);
    queue->Emplace(hash, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncReorganizeBlob);

  /**
   * Tag a blob
   *
   * @param blob_id id of the blob being tagged
   * @param tag_name tag name
   * */
  TagBlobTask* AsyncTagBlob(const TaskNode &task_node,
                            const TagId &tag_id,
                            const BlobId &blob_id,
                            const TagId &tag) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);
    u32 hash = blob_id.unique_;
    auto *task = LABSTOR_CLIENT->NewTask<TagBlobTask>(
        p,
        task_node, DomainId::GetNode(blob_id.node_id_), id_,
        tag_id, blob_id, tag);
    queue->Emplace(hash, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncTagBlob);
  void TagBlobRoot(const TagId &tag_id,
                   const BlobId &blob_id,
                   const TagId &tag) {
    TagBlobTask *task = AsyncTagBlobRoot(tag_id, blob_id, tag);
    task->Wait();
    LABSTOR_CLIENT->DelTask(task);
  }

  /**
   * Check if blob has a tag
   * */
  BlobHasTagTask* AsyncBlobHasTag(const TaskNode &task_node,
                                  const TagId &tag_id,
                                  const BlobId &blob_id,
                                  const TagId &tag) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);
    u32 hash = blob_id.unique_;
    auto *task = LABSTOR_CLIENT->NewTask<BlobHasTagTask>(
        p,
        task_node, DomainId::GetNode(blob_id.node_id_), id_,
        tag_id, blob_id, tag);
    queue->Emplace(hash, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncBlobHasTag);
  bool BlobHasTagRoot(const TagId &tag_id,
                      const BlobId &blob_id,
                      const TagId &tag) {
    BlobHasTagTask *task = AsyncBlobHasTagRoot(tag_id, blob_id, tag);
    task->Wait();
    bool has_tag = task->has_tag_;
    LABSTOR_CLIENT->DelTask(task);
    return has_tag;
  }

  /**
   * Get \a blob_name BLOB from \a bkt_id bucket
   * */
  GetBlobIdTask* AsyncGetBlobId(const TaskNode &task_node,
                                const TagId &tag_id,
                                const hshm::charbuf &blob_name) {
    hipc::Pointer p;
    u32 hash = std::hash<hshm::charbuf>{}(blob_name);
    auto *task = LABSTOR_CLIENT->NewTask<GetBlobIdTask>(
        p,
        task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        tag_id, blob_name);
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);
    queue->Emplace(hash, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncGetBlobId);
  BlobId GetBlobIdRoot(const TagId &tag_id,
                       const hshm::charbuf &blob_name) {
    GetBlobIdTask *task = AsyncGetBlobIdRoot(tag_id, blob_name);
    task->Wait();
    BlobId blob_id = task->blob_id_;
    LABSTOR_CLIENT->DelTask(task);
    return blob_id;
  }

  /**
   * Get \a blob_name BLOB name from \a blob_id BLOB id
   * */
  GetBlobNameTask* AsyncGetBlobName(const TaskNode &task_node,
                                    const TagId &tag_id,
                                    const BlobId &blob_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);
    u32 hash = blob_id.unique_;
    auto *task = LABSTOR_CLIENT->NewTask<GetBlobNameTask>(
        p,
        task_node, DomainId::GetNode(blob_id.node_id_), id_,
        tag_id, blob_id);
    queue->Emplace(hash, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncGetBlobName);
  std::string GetBlobNameRoot(const TagId &tag_id,
                              const BlobId &blob_id) {
    GetBlobNameTask *task = AsyncGetBlobNameRoot(tag_id, blob_id);
    task->Wait();
    std::string blob_name = task->blob_name_->str();
    LABSTOR_CLIENT->DelTask(task);
    return blob_name;
  }

  /**
   * Get \a size from \a blob_id BLOB id
   * */
  GetBlobSizeTask* AsyncGetBlobSize(const TaskNode &task_node,
                                    const TagId &tag_id,
                                    const BlobId &blob_id) {
    HILOG(kDebug, "Getting blob size {}", task_node);
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);
    u32 hash = blob_id.unique_;
    auto *task = LABSTOR_CLIENT->NewTask<GetBlobSizeTask>(
        p,
        task_node, DomainId::GetNode(blob_id.node_id_), id_,
        tag_id, blob_id);
    queue->Emplace(hash, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncGetBlobSize);
  size_t GetBlobSizeRoot(const TagId &tag_id,
                         const BlobId &blob_id) {
    GetBlobSizeTask *task = AsyncGetBlobSizeRoot(tag_id, blob_id);
    task->Wait();
    size_t size = task->size_;
    LABSTOR_CLIENT->DelTask(task);
    return size;
  }

  /**
   * Get \a score from \a blob_id BLOB id
   * */
  GetBlobScoreTask* AsyncGetBlobScore(const TaskNode &task_node,
                                      const TagId &tag_id,
                                      const BlobId &blob_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);
    u32 hash = blob_id.unique_;
    auto *task = LABSTOR_CLIENT->NewTask<GetBlobScoreTask>(
        p,
        task_node, DomainId::GetNode(blob_id.node_id_), id_,
        tag_id, blob_id);
    queue->Emplace(hash, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncGetBlobScore);
  float GetBlobScoreRoot(const TagId &tag_id,
                         const BlobId &blob_id) {
    GetBlobScoreTask *task = AsyncGetBlobScoreRoot(tag_id, blob_id);
    task->Wait();
    float score = task->score_;
    LABSTOR_CLIENT->DelTask(task);
    return score;
  }

  /**
   * Get \a blob_id blob's buffers
   * */
  GetBlobBuffersTask* AsyncGetBlobBuffers(const TaskNode &task_node,
                                          const TagId &tag_id,
                                          const BlobId &blob_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);
    u32 hash = blob_id.unique_;
    auto *task = LABSTOR_CLIENT->NewTask<GetBlobBuffersTask>(
        p, task_node, DomainId::GetNode(blob_id.node_id_), id_,
        tag_id, blob_id);
    queue->Emplace(hash, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncGetBlobBuffers)
  std::vector<BufferInfo> GetBlobBuffersRoot(const TagId &tag_id,
                                             const BlobId &blob_id) {
    GetBlobBuffersTask *task = AsyncGetBlobBuffersRoot(tag_id, blob_id);
    task->Wait();
    std::vector<BufferInfo> buffers =
        hshm::to_stl_vector<BufferInfo>(*task->buffers_);
    LABSTOR_CLIENT->DelTask(task);
    return buffers;
  }

  /**
   * Rename \a blob_id blob to \a new_blob_name new blob name
   * in \a bkt_id bucket.
   * */
  RenameBlobTask* AsyncRenameBlob(const TaskNode &task_node,
                                  const TagId &tag_id,
                                  const BlobId &blob_id,
                                  const hshm::charbuf &new_blob_name) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);
    u32 hash = blob_id.unique_;
    auto *task = LABSTOR_CLIENT->NewTask<RenameBlobTask>(
        p,
        task_node, DomainId::GetNode(blob_id.node_id_), id_,
        tag_id, blob_id, new_blob_name);
    queue->Emplace(hash, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncRenameBlob);
  void RenameBlobRoot(const TagId &tag_id,
                      const BlobId &blob_id,
                      const hshm::charbuf &new_blob_name) {
    RenameBlobTask *task = AsyncRenameBlobRoot(tag_id, blob_id, new_blob_name);
    task->Wait();
    LABSTOR_CLIENT->DelTask(task);
  }

  /**
   * Truncate a blob to a new size
   * */
  TruncateBlobTask* AsyncTruncateBlob(const TaskNode &task_node,
                                      const TagId &tag_id,
                                      const BlobId &blob_id,
                                      size_t new_size) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);
    u32 hash = blob_id.unique_;
    auto *task = LABSTOR_CLIENT->NewTask<TruncateBlobTask>(
        p,
        task_node, DomainId::GetNode(blob_id.node_id_), id_,
        tag_id, blob_id, new_size);
    queue->Emplace(hash, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncTruncateBlob);
  void TruncateBlobRoot(const TagId &tag_id,
                        const BlobId &blob_id,
                        size_t new_size) {
    TruncateBlobTask *task = AsyncTruncateBlobRoot(tag_id, blob_id, new_size);
    task->Wait();
    LABSTOR_CLIENT->DelTask(task);
  }

  /**
   * Destroy \a blob_id blob in \a bkt_id bucket
   * */
  DestroyBlobTask* AsyncDestroyBlob(const TaskNode &task_node,
                                    const TagId &tag_id,
                                    const BlobId &blob_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);
    u32 hash = blob_id.unique_;
    auto *task = LABSTOR_CLIENT->NewTask<DestroyBlobTask>(
        p,
        task_node, DomainId::GetNode(blob_id.node_id_), id_,
        tag_id, blob_id);
    queue->Emplace(hash, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncDestroyBlob);
  void DestroyBlobRoot(const TagId &tag_id,
                       const BlobId &blob_id) {
    DestroyBlobTask *task = AsyncDestroyBlobRoot(tag_id, blob_id);
    task->Wait();
    LABSTOR_CLIENT->DelTask(task);
  }
};

}  // namespace labstor

#endif  // LABSTOR_hermes_blob_mdm_H_
