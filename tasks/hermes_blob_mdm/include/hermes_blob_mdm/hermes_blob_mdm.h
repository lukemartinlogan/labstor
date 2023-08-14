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
  void Create(Args&& ...args) {
    auto *task = AsyncCreate(std::forward<Args>(args)...);
    task->Wait();
    AsyncCreateComplete(task);
  }
  LABSTOR_TASK_NODE_ROOT(Create);

  /** Destroy task state + queue */
  HSHM_ALWAYS_INLINE
  void Destroy(const TaskNode &task_node,
               const DomainId &domain_id) {
    LABSTOR_ADMIN->DestroyTaskState(task_node, domain_id, id_);
  }
  LABSTOR_TASK_NODE_ROOT(Destroy);

  /**
  * Create a blob's metadata
  *
  * @param tag_id id of the bucket
  * @param blob_name semantic blob name
  * @param[INOUT] blob_id the id of the blob
  * @param blob_off the offset of the data placed in existing blob
  * @param blob_size the amount of data being placed
  * @param blob a SHM pointer to the data to place
  * @param score the current score of the blob
  * @param replace whether to replace the blob if it exists
  * @param[OUT] did_create whether the blob was created or not
  * */
  Task* AsyncPutBlob(
      const TaskNode &task_node,
      TagId tag_id, const hshm::charbuf &blob_name,
      BlobId &blob_id, size_t blob_off, size_t blob_size,
      const hipc::Pointer &blob, float score,
      bitfield32_t flags) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    u32 hash = tag_id.unique_;
    auto *task = LABSTOR_CLIENT->NewTask<PutBlobTask>(
        p,
        task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        tag_id, blob_name, blob_id,
        blob_off, blob_size,
        blob, score, flags);
    queue->Emplace(hash, p);
    return task;
  }
  LABSTOR_TASK_NODE_ROOT(AsyncPutBlob);

  /**
  * Create a blob's metadata
  *
  * @param tag_id id of the bucket
  * @param blob_name semantic blob name
  * @param[INOUT] blob_id the id of the blob
  * @param blob_off the offset of the data placed in existing blob
  * @param blob_size the amount of data being placed
  * @param blob a SHM pointer to the data to place
  * @param score the current score of the blob
  * @param replace whether to replace the blob if it exists
  * @param[OUT] did_create whether the blob was created or not
  * */
  void PutBlob(
      const TaskNode &task_node,
      TagId tag_id, const hshm::charbuf &blob_name,
      BlobId &blob_id, size_t blob_off, size_t blob_size,
      const hipc::Pointer &blob, float score,
      bitfield32_t flags) {
    auto *task = AsyncPutBlob(task_node, tag_id, blob_name, blob_id,
                              blob_off, blob_size, blob, score, flags);
    task->Wait();
    // blob_id = task->blob_id_;
    // LABSTOR_CLIENT->DelTask(task);
  }
  LABSTOR_TASK_NODE_ROOT(PutBlob);

  /** Get a blob's data */
  hipc::Pointer GetBlob(const TaskNode &task_node,
                        BlobId blob_id, size_t off, ssize_t &size) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    u32 hash = blob_id.unique_;
    auto *task = LABSTOR_CLIENT->NewTask<GetBlobTask>(
        p,
        task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        blob_id, off, size);
    queue->Emplace(hash, p);
    task->Wait();
    hipc::Pointer data = task->data_;
    size = task->data_size_;
    LABSTOR_CLIENT->DelTask(task);
    return data;
  }
  LABSTOR_TASK_NODE_ROOT(GetBlob);

  /**
   * Tag a blob
   *
   * @param blob_id id of the blob being tagged
   * @param tag_name tag name
   * */
  void TagBlob(const TaskNode &task_node,
               BlobId blob_id, TagId tag_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    u32 hash = blob_id.unique_;
    auto *task = LABSTOR_CLIENT->NewTask<TagBlobTask>(
        p,
        task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        blob_id, tag_id);
    queue->Emplace(hash, p);
    task->Wait();
    LABSTOR_CLIENT->DelTask(task);
  }
  LABSTOR_TASK_NODE_ROOT(TagBlob);

  /**
   * Check if blob has a tag
   * */
  bool BlobHasTag(const TaskNode &task_node,
                  BlobId blob_id, TagId tag_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    u32 hash = blob_id.unique_;
    auto *task = LABSTOR_CLIENT->NewTask<BlobHasTagTask>(
        p,
        task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        blob_id, tag_id);
    queue->Emplace(hash, p);
    task->Wait();
    bool has_tag = task->has_tag_;
    LABSTOR_CLIENT->DelTask(task);
    return has_tag;
  }
  LABSTOR_TASK_NODE_ROOT(BlobHasTag);

  /**
   * Get \a blob_name BLOB from \a bkt_id bucket
   * */
  BlobId GetBlobId(const TaskNode &task_node,
                   TagId tag_id, const hshm::charbuf &blob_name) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    u32 hash = std::hash<hshm::charbuf>{}(blob_name);
    auto *task = LABSTOR_CLIENT->NewTask<GetBlobIdTask>(
        p,
        task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        tag_id, blob_name);
    queue->Emplace(hash, p);
    task->Wait();
    BlobId blob_id = task->blob_id_;
    LABSTOR_CLIENT->DelTask(task);
    return blob_id;
  }
  LABSTOR_TASK_NODE_ROOT(GetBlobId);

  /**
   * Get \a blob_name BLOB name from \a blob_id BLOB id
   * */
  std::string GetBlobName(const TaskNode &task_node,
                          BlobId blob_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    u32 hash = blob_id.unique_;
    auto *task = LABSTOR_CLIENT->NewTask<GetBlobNameTask>(
        p,
        task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        blob_id);
    queue->Emplace(hash, p);
    task->Wait();
    std::string blob_name = task->blob_name_->str();
    LABSTOR_CLIENT->DelTask(task);
    return blob_name;
  }
  LABSTOR_TASK_NODE_ROOT(GetBlobName);

  /**
   * Get \a score from \a blob_id BLOB id
   * */
  float GetBlobScore(const TaskNode &task_node,
                     BlobId blob_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    u32 hash = blob_id.unique_;
    auto *task = LABSTOR_CLIENT->NewTask<GetBlobScoreTask>(
        p,
        task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        blob_id);
    queue->Emplace(hash, p);
    task->Wait();
    float score = task->score_;
    LABSTOR_CLIENT->DelTask(task);
    return score;
  }
  LABSTOR_TASK_NODE_ROOT(GetBlobScore);

  /**
   * Get \a blob_id blob's buffers
   * */
  std::vector<BufferInfo> GetBlobBuffers(const TaskNode &task_node,
                                         BlobId blob_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    u32 hash = blob_id.unique_;
    auto *task = LABSTOR_CLIENT->NewTask<GetBlobBuffersTask>(
        p,
        task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        blob_id);
    queue->Emplace(hash, p);
    task->Wait();
    std::vector<BufferInfo> buffers =
        hshm::to_stl_vector<BufferInfo>(*task->buffers_);
    LABSTOR_CLIENT->DelTask(task);
    return buffers;
  }
  LABSTOR_TASK_NODE_ROOT(GetBlobBuffers);

  /**
   * Rename \a blob_id blob to \a new_blob_name new blob name
   * in \a bkt_id bucket.
   * */
  void RenameBlob(const TaskNode &task_node,
                  BlobId blob_id, const hshm::charbuf &new_blob_name) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    u32 hash = blob_id.unique_;
    auto *task = LABSTOR_CLIENT->NewTask<RenameBlobTask>(
        p,
        task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        blob_id, new_blob_name);
    queue->Emplace(hash, p);
    task->Wait();
    LABSTOR_CLIENT->DelTask(task);
  }
  LABSTOR_TASK_NODE_ROOT(RenameBlob);

  /**
   * Truncate a blob to a new size
   * */
  void TruncateBlob(const TaskNode &task_node,
                    BlobId blob_id, size_t new_size) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    u32 hash = blob_id.unique_;
    auto *task = LABSTOR_CLIENT->NewTask<TruncateBlobTask>(
        p,
        task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        blob_id, new_size);
    queue->Emplace(hash, p);
    task->Wait();
    LABSTOR_CLIENT->DelTask(task);
  }
  LABSTOR_TASK_NODE_ROOT(TruncateBlob);

  /**
   * Destroy \a blob_id blob in \a bkt_id bucket
   * */
  void DestroyBlob(const TaskNode &task_node,
                   BlobId blob_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    u32 hash = blob_id.unique_;
    auto *task = LABSTOR_CLIENT->NewTask<DestroyBlobTask>(
        p,
        task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        blob_id);
    queue->Emplace(hash, p);
    task->Wait();
    LABSTOR_CLIENT->DelTask(task);
  }
  LABSTOR_TASK_NODE_ROOT(DestroyBlob);
};

}  // namespace labstor

#endif  // LABSTOR_hermes_blob_mdm_H_
