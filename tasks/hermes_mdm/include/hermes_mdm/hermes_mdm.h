//
// Created by lukemartinlogan on 6/29/23.
//

#ifndef LABSTOR_hermes_mdm_H_
#define LABSTOR_hermes_mdm_H_

#include "labstor/api/labstor_client.h"
#include "labstor/task_registry/task_lib.h"
#include "labstor_admin/labstor_admin.h"
#include "labstor/queue_manager/queue_manager_client.h"
#include "hermes/hermes_types.h"

namespace hermes::mdm {

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

/** A task to create hermes_mdm */
struct ConstructTask : public Task {
  hipc::ShmArchive<hipc::string> server_config_path_;

  HSHM_ALWAYS_INLINE
  ConstructTask(hipc::Allocator *alloc,
                const TaskStateId &state_id,
                const DomainId &domain_id,
                const std::string &server_config_path = "") : Task(alloc) {
    // Initialize task
    key_ = 0;
    task_state_ = state_id;
    method_ = Method::kConstruct;
    task_flags_.SetBits(0);
    domain_id_ = domain_id;

    // Custom params
    HSHM_MAKE_AR(server_config_path_, alloc, server_config_path);
  }

  HSHM_ALWAYS_INLINE
  ~ConstructTask() {
    HSHM_DESTROY_AR(server_config_path_);
  }
};

/** A task to destroy hermes_mdm */
struct DestructTask : public Task {
  HSHM_ALWAYS_INLINE
  DestructTask(hipc::Allocator *alloc,
               TaskStateId &state_id,
               const DomainId &domain_id) : Task(alloc) {
    // Initialize task
    key_ = 0;
    task_state_ = state_id;
    method_ = Method::kDestruct;
    task_flags_.SetBits(0);
    domain_id_ = domain_id;
  }
};

/**====================================
 * Tag Operations
 * ===================================*/

/** A task to get or create a tag */
struct GetOrCreateTagTask : public Task {
  IN hipc::ShmArchive<hipc::string> tag_name_;
  IN bool blob_owner_;
  IN hipc::ShmArchive<hipc::vector<TraitId>> traits_;
  IN size_t backend_size_;
  OUT TagId tag_id_;

  HSHM_ALWAYS_INLINE
  GetOrCreateTagTask(hipc::Allocator *alloc,
                     const TaskStateId &state_id,
                     const DomainId &domain_id,
                     const hshm::charbuf &tag_name,
                     bool blob_owner,
                     const std::vector<TraitId> &traits,
                     size_t backend_size) : Task(alloc) {
    // Initialize task
    key_ = std::hash<hshm::charbuf>{}(tag_name);
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
               const TaskStateId &state_id,
               const DomainId &domain_id,
               const hshm::charbuf &tag_name) : Task(alloc) {
    // Initialize task
    key_ = std::hash<hshm::charbuf>{}(tag_name);
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
                 const TaskStateId &state_id,
                 const DomainId &domain_id,
                 const TagId &tag_id) : Task(alloc) {
    // Initialize task
    key_ = tag_id.unique_;
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
                const TaskStateId &state_id,
                const DomainId &domain_id,
                const TagId &tag_id,
                const hshm::charbuf &tag_name) : Task(alloc) {
    // Initialize task
    key_ = tag_id.unique_;
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
                 const TaskStateId &state_id,
                 const DomainId &domain_id,
                 TagId tag_id) : Task(alloc) {
    // Initialize task
    key_ = tag_id.unique_;
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
                 const TaskStateId &state_id,
                 const DomainId &domain_id,
                 TagId tag_id,
                 const BlobId &blob_id) : Task(alloc) {
    // Initialize task
    key_ = tag_id.unique_;
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
                    const TaskStateId &state_id,
                    const DomainId &domain_id,
                    TagId tag_id,
                    const BlobId &blob_id) : Task(alloc) {
    // Initialize task
    key_ = tag_id.unique_;
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
                    const TaskStateId &state_id,
                    const DomainId &domain_id,
                    TagId tag_id) : Task(alloc) {
    // Initialize task
    key_ = tag_id.unique_;
    task_state_ = state_id;
    method_ = Method::kTagClearBlobs;
    task_flags_.SetBits(TASK_LOW_LATENCY);
    domain_id_ = domain_id;

    // Custom params
    tag_id_ = tag_id;
  }
};

/**====================================
 * Blob Operations
 * ===================================*/

/** A task to put data in a blob */
struct PutBlobTask : public Task {
  IN hshm::charbuf blob_name_;
  IN std::vector<BufferInfo> buffers_;
  INOUT BlobId blob_id_;

