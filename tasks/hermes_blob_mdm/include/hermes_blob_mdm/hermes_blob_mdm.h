//
// Created by lukemartinlogan on 6/29/23.
//

#ifndef LABSTOR_hermes_blob_mdm_H_
#define LABSTOR_hermes_blob_mdm_H_

#include "labstor/api/labstor_client.h"
#include "labstor/task_registry/task_lib.h"
#include "labstor_admin/labstor_admin.h"
#include "labstor/queue_manager/queue_manager_client.h"
#include "hermes/hermes_types.h"
#include "bdev/bdev.h"

namespace hermes::blob_mdm {

/** The set of methods in the hermes_blob_mdm task */
struct Method : public TaskMethod {
  TASK_METHOD_T kPutBlob = TaskMethod::kLast + 0;
  TASK_METHOD_T kGetBlob = TaskMethod::kLast + 1;
  TASK_METHOD_T kTruncateBlob = TaskMethod::kLast + 2;
  TASK_METHOD_T kDestroyBlob = TaskMethod::kLast + 3;
  TASK_METHOD_T kTagBlob = TaskMethod::kLast + 4;
  TASK_METHOD_T kUntagBlob = TaskMethod::kLast + 5;
  TASK_METHOD_T kBlobHasTag = TaskMethod::kLast + 6;
  TASK_METHOD_T kGetBlobTags = TaskMethod::kLast + 7;
  TASK_METHOD_T kGetBlobId = TaskMethod::kLast + 8;
  TASK_METHOD_T kGetBlobName = TaskMethod::kLast + 9;
  TASK_METHOD_T kGetBlobSize = TaskMethod::kLast + 10;
  TASK_METHOD_T kGetBlobScore = TaskMethod::kLast + 11;
  TASK_METHOD_T kGetBlobBuffers = TaskMethod::kLast + 12;
  TASK_METHOD_T kRenameBlob = TaskMethod::kLast + 13;
};


/** Phases of the construct task */
using labstor::Admin::CreateTaskStatePhase;
class ConstructTaskPhase : public CreateTaskStatePhase {
 public:
  TASK_METHOD_T kCreateTaskStates = kLast + 0;
  TASK_METHOD_T kWaitForTaskStates = kLast + 1;
  TASK_METHOD_T kCreateQueues = kLast + 2;
  TASK_METHOD_T kWaitForQueues = kLast + 3;
};

/**
 * A task to create hermes_mdm
 * */
using labstor::Admin::CreateTaskStateTask;
struct ConstructTask : public CreateTaskStateTask {
  HSHM_ALWAYS_INLINE
  ConstructTask(hipc::Allocator *alloc,
                const TaskNode &task_node,
                const DomainId &domain_id,
                const std::string &state_name,
                const TaskStateId &state_id)
      : CreateTaskStateTask(alloc, task_node, domain_id,
                            state_name,
                            "hermes_blob_mdm",
                            state_id) {
  }

  HSHM_ALWAYS_INLINE
  ~ConstructTask() {}
};

/** A task to destroy hermes_mdm */
using labstor::Admin::DestroyTaskStateTask;
struct DestructTask : public DestroyTaskStateTask {
  HSHM_ALWAYS_INLINE
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

/** A task to put data in a blob */
struct PutBlobTask : public Task {
  IN TagId tag_id_;
  IN hipc::ShmArchive<hipc::charbuf> blob_name_;
  IN size_t blob_off_;
  IN size_t data_size_;
  IN hipc::Pointer data_;
  IN float score_;
  IN bitfield32_t flags_;
  INOUT BlobId blob_id_;
  TEMP bool did_create_;
  TEMP int phase_;
  TEMP int plcmnt_idx_;
  TEMP int sub_plcmnt_idx_;
  TEMP hermes::bdev::AllocTask *cur_bdev_alloc_;
  TEMP hipc::ShmArchive<std::vector<PlacementSchema>> schema_;
  TEMP hipc::ShmArchive<std::vector<hermes::bdev::WriteTask*>> bdev_writes_;

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
    phase_ = PutBlobPhase::kCreate;
    plcmnt_idx_ = 0;
  }

  ~PutBlobTask() {
    HSHM_DESTROY_AR(blob_name_);
    LABSTOR_CLIENT->FreeBuffer(data_);
  }
};

/** Phases for the get task */
class GetBlobPhase {
 public:
  TASK_METHOD_T kStart = 0;
  TASK_METHOD_T kWait = 1;
};

/** A task to get data from a blob */
struct GetBlobTask : public Task {
  IN BlobId blob_id_;
  IN size_t blob_off_;
  INOUT ssize_t data_size_;
  OUT int phase_;
  OUT hipc::ShmArchive<std::vector<bdev::ReadTask*>> bdev_reads_;
  OUT hipc::Pointer data_;

