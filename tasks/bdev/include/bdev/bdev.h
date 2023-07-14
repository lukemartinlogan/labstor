//
// Created by lukemartinlogan on 6/29/23.
//

#ifndef LABSTOR_bdev_H_
#define LABSTOR_bdev_H_

#include "labstor/api/labstor_client.h"
#include "labstor/task_registry/task_lib.h"
#include "labstor_admin/labstor_admin.h"
#include "labstor/queue_manager/queue_manager_client.h"
#include "hermes/hermes_types.h"
#include "hermes/config_server.h"

namespace hermes::bdev {

/** The set of methods in the admin task */
struct Method : public TaskMethod {
  TASK_METHOD_T kWrite = TaskMethod::kLast;
  TASK_METHOD_T kRead = TaskMethod::kLast + 1;
  TASK_METHOD_T kAlloc = TaskMethod::kLast + 2;
  TASK_METHOD_T kFree = TaskMethod::kLast + 3;
  TASK_METHOD_T kBdevLast = TaskMethod::kLast + 4;
};

/**
 * A task to create hermes_mdm
 * */
using labstor::Admin::CreateTaskStateTask;
struct ConstructTask : public CreateTaskStateTask {
  IN DeviceInfo *info_;

  HSHM_ALWAYS_INLINE
  ConstructTask(hipc::Allocator *alloc,
                const DomainId &domain_id,
                const std::string &state_name,
                const std::string &lib_name,
                const TaskStateId &state_id,
                DeviceInfo &info)
      : CreateTaskStateTask(alloc, domain_id,
                            state_name,
                            lib_name,
                            state_id) {
    // Custom params
    info_ = &info;
  }
};

/** A task to destroy hermes_mdm */
using labstor::Admin::DestroyTaskStateTask;
struct DestructTask : public DestroyTaskStateTask {
  HSHM_ALWAYS_INLINE
  DestructTask(hipc::Allocator *alloc,
               TaskStateId &state_id,
               const DomainId &domain_id)
      : DestroyTaskStateTask(alloc, domain_id, state_id) {}
};

/**
 * A custom task in bdev
 * */
struct AllocTask : public LocalTask {
  IN size_t size_;  /**< Size in buf */
  OUT std::vector<BufferInfo> buffers_;
  OUT size_t alloc_size_;

  HSHM_ALWAYS_INLINE
  AllocTask(hipc::Allocator *alloc,
            const TaskStateId &state_id,
            const DomainId &domain_id,
            size_t size) : Task(alloc) {
    // Initialize task
    key_ = 0;
    task_state_ = state_id;
    method_ = Method::kAlloc;
    task_flags_.SetBits(0);
    domain_id_ = domain_id;

    // Free params
    size_ = size;
  }
};

/**
 * A custom task in bdev
 * */
struct FreeTask : public LocalTask {
  IN const std::vector<BufferInfo> *buffers_;

  HSHM_ALWAYS_INLINE
  FreeTask(hipc::Allocator *alloc,
           const TaskStateId &state_id,
           const DomainId &domain_id,
           const std::vector<BufferInfo> &buffers) : Task(alloc) {
    // Initialize task
    key_ = 0;
    task_state_ = state_id;
    method_ = Method::kFree;
    task_flags_.SetBits(0);
    domain_id_ = domain_id;

    // Free params
    buffers_ = &buffers;
  }
};

/**
 * A custom task in bdev
 * */
struct WriteTask : public LocalTask {
  IN const char *buf_;    /**< Data in memory */
  IN size_t disk_off_;   /**< Offset on disk */
  IN size_t size_;  /**< Size in buf */