  HSHM_ALWAYS_INLINE
  PutBlobTask(hipc::Allocator *alloc,
              const TaskStateId &state_id,
              const DomainId &domain_id,
              const BlobId &blob_id,
              const hshm::charbuf &blob_name,
              const std::vector<BufferInfo> &buffers) : Task(alloc) {
    // Initialize task
    key_ = blob_id.unique_;
    task_state_ = state_id;
    method_ = Method::kPutBlob;
    task_flags_.SetBits(TASK_LOW_LATENCY);
    domain_id_ = domain_id;

    // Custom params
    blob_id_ = blob_id;
    buffers_ = buffers;
    blob_name_ = blob_name;
  }
};

/** A task to get data from a blob */
struct GetBlobTask : public Task {
  BlobId blob_id_;
  OUT hipc::ShmArchive<hipc::string> data_;

  HSHM_ALWAYS_INLINE
  GetBlobTask(hipc::Allocator *alloc,
              const TaskStateId &state_id,
              const DomainId &domain_id,
              const BlobId &blob_id) : Task(alloc) {
    // Initialize task
    key_ = blob_id.unique_;
    task_state_ = state_id;
    method_ = Method::kGetBlob;
    task_flags_.SetBits(TASK_LOW_LATENCY);
    domain_id_ = domain_id;

    // Custom params
    blob_id_ = blob_id;
  }
};

/** A task to tag a blob */
struct TagBlobTask : public Task {
  IN BlobId blob_id_;
  IN TagId tag_;

  HSHM_ALWAYS_INLINE
  TagBlobTask(hipc::Allocator *alloc,
              const TaskStateId &state_id,
              const DomainId &domain_id,
              const BlobId &blob_id,
              const TagId &tag) : Task(alloc) {
    // Initialize task
    key_ = blob_id.unique_;
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
                 const TaskStateId &state_id,
                 const DomainId &domain_id,
                 const BlobId &blob_id,
                 const TagId &tag) : Task(alloc) {
    // Initialize task
    key_ = blob_id.unique_;
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
  IN hipc::ShmArchive<hipc::charbuf> blob_name_;
  OUT BlobId blob_id_;

  HSHM_ALWAYS_INLINE
  GetBlobIdTask(hipc::Allocator *alloc,
                const TaskStateId &state_id,
                const DomainId &domain_id,
                const hshm::charbuf &blob_name) : Task(alloc) {
    // Initialize task
    key_ = std::hash<hshm::charbuf>{}(blob_name);
    task_state_ = state_id;
    method_ = Method::kGetBlobId;
    task_flags_.SetBits(TASK_LOW_LATENCY);
    domain_id_ = domain_id;

    // Custom
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
                  const TaskStateId &state_id,
                  const DomainId &domain_id,
                  const BlobId &blob_id) : Task(alloc) {
    // Initialize task
    key_ = blob_id.unique_;
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
                   const TaskStateId &state_id,
                   const DomainId &domain_id,
                   const BlobId &blob_id) : Task(alloc) {
    // Initialize task
    key_ = blob_id.unique_;
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
                     const TaskStateId &state_id,
                     const DomainId &domain_id,
                     const BlobId &blob_id) : Task(alloc) {
    // Initialize task
    key_ = blob_id.unique_;
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
                 const TaskStateId &state_id,
                 const DomainId &domain_id,
                 const BlobId &blob_id,
                 const hshm::charbuf &new_blob_name) : Task(alloc) {
    // Initialize task
    key_ = blob_id.unique_;
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
                   const TaskStateId &state_id,
                   const DomainId &domain_id,
                   const BlobId &blob_id,
                   u64 size) : Task(alloc) {
    // Initialize task
    key_ = blob_id.unique_;
    task_state_ = state_id;
    method_ = Method::kTruncateBlob;
    task_flags_.SetBits(TASK_LOW_LATENCY);
    domain_id_ = domain_id;

    // Custom params
    blob_id_ = blob_id;
    size_ = size;
  }
};

/** A task to destroy a blob */
struct DestroyBlobTask : public Task {
  BlobId blob_id_;