  HSHM_ALWAYS_INLINE
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
};

/** A task to tag a blob */
struct TagBlobTask : public Task {
  IN BlobId blob_id_;
  IN TagId tag_;

  HSHM_ALWAYS_INLINE
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
};

/**
 * Check if blob has a tag
 * */
struct BlobHasTagTask : public Task {
  IN BlobId blob_id_;
  IN TagId tag_;
  OUT bool has_tag_;

  HSHM_ALWAYS_INLINE
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
};

/**
 * Get \a blob_name BLOB from \a bkt_id bucket
 * */
struct GetBlobIdTask : public Task {
  IN TagId tag_id_;
  IN hipc::ShmArchive<hipc::charbuf> blob_name_;
  OUT BlobId blob_id_;

  HSHM_ALWAYS_INLINE
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

  ~GetBlobIdTask() {
    HSHM_DESTROY_AR(blob_name_)
  }
};

/**
 * Get \a blob_name BLOB name from \a blob_id BLOB id
 * */
struct GetBlobNameTask : public Task {
  IN BlobId blob_id_;
  OUT hipc::ShmArchive<hipc::string> blob_name_;

  HSHM_ALWAYS_INLINE
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

  ~GetBlobNameTask() {
    HSHM_DESTROY_AR(blob_name_)
  };
};

/** Get \a score from \a blob_id BLOB id */
struct GetBlobScoreTask : public Task {
  IN BlobId blob_id_;
  OUT float score_;

  HSHM_ALWAYS_INLINE
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
};

/** Get \a blob_id blob's buffers */
struct GetBlobBuffersTask : public Task {
  IN BlobId blob_id_;
  OUT hipc::ShmArchive<hipc::vector<BufferInfo>> buffers_;

  HSHM_ALWAYS_INLINE
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

  ~GetBlobBuffersTask() {
    HSHM_DESTROY_AR(buffers_)
  }
};

/**
 * Rename \a blob_id blob to \a new_blob_name new blob name
 * in \a bkt_id bucket.
 * */
struct RenameBlobTask : public Task {
  IN BlobId blob_id_;
  IN hipc::ShmArchive<hipc::charbuf> new_blob_name_;

  HSHM_ALWAYS_INLINE
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

  ~RenameBlobTask() {
    HSHM_DESTROY_AR(new_blob_name_)
  }
};

/** A task to truncate a blob */
struct TruncateBlobTask : public Task {
  BlobId blob_id_;
  IN u64 size_;

  HSHM_ALWAYS_INLINE
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
};

/** Phases of the destroy blob task */
struct DestroyBlobPhase {
  TASK_METHOD_T kFreeBuffers = 0;
  TASK_METHOD_T kWaitFreeBuffers = 1;
};

/** A task to destroy a blob */
struct DestroyBlobTask : public Task {
  IN BlobId blob_id_;
  TEMP int phase_;
  TEMP hipc::ShmArchive<std::vector<bdev::FreeTask*>> free_tasks_;
  TEMP BlobInfo *blob_info_;
  TEMP

  HSHM_ALWAYS_INLINE
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
};

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
  void Create(const TaskNode &task_node,
              const DomainId &domain_id,
              const std::string &state_name) {
    id_ = TaskStateId::GetNull();
    id_ = LABSTOR_ADMIN->CreateTaskState<ConstructTask>(
        task_node,
        domain_id,
        state_name,
        id_);
    queue_id_ = QueueId(id_);
    LABSTOR_ADMIN->CreateQueue(task_node, domain_id, queue_id_,
                               LABSTOR_CLIENT->server_config_.queue_manager_.max_lanes_,
                               LABSTOR_CLIENT->server_config_.queue_manager_.max_lanes_,
                               LABSTOR_CLIENT->server_config_.queue_manager_.queue_depth_,
                               bitfield32_t(0));
  }
  LABSTOR_TASK_NODE_ROOT(Create);

  /** Destroy task state + queue */
  HSHM_ALWAYS_INLINE
  void Destroy(const TaskNode &task_node,
               const DomainId &domain_id) {
    LABSTOR_ADMIN->DestroyTaskState(task_node, domain_id, id_);
    LABSTOR_ADMIN->DestroyQueue(task_node, domain_id, queue_id_);
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
