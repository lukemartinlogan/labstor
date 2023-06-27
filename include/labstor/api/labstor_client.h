//
// Created by lukemartinlogan on 6/23/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_CLIENT_LABSTOR_CLIENT_H_
#define LABSTOR_INCLUDE_LABSTOR_CLIENT_LABSTOR_CLIENT_H_

#include <string>
#include "manager.h"

// Singleton macros
#define LABSTOR_CLIENT hshm::Singleton<labstor::Client>::GetInstance()
#define LABSTOR_CLIENT_T labstor::Client*

namespace labstor {

class Client : public ConfigurationManager {
 public:
  int data_;

 public:
  /** Default constructor */
  Client() = default;

  /** Initialize the client */
  Client* Create(std::string server_config_path = "",
                 std::string client_config_path = "") {
    hshm::ScopedMutex lock(lock_, 1);
    if (is_initialized_) {
      return this;
    }
    mode_ = LabstorMode::kClient;
    is_being_initialized_ = true;
    InitClient(std::move(server_config_path),
               std::move(client_config_path));
    is_initialized_ = true;
    is_being_initialized_ = false;
    return this;
  }

 private:
  /** Initialize client */
  void InitClient(std::string server_config_path,
                  std::string client_config_path) {
    LoadServerConfig(server_config_path);
    LoadClientConfig(client_config_path);
    LoadSharedMemory();

    // Initialize references to SHM types
    // rpc_.InitClient();
    HERMES_THREAD_MODEL->SetThreadModel(hshm::ThreadType::kArgobots);

    // Initialize queue manager

    // Initialize work orchestrator
  }

 public:
  /** Connect to a Daemon's shared memory */
  void LoadSharedMemory() {
    // Load shared-memory allocator
    auto mem_mngr = HERMES_MEMORY_MANAGER;
    mem_mngr->AttachBackend(hipc::MemoryBackendType::kPosixShmMmap,
                            server_config_.wo_.shmem_name_);
    main_alloc_ = mem_mngr->GetAllocator(main_alloc_id_);
    header_ = main_alloc_->GetCustomHeader<LabstorShm>();
  }

  /** Finalize Hermes explicitly */
  void Finalize() {}
};

}  // namespace labstor

#define TRANSPARENT_LABSTOR_CLIENT\
  if (!LABSTOR->IsInitialized() && \
      !LABSTOR->IsBeingInitialized() && \
      !LABSTOR->IsTerminated()) {\
    LABSTOR_CLIENT->Create();\
    LABSTOR_CLIENT->is_transparent_ = true;\
  }

#endif  // LABSTOR_INCLUDE_LABSTOR_CLIENT_LABSTOR_CLIENT_H_