  HSHM_ALWAYS_INLINE
  DestroyBlobTask(hipc::Allocator *alloc,
                  const TaskStateId &state_id,
                  const DomainId &domain_id,
                  const BlobId &blob_id) : Task(alloc) {
    // Initialize task
    key_ = blob_id.unique_;
    task_state_ = state_id;
    method_ = Method::kDestroyBlob;
    task_flags_.SetBits(TASK_LOW_LATENCY);
    domain_id_ = domain_id;

    // Custom params
    blob_id_ = blob_id;
  }
};

/**====================================
 * Client API
 * ===================================*/

/** Create requests */
class Client {
 public:
  TaskStateId id_;
  QueueId queue_id_;

 public:
  /** Default constructor */
  Client() = default;

  /** Destructor */
  ~Client() = default;

  /** Create a hermes_mdm */
  HSHM_ALWAYS_INLINE
  void Create(const std::string &state_name, const DomainId &domain_id) {
    id_ = TaskStateId::GetNull();
    id_ = LABSTOR_ADMIN->CreateTaskState<ConstructTask>(
        domain_id,
        state_name,
        "hermes_mdm",
        id_);
    queue_id_ = QueueId(id_);
    LABSTOR_ADMIN->CreateQueue(domain_id, queue_id_,
                               LABSTOR_CLIENT->server_config_.queue_manager_.max_lanes_,
                               LABSTOR_CLIENT->server_config_.queue_manager_.max_lanes_,
                               LABSTOR_CLIENT->server_config_.queue_manager_.queue_depth_,
                               bitfield32_t(0));
  }

  /** Destroy task state + queue */
  HSHM_ALWAYS_INLINE
  void Destroy(const std::string &state_name, const DomainId &domain_id) {
    LABSTOR_ADMIN->DestroyTaskState(domain_id, id_);
    LABSTOR_ADMIN->DestroyQueue(domain_id, queue_id_);
  }

  /**====================================
   * Tag Operations
   * ===================================*/

  /** Create a tag or get the ID of existing tag */
  HSHM_ALWAYS_INLINE
  void GetOrCreateTag(const hshm::charbuf &tag_name,
                      bool blob_owner,
                      const std::vector<TraitId> &traits,
                      size_t backend_size) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    u32 hash = std::hash<hshm::charbuf>{}(tag_name);
    auto *task = queue->Allocate<GetOrCreateTagTask>(
        LABSTOR_CLIENT->main_alloc_, p,
        id_, DomainId::GetNode(HASH_TO_NODE_ID(hash)),
        tag_name, blob_owner, traits, backend_size);
    queue->Emplace(hash, p);
    task->Wait();
    queue->Free(LABSTOR_CLIENT->main_alloc_, p);
  }

  /** Get tag ID */
  TagId GetTagId(const hshm::charbuf &tag_name) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    u32 hash = std::hash<hshm::charbuf>{}(tag_name);
    auto *task = queue->Allocate<GetTagIdTask>(
        LABSTOR_CLIENT->main_alloc_, p,
        id_, DomainId::GetNode(HASH_TO_NODE_ID(hash)),
        tag_name);
    queue->Emplace(hash, p);
    task->Wait();
    TagId tag_id = task->tag_id_;
    queue->Free(LABSTOR_CLIENT->main_alloc_, p);
    return tag_id;
  }

  /** Get tag name */
  hshm::string GetTagName(const TagId &tag_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    u32 hash = std::hash<TagId>{}(tag_id);
    auto *task = queue->Allocate<GetTagNameTask>(
        LABSTOR_CLIENT->main_alloc_, p,
        id_, DomainId::GetNode(HASH_TO_NODE_ID(hash)),
        tag_id);
    queue->Emplace(hash, p);
    task->Wait();
    hshm::string tag_name = hshm::to_charbuf<hipc::string>(*task->tag_name_.get());
    queue->Free(LABSTOR_CLIENT->main_alloc_, p);
    return tag_name;
  }