  HSHM_ALWAYS_INLINE
  WriteTask(hipc::Allocator *alloc,
           const TaskStateId &state_id,
           const DomainId &domain_id,
           const char *buf,
           size_t disk_off,
           size_t size) : Task(alloc) {
    // Initialize task
    key_ = 0;
    task_state_ = state_id;
    method_ = Method::kWrite;
    task_flags_.SetBits(0);
    domain_id_ = domain_id;

    // Free params
    buf_ = buf;
    disk_off_ = disk_off;
    size_ = size;
  }
};

/**
 * A custom task in bdev
 * */
struct ReadTask : public LocalTask {
  IN char *buf_;    /**< Data in memory */
  IN size_t disk_off_;   /**< Offset on disk */
  IN size_t size_;  /**< Size in disk buf */

  HSHM_ALWAYS_INLINE
  ReadTask(hipc::Allocator *alloc,
            const TaskStateId &state_id,
            const DomainId &domain_id,
            char *buf,
            size_t disk_off,
            size_t size) : Task(alloc) {
    // Initialize task
    key_ = 0;
    task_state_ = state_id;
    method_ = Method::kRead;
    task_flags_.SetBits(0);
    domain_id_ = domain_id;

    // Free params
    buf_ = buf;
    disk_off_ = disk_off;
    size_ = size;
  }
};

/**
 * BDEV Client API
 * */
class Client {
 public:
  TaskStateId id_;
  QueueId queue_id_;

 public:
  /** Create a bdev */
  HSHM_ALWAYS_INLINE
  void Create(const DomainId &domain_id,
              const std::string &state_name,
              const std::string &lib_name,
              DeviceInfo &dev_info) {
    id_ = TaskStateId::GetNull();
    id_ = LABSTOR_ADMIN->CreateTaskState<ConstructTask>(
        domain_id,
        state_name,
        lib_name,
        id_,
        dev_info);
    queue_id_ = QueueId(id_);
    LABSTOR_ADMIN->CreateQueue(domain_id, queue_id_,
                               LABSTOR_CLIENT->server_config_.queue_manager_.max_lanes_,
                               LABSTOR_CLIENT->server_config_.queue_manager_.max_lanes_,
                               LABSTOR_CLIENT->server_config_.queue_manager_.queue_depth_,
                               bitfield32_t(0));
  }

  /** Destroy task state + queue */
  HSHM_ALWAYS_INLINE
  void Destroy(const DomainId &domain_id, const std::string &state_name) {
    LABSTOR_ADMIN->DestroyTaskState(domain_id, id_);
    LABSTOR_ADMIN->DestroyQueue(domain_id, queue_id_);
  }

  /** Allocate buffers from the bdev */
  HSHM_ALWAYS_INLINE
  void Allocate(const DomainId &domain_id,
                size_t size, std::vector<BufferInfo> &buffers,
                size_t &total_size) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    auto *task = queue->Allocate<AllocTask>(
        LABSTOR_CLIENT->main_alloc_, p,
        id_, domain_id, size);
    queue->Emplace(0, p);
    task->Wait();
  }

  /** Free data */
  HSHM_ALWAYS_INLINE
  void Free(const DomainId &domain_id,
            const std::vector<BufferInfo> &buffers) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    auto *task = queue->Allocate<FreeTask>(
        LABSTOR_CLIENT->main_alloc_, p,
        id_, domain_id, buffers);
    queue->Emplace(0, p);
    task->Wait();
  }

  /** Allocate buffers from the bdev */
  HSHM_ALWAYS_INLINE
  void Write(const DomainId &domain_id,
             const char *data, size_t off, size_t size) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    auto *task = queue->Allocate<WriteTask>(
        LABSTOR_CLIENT->main_alloc_, p,
        id_, domain_id, data, off, size);
    queue->Emplace(0, p);
    task->Wait();
  }

  /** Allocate buffers from the bdev */
  HSHM_ALWAYS_INLINE
  void Read(const DomainId &domain_id,
            char *data, size_t off, size_t size) {
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(queue_id_);
    auto *task = queue->Allocate<ReadTask>(
        LABSTOR_CLIENT->main_alloc_, p,
        id_, domain_id, data, off, size);
    queue->Emplace(0, p);
    task->Wait();
  }
};

}  // namespace hermes::bdev

#endif  // LABSTOR_bdev_H_
