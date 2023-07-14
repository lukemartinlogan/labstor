//
// Created by lukemartinlogan on 6/29/23.
//

#ifndef LABSTOR_hermes_dpe_H_
#define LABSTOR_hermes_dpe_H_

#include "labstor/api/labstor_client.h"
#include "labstor/task_registry/task_lib.h"
#include "labstor_admin/labstor_admin.h"
#include "labstor/queue_manager/queue_manager_client.h"
#include "hermes/hermes_types.h"

namespace hermes::dpe {

/** The set of methods in the admin task */
struct Method : public TaskMethod {
  TASK_METHOD_T kPut = TaskMethod::kLast;
};

/**
 * A task to create hermes_dpe
 * */
struct ConstructTask : public Task {
  HSHM_ALWAYS_INLINE
  ConstructTask(hipc::Allocator *alloc,
                const TaskStateId &state_id,
                const DomainId &domain_id) : Task(alloc) {
    // Initialize task
    key_ = 0;
    task_state_ = state_id;
    method_ = Method::kConstruct;
    task_flags_.SetBits(0);
    domain_id_ = domain_id;

    // Custom params
  }
};

/** A task to destroy hermes_dpe */
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

/**
 * A custom task in hermes_dpe
 * */
struct PutTask : public Task {
  IN BlobId blob_id_;
  IN hipc::ShmArchive<hipc::charbuf> blob_name_;
  IN hipc::Pointer data_;
  IN size_t data_size_;
  IN Context ctx_;

  HSHM_ALWAYS_INLINE
  PutTask(hipc::Allocator *alloc,
          const TaskStateId &state_id,
          const DomainId &domain_id,
          const BlobId &blob_id,
          const hshm::charbuf &blob_name,
          const hipc::Pointer &data,
          size_t data_size) : Task(alloc) {
    // Initialize task
    key_ = 0;
    task_state_ = state_id;
    method_ = Method::kPut;
    task_flags_.SetBits(0);
    domain_id_ = domain_id;

    // Custom params
    blob_id_ = blob_id;
    HSHM_MAKE_AR(blob_name_, LABSTOR_CLIENT->main_alloc_, blob_name);
    data_ = data;
    data_size_ = data_size;
  }

  HSHM_ALWAYS_INLINE
  ~PutTask() {
    HSHM_DESTROY_AR(blob_name_);
  }
};

/** Create admin requests */
class Client {
 public:
  TaskStateId id_;
  QueueId queue_id_;

 public:
  /** Default constructor */
  Client() = default;

  /** Destructor */
  ~Client() = default;

  /** Create a hermes_dpe */
  HSHM_ALWAYS_INLINE
  void Create(const std::string &state_name, const DomainId &domain_id) {
    id_ = TaskStateId::GetNull();
    id_ = LABSTOR_ADMIN->CreateTaskState<ConstructTask>(
        domain_id,
        state_name,
        "hermes_dpe",
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

  /** Create a queue with an ID */
  HSHM_ALWAYS_INLINE
      BlobId Put(const BlobId &blob_id,
                 const hshm::charbuf &blob_name,
                 const hipc::Pointer &data,
                 size_t data_size) {
    if (blob_id.IsNull()) {
      return Put(blob_name, data, data_size);
    } else {
      return Put(blob_id, data, data_size);
    }
  }

  /** Put data into an existing blob */
  HSHM_ALWAYS_INLINE
      BlobId Put(const BlobId &blob_id,
                 const hipc::Pointer &data,
                 size_t data_size) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    u32 hash = std::hash<u64>{}(blob_id.unique_);
    auto *task = queue->Allocate<PutTask>(
        LABSTOR_CLIENT->main_alloc_, p,
        id_,
        DomainId::GetNode(blob_id.node_id_),
        blob_id,
        hshm::charbuf(""),
        data,
        data_size);
    queue->Emplace(hash, p);
    task->Wait();
    queue->Free(LABSTOR_CLIENT->main_alloc_, p);
    return blob_id;
  }

  /** Put data into a new or existing blob */
  HSHM_ALWAYS_INLINE
      BlobId Put(const hshm::charbuf &blob_name,
                 const hipc::Pointer &data,
                 size_t data_size) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    u32 hash = std::hash<hshm::charbuf>{}(blob_name);
    auto *task = queue->Allocate<PutTask>(
        LABSTOR_CLIENT->main_alloc_, p,
        id_,
        DomainId::GetNode(HASH_TO_NODE_ID(hash)),
        BlobId::GetNull(),
        blob_name,
        data,
        data_size);
    queue->Emplace(hash, p);
    task->Wait();
    BlobId blob_id = task->blob_id_;
    queue->Free(LABSTOR_CLIENT->main_alloc_, p);
    return blob_id;
  }
};

}  // namespace labstor

#endif  // LABSTOR_hermes_dpe_H_