  /** Rename tag */
  void RenameTag(const TagId &tag_id, const hshm::charbuf &new_tag_name) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    u32 hash = std::hash<TagId>{}(tag_id);
    auto *task = queue->Allocate<RenameTagTask>(
        LABSTOR_CLIENT->main_alloc_, p,
        id_, DomainId::GetNode(HASH_TO_NODE_ID(hash)),
        tag_id, new_tag_name);
    queue->Emplace(hash, p);
    task->Wait();
    queue->Free(LABSTOR_CLIENT->main_alloc_, p);
  }

  /** Destroy tag */
  void DestroyTag(const TagId &tag_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    u32 hash = std::hash<TagId>{}(tag_id);
    auto *task = queue->Allocate<DestroyTagTask>(
        LABSTOR_CLIENT->main_alloc_, p,
        id_, DomainId::GetNode(HASH_TO_NODE_ID(hash)),
        tag_id);
    queue->Emplace(hash, p);
    task->Wait();
    queue->Free(LABSTOR_CLIENT->main_alloc_, p);
  }

  /** Add a blob to a tag */
  void TagAddBlob(const TagId &tag_id, const BlobId &blob_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    u32 hash = std::hash<TagId>{}(tag_id);
    auto *task = queue->Allocate<TagAddBlobTask>(
        LABSTOR_CLIENT->main_alloc_, p,
        id_, DomainId::GetNode(HASH_TO_NODE_ID(hash)),
        tag_id, blob_id);
    queue->Emplace(hash, p);
    task->Wait();
    queue->Free(LABSTOR_CLIENT->main_alloc_, p);
  }

  /** Remove a blob from a tag */
  void TagRemoveBlob(const TagId &tag_id, const BlobId &blob_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    u32 hash = std::hash<TagId>{}(tag_id);
    auto *task = queue->Allocate<TagRemoveBlobTask>(
        LABSTOR_CLIENT->main_alloc_, p,
        id_, DomainId::GetNode(HASH_TO_NODE_ID(hash)),
        tag_id, blob_id);
    queue->Emplace(hash, p);
    task->Wait();
    queue->Free(LABSTOR_CLIENT->main_alloc_, p);
  }

  /** Clear blobs from a tag */
  void TagClearBlobs(const TagId &tag_id) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    u32 hash = std::hash<TagId>{}(tag_id);
    auto *task = queue->Allocate<TagClearBlobsTask>(
        LABSTOR_CLIENT->main_alloc_, p,
        id_, DomainId::GetNode(HASH_TO_NODE_ID(hash)),
        tag_id);
    queue->Emplace(hash, p);
    task->Wait();
    queue->Free(LABSTOR_CLIENT->main_alloc_, p);
  }

  /**====================================
   * Blob Operations
   * ===================================*/

  /**
   * Create a blob's metadata
   *
   * @param bkt_id id of the bucket
   * @param blob_name semantic blob name
   * @param blob_size the amount of data placed
   * @param buffers the buffers corresponding to data
   * @param score the current score of the blob
   * @param did_create whether the blob was created or not
   * */
  BlobId PutBlob(
      TagId bkt_id, const std::string &blob_name, size_t blob_size,
      std::vector<BufferInfo> &buffers, float score, bool &did_create) {
  }
  
  /** Get a blob's data */
  void GetBlob(BlobId blob_id, char *buffer, size_t size) {
  }

  /**
   * Tag a blob
   *
   * @param blob_id id of the blob being tagged
   * @param tag_name tag name
   * */
  Status TagBlob(BlobId blob_id, TagId tag_id) {

  }

  /**
   * Check if blob has a tag
   * */
  bool BlobHasTag(BlobId blob_id, TagId tag_id) {
  }

  /**
   * Get \a blob_name BLOB from \a bkt_id bucket
   * */
  BlobId GetBlobId(TagId bkt_id, const std::string &blob_name) {
  }

  /**
   * Get \a blob_name BLOB name from \a blob_id BLOB id
   * */
  std::string GetBlobName(BlobId blob_id) {
  }

  /**
   * Get \a score from \a blob_id BLOB id
   * */
  float GetBlobScore(BlobId blob_id) {
  }

  /**
   * Get \a blob_id blob's buffers
   * */
  std::vector<BufferInfo> GetBlobBuffers(BlobId blob_id) {
  }

  /**
   * Rename \a blob_id blob to \a new_blob_name new blob name
   * in \a bkt_id bucket.
   * */
  bool RenameBlob(TagId bkt_id, BlobId blob_id,
                       const std::string &new_blob_name) {
  }

  /**
   * Truncate a blob to a new size
   * */
  void TruncateBlob(BlobId blob_id, size_t new_size) {
  }

  /**
   * Destroy \a blob_id blob in \a bkt_id bucket
   * */
  bool DestroyBlob(TagId bkt_id, BlobId blob_id) {
  }
};

}  // namespace labstor

#endif  // LABSTOR_hermes_mdm_H_
