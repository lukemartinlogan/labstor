//
// Created by lukemartinlogan on 6/27/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_CLIENT_LABSTOR_SERVER_H_
#define LABSTOR_INCLUDE_LABSTOR_CLIENT_LABSTOR_SERVER_H_

#include "labstor/task_registry/task_registry.h"
#include "labstor/work_orchestrator/work_orchestrator.h"
#include "labstor/queue_manager/queue_manager_runtime.h"
#include "labstor_client.h"
#include "manager.h"
#include "labstor/network/rpc.h"
#include "labstor_admin/labstor_admin.h"

// Singleton macros
#define LABSTOR_RUNTIME hshm::Singleton<labstor::Runtime>::GetInstance()
#define LABSTOR_RUNTIME_T labstor::Runtime*

namespace labstor {

class Runtime : public ConfigurationManager {
 public:
  int data_;
  TaskRegistry task_registry_;
  WorkOrchestrator work_orchestrator_;
  QueueManagerRuntime queue_manager_;
  RpcContext rpc_;

 public:
  /** Default constructor */
  Runtime() = default;

  /** Create the server-side API */
  Runtime* Create(std::string server_config_path = "") {
    hshm::ScopedMutex lock(lock_, 1);
    if (is_initialized_) {
      return this;
    }
    mode_ = LabstorMode::kServer;
    is_being_initialized_ = true;
    InitServer(std::move(server_config_path));
    is_initialized_ = true;
    is_being_initialized_ = false;
    return this;
  }

 private:
  /** Initialize */
  void InitServer(std::string server_config_path) {
    LoadServerConfig(server_config_path);
    InitSharedMemory();
    task_registry_.ServerInit(&server_config_);
    rpc_.ServerInit(&server_config_);
    queue_manager_.ServerInit(main_alloc_, rpc_.node_id_, &server_config_, header_->queue_manager_);
    HERMES_THREAD_MODEL->SetThreadModel(hshm::ThreadType::kPthread);
    work_orchestrator_.ServerInit(&server_config_);
    LABSTOR_CLIENT->Create(server_config_path, "", true);

    // Create the admin library
    task_registry_.CreateTaskStateId(0);
    task_registry_.RegisterTaskLib("labstor_admin");
    task_registry_.CreateTaskState("labstor_admin",
                                   "labstor_admin",
                                   rpc_.node_id_,
                                   QueueManager::kAdminTaskState,
                                   nullptr);
    TaskState *admin = task_registry_.GetTaskState(
        QueueManager::kAdminTaskState);

    // Create the work orchestrator queue scheduling library
    TaskStateId queue_sched_id = task_registry_.CreateTaskStateId(0);
    task_registry_.RegisterTaskLib("worch_queue_round_robin");
    task_registry_.CreateTaskState("worch_queue_round_robin",
                                   "worch_queue_round_robin",
                                   rpc_.node_id_,
                                   queue_sched_id,
                                   nullptr);

    // Create the work orchestrator process scheduling library
    TaskStateId proc_sched_id = task_registry_.CreateTaskStateId(0);
    task_registry_.RegisterTaskLib("worch_proc_round_robin");
    task_registry_.CreateTaskState("worch_proc_round_robin",
                                   "worch_proc_round_robin",
                                   rpc_.node_id_,
                                   proc_sched_id,
                                   nullptr);

    // Begin spawning long-running tasks
    hipc::Pointer p;
    MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(QueueManager::kAdminQueue);

    // Set the work orchestrator queue scheduler
    auto *queue_sched_task = queue->Allocate<Admin::SetWorkOrchestratorProcessPolicyTask>(
        LABSTOR_CLIENT->main_alloc_, p, 0, queue_sched_id);
    admin->Run(queue, queue_sched_task->method_, queue_sched_task);
    queue->Free(LABSTOR_CLIENT->main_alloc_, p);

    // Set the work orchestrator process scheduler
    auto *proc_sched_task = queue->Allocate<Admin::SetWorkOrchestratorProcessPolicyTask>(
        LABSTOR_CLIENT->main_alloc_, p, 0, queue_sched_id);
    admin->Run(queue, proc_sched_task->method_, proc_sched_task);
    queue->Free(LABSTOR_CLIENT->main_alloc_, p);
  }

 public:
  /** Initialize shared-memory between daemon and client */
  void InitSharedMemory() {
    // Create shared-memory allocator
    auto mem_mngr = HERMES_MEMORY_MANAGER;
    if (server_config_.queue_manager_.shm_size_ == 0) {
      server_config_.queue_manager_.shm_size_ =
          hipc::MemoryManager::GetDefaultBackendSize();
    }
    mem_mngr->CreateBackend<hipc::PosixShmMmap>(
        server_config_.queue_manager_.shm_size_,
        server_config_.queue_manager_.shm_name_);
    main_alloc_ =
        mem_mngr->CreateAllocator<hipc::ScalablePageAllocator>(
            server_config_.queue_manager_.shm_name_,
            main_alloc_id_,
            sizeof(LabstorShm));
    header_ = main_alloc_->GetCustomHeader<LabstorShm>();
  }

  /** Finalize Hermes explicitly */
  void Finalize() {}

  /** Run the Hermes core Daemon */
  void RunDaemon() {
    while (LABSTOR_WORK_ORCHESTRATOR->IsRuntimeAlive()) {
      // Scheduler callbacks?
      HERMES_THREAD_MODEL->SleepForUs(1000);
    }
    LABSTOR_WORK_ORCHESTRATOR->Join();
  }

  /** Stop the Hermes core Daemon */
  void StopDaemon() {}
};

}  // namespace labstor

#endif  // LABSTOR_INCLUDE_LABSTOR_CLIENT_LABSTOR_SERVER_H_
