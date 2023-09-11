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
  void AsyncPutBlobConstruct(PutBlobTask *task,
                             const TaskNode &task_node,
                             TagId tag_id, const BlobId &blob_id,
                             size_t blob_off, size_t blob_size,
                             bitfield32_t flags) {
    LABSTOR_CLIENT->ConstructTask<PutBlobTask>(
        task, task_node, DomainId::GetNode(tag_id.node_id_), id_,
        tag_id, blob_id,
        blob_off, blob_size, flags);
  }
  LABSTOR_TASK_NODE_PUSH_ROOT(PutBlob);

  /** Append data to the bucket (fire & forget) */
  HSHM_ALWAYS_INLINE
  void AsyncAppendBlobSchemaConstruct(AppendBlobSchemaTask *task,
                                      const TaskNode &task_node,
                                      TagId tag_id,
                                      size_t data_size,
                                      size_t page_size) {
    LABSTOR_CLIENT->ConstructTask<AppendBlobSchemaTask>(
        task, task_node, DomainId::GetNode(tag_id.node_id_), id_,
        tag_id, data_size, page_size);
  }
  template<typename ...Args>\
  hipc::LPointer<AppendBlobSchemaTask> AsyncAppendBlobSchemaAlloc(const TaskNode &task_node,\
                                Args&& ...args) {\
    hipc::LPointer<AppendBlobSchemaTask> task = LABSTOR_CLIENT->AllocTask<AppendBlobSchemaTask>();\
    AsyncAppendBlobSchemaConstruct(task.ptr_, task_node, std::forward<Args>(args)...);\
    return task;\
  }\
  template<typename ...Args>\
  hipc::LPointer<AppendBlobSchemaTask> AsyncAppendBlobSchema(const TaskNode &task_node,\
                           Args&& ...args) {\
    hipc::LPointer<AppendBlobSchemaTask> task = AsyncAppendBlobSchemaAlloc(task_node, std::forward<Args>(args)...);\
    MultiQueue *queue = LABSTOR_CLIENT->GetQueue(queue_id_);\
    queue->Emplace(task.ptr_->lane_hash_, task.shm_);\
    return task;\
  }\
  template<typename ...Args>\
  hipc::LPointer<labpq::TypedPushTask<AppendBlobSchemaTask>> AsyncAppendBlobSchemaRoot(Args&& ...args) {\
    TaskNode task_node = LABSTOR_CLIENT->MakeTaskNodeId();\
    hipc::LPointer<AppendBlobSchemaTask> task = AsyncAppendBlobSchemaAlloc(task_node + 1, std::forward<Args>(args)...);\
    hipc::LPointer<labpq::TypedPushTask<AppendBlobSchemaTask>> push_task =\
        LABSTOR_PROCESS_QUEUE->AsyncPush<AppendBlobSchemaTask>(task_node,\
                                                 DomainId::GetLocal(),\
                                                 task.shm_);\
    return push_task;\
  }
  // LABSTOR_TASK_NODE_PUSH_ROOT(AppendBlobSchema);

  /** Append data to the bucket (fire & forget) */
  HSHM_ALWAYS_INLINE
  void AsyncAppendBlobConstruct(
      AppendBlobTask *task,
      const TaskNode &task_node,
      TagId tag_id,
      size_t data_size,
      const hipc::Pointer &data,
      size_t page_size,
      float score,
      u32 node_id) {
    LABSTOR_CLIENT->ConstructTask<AppendBlobTask>(
        task, task_node, DomainId::GetNode(tag_id.node_id_), id_,
        tag_id, data_size, data, page_size, score, node_id);
  }
  HSHM_ALWAYS_INLINE
  void AppendBlobRoot(TagId tag_id,
                      size_t data_size,
                      const hipc::Pointer &data,
                      size_t page_size,
                      float score,
                      u32 node_id) {
    AsyncAppendBlobRoot(tag_id, data_size, data, page_size, score, node_id);
  }
  LABSTOR_TASK_NODE_PUSH_ROOT(AppendBlob);

  /** Create a tag or get the ID of existing tag */
  HSHM_ALWAYS_INLINE
  void AsyncGetOrCreateTagConstruct(GetOrCreateTagTask *task,
                                    const TaskNode &task_node,
                                    const hshm::charbuf &tag_name,
                                    bool blob_owner,
                                    const std::vector<TraitId> &traits,
                                    size_t backend_size) {
    HILOG(kDebug, "Creating a tag {}", tag_name.str());
    u32 hash = std::hash<hshm::charbuf>{}(tag_name);
    LABSTOR_CLIENT->ConstructTask<GetOrCreateTagTask>(
        task, task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        tag_name, blob_owner, traits, backend_size);
  }
  HSHM_ALWAYS_INLINE
  TagId GetOrCreateTagRoot(const hshm::charbuf &tag_name,
                           bool blob_owner,
                           const std::vector<TraitId> &traits,
                           size_t backend_size) {
    LPointer<labpq::TypedPushTask<GetOrCreateTagTask>> push_task = 
        AsyncGetOrCreateTagRoot(tag_name, blob_owner, traits, backend_size);
    push_task->Wait();
    GetOrCreateTagTask *task = push_task->get();
    TagId tag_id = task->tag_id_;
    LABSTOR_CLIENT->DelTask(push_task);
    return tag_id;
  }
  LABSTOR_TASK_NODE_PUSH_ROOT(GetOrCreateTag);

  /** Get tag ID */
  void AsyncGetTagIdConstruct(GetTagIdTask *task,
                              const TaskNode &task_node,
                              const hshm::charbuf &tag_name) {
    u32 hash = std::hash<hshm::charbuf>{}(tag_name);
    LABSTOR_CLIENT->ConstructTask<GetTagIdTask>(
        task, task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        tag_name);
  }
  TagId GetTagIdRoot(const hshm::charbuf &tag_name) {
    LPointer<labpq::TypedPushTask<GetTagIdTask>> push_task = 
        AsyncGetTagIdRoot(tag_name);
    push_task->Wait();
    GetTagIdTask *task = push_task->get();
    TagId tag_id = task->tag_id_;
    LABSTOR_CLIENT->DelTask(push_task);
    return tag_id;
  }
  LABSTOR_TASK_NODE_PUSH_ROOT(GetTagId);

  /** Get tag name */
  void AsyncGetTagNameConstruct(GetTagNameTask *task,
                                const TaskNode &task_node,
                                const TagId &tag_id) {
    u32 hash = tag_id.unique_;
    LABSTOR_CLIENT->ConstructTask<GetTagNameTask>(
        task, task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        tag_id);
  }
  hshm::string GetTagNameRoot(const TagId &tag_id) {
    LPointer<labpq::TypedPushTask<GetTagNameTask>> push_task = 
        AsyncGetTagNameRoot(tag_id);
    push_task->Wait();
    GetTagNameTask *task = push_task->get();
    hshm::string tag_name = hshm::to_charbuf<hipc::string>(*task->tag_name_.get());
    LABSTOR_CLIENT->DelTask(push_task);
    return tag_name;
  }
  LABSTOR_TASK_NODE_PUSH_ROOT(GetTagName);

  /** Rename tag */
  void AsyncRenameTagConstruct(RenameTagTask *task,
                               const TaskNode &task_node,
                               const TagId &tag_id,
                               const hshm::charbuf &new_tag_name) {
    u32 hash = tag_id.unique_;
    LABSTOR_CLIENT->ConstructTask<RenameTagTask>(
        task, task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        tag_id, new_tag_name);
  }
  void RenameTagRoot(const TagId &tag_id, const hshm::charbuf &new_tag_name) {
    LPointer<labpq::TypedPushTask<RenameTagTask>> push_task = 
        AsyncRenameTagRoot(tag_id, new_tag_name);
    push_task->Wait();
    LABSTOR_CLIENT->DelTask(push_task);
  }
  LABSTOR_TASK_NODE_PUSH_ROOT(RenameTag);

  /** Destroy tag */
  void AsyncDestroyTagConstruct(DestroyTagTask *task,
                                const TaskNode &task_node,
                                const TagId &tag_id) {
    u32 hash = tag_id.unique_;
    LABSTOR_CLIENT->ConstructTask<DestroyTagTask>(
        task, task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        tag_id);
  }
  void DestroyTagRoot(const TagId &tag_id) {
    LPointer<labpq::TypedPushTask<DestroyTagTask>> push_task = 
        AsyncDestroyTagRoot(tag_id);
    push_task->Wait();
    LABSTOR_CLIENT->DelTask(push_task);
  }
  LABSTOR_TASK_NODE_PUSH_ROOT(DestroyTag);

  /** Add a blob to a tag */
  void AsyncTagAddBlobConstruct(TagAddBlobTask *task,
                                const TaskNode &task_node,
                                const TagId &tag_id,
                                const BlobId &blob_id) {
    u32 hash = tag_id.unique_;
    LABSTOR_CLIENT->ConstructTask<TagAddBlobTask>(
        task, task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        tag_id, blob_id);
  }
  void TagAddBlobRoot(const TagId &tag_id, const BlobId &blob_id) {
    LPointer<labpq::TypedPushTask<TagAddBlobTask>> push_task = 
        AsyncTagAddBlobRoot(tag_id, blob_id);
    push_task->Wait();
    LABSTOR_CLIENT->DelTask(push_task);
  }
  LABSTOR_TASK_NODE_PUSH_ROOT(TagAddBlob);

  /** Remove a blob from a tag */
  void AsyncTagRemoveBlobConstruct(TagRemoveBlobTask *task,
                                   const TaskNode &task_node,
                                   const TagId &tag_id, const BlobId &blob_id) {
    u32 hash = tag_id.unique_;
    LABSTOR_CLIENT->ConstructTask<TagRemoveBlobTask>(
        task, task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        tag_id, blob_id);
  }
  void TagRemoveBlobRootConstruct(const TagId &tag_id, const BlobId &blob_id) {
    LPointer<labpq::TypedPushTask<TagRemoveBlobTask>> push_task = 
        AsyncTagRemoveBlobRoot(tag_id, blob_id);
    push_task->Wait();
    LABSTOR_CLIENT->DelTask(push_task);
  }
  LABSTOR_TASK_NODE_PUSH_ROOT(TagRemoveBlob);

  /** Clear blobs from a tag */
  void AsyncTagClearBlobsConstruct(TagClearBlobsTask *task,
                                   const TaskNode &task_node,
                                   const TagId &tag_id) {
    u32 hash = tag_id.unique_;
    LABSTOR_CLIENT->ConstructTask<TagClearBlobsTask>(
        task, task_node, DomainId::GetNode(HASH_TO_NODE_ID(hash)), id_,
        tag_id);
  }
  void TagClearBlobsRoot(const TagId &tag_id) {
    LPointer<labpq::TypedPushTask<TagClearBlobsTask>> push_task = 
        AsyncTagClearBlobsRoot(tag_id);
    push_task->Wait();
    LABSTOR_CLIENT->DelTask(push_task);
  }
  LABSTOR_TASK_NODE_PUSH_ROOT(TagClearBlobs);
};

}  // namespace labstor

#endif  // LABSTOR_hermes_bucket_mdm_H_
