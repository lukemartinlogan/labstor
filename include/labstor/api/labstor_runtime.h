//
// Created by lukemartinlogan on 6/27/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_CLIENT_LABSTOR_SERVER_H_
#define LABSTOR_INCLUDE_LABSTOR_CLIENT_LABSTOR_SERVER_H_

#include "labstor/task_registry/task_registry.h"
#include "manager.h"

// Singleton macros
#define LABSTOR_RUNTIME hshm::Singleton<labstor::Runtime>::GetInstance()
#define LABSTOR_RUNTIME_T labstor::Runtime*

namespace labstor {

class Runtime : public ConfigurationManager {
 public:
  int data_;
  TaskRegistry task_registry_;

 public:
  /** Default constructor */
  Runtime() = default;

  /** Create the server-side API */
  Runtime* Create(std::string server_config_path = "") {
    hshm::ScopedMutex lock(lock_, 1);
    if (is_initialized_) {
      return this;
    }
    mode_ = LabstorMode::kClient;
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

    // Initialize RPC
    // rpc_.InitRuntime();
    HERMES_THREAD_MODEL->SetThreadModel(hshm::ThreadType::kArgobots);

    // Initialize queue manager

    // Initialize work orchestrator
  }

 public:
  /** Initialize shared-memory between daemon and client */
  void InitSharedMemory() {
    // Create shared-memory allocator
    auto mem_mngr = HERMES_MEMORY_MANAGER;
    if (server_config_.wo_.shmem_size_ == 0) {
      server_config_.wo_.shmem_size_ =
        hipc::MemoryManager::GetDefaultBackendSize();
    }
    mem_mngr->CreateBackend<hipc::PosixShmMmap>(
      server_config_.wo_.shmem_size_,
      server_config_.wo_.shmem_name_);
    main_alloc_ =
      mem_mngr->CreateAllocator<hipc::ScalablePageAllocator>(
        server_config_.wo_.shmem_name_,
        main_alloc_id_,
        sizeof(LabstorShm));
    header_ = main_alloc_->GetCustomHeader<LabstorShm>();
  }

  /** Finalize Hermes explicitly */
  void Finalize() {}

  /** Run the Hermes core Daemon */
  void RunDaemon() {}

  /** Stop the Hermes core Daemon */
  void StopDaemon() {}
};

}  // namespace labstor

#endif //LABSTOR_INCLUDE_LABSTOR_CLIENT_LABSTOR_SERVER_H_
