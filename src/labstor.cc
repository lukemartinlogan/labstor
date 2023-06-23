/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Distributed under BSD 3-Clause license.                                   *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Illinois Institute of Technology.                        *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of Hermes. The full Hermes copyright notice, including  *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the top directory. If you do not  *
 * have access to the file, you may request a copy from help@hdfgroup.org.   *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "labstor/labstor.h"

namespace labstor {

/**====================================
 * PRIVATE Init + Finalize Operations
 * ===================================*/

/** Internal initialization of Hermes */
void Labstor::Init(LabstorMode mode,
                   std::string server_config_path,
                   std::string client_config_path) {
  // Initialize hermes
  hshm::ScopedMutex lock(lock_, 1);
  if (is_initialized_) {
    return;
  }
  mode_ = mode;
  is_being_initialized_ = true;
  switch (mode_) {
    case LabstorMode::kServer: {
      InitServer(std::move(server_config_path));
      break;
    }
    case LabstorMode::kClient: {
      InitClient(std::move(server_config_path),
                 std::move(client_config_path));
      break;
    }
    case LabstorMode::kNone: {
      HELOG(kFatal, "Cannot have none LabstorMode")
    }
  }
  is_initialized_ = true;
  is_being_initialized_ = false;
}

/** Initialize Hermes as a server */
void Labstor::InitServer(std::string server_config_path) {
  LoadServerConfig(server_config_path);
  InitSharedMemory();

  // Initialize RPC
  // rpc_.InitServer();
  HERMES_THREAD_MODEL->SetThreadModel(hshm::ThreadType::kArgobots);

  // Initialize queue manager

  // Initialize work orchestrator
}

/** Initialize Hermes as a client to the daemon */
void Labstor::InitClient(std::string server_config_path,
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

/** Load the server-side configuration */
void Labstor::LoadServerConfig(std::string config_path) {
  if (config_path.empty()) {
    config_path = Constants::GetEnvSafe(Constants::kServerConfEnv);
  }
  if (mode_ == LabstorMode::kServer) {
    HILOG(kInfo, "Loading server configuration: {}", config_path);
  }
  server_config_.LoadFromFile(config_path);
}

/** Load the client-side configuration */
void Labstor::LoadClientConfig(std::string config_path) {
  if (config_path.empty()) {
    config_path = Constants::GetEnvSafe(Constants::kClientConfEnv);
  }
  HILOG(kDebug, "Loading client configuration: {}", config_path);
  client_config_.LoadFromFile(config_path);
}

/** Initialize shared-memory between daemon and client */
void Labstor::InitSharedMemory() {
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

/** Connect to a Daemon's shared memory */
void Labstor::LoadSharedMemory() {
  // Load shared-memory allocator
  auto mem_mngr = HERMES_MEMORY_MANAGER;
  mem_mngr->AttachBackend(hipc::MemoryBackendType::kPosixShmMmap,
                          server_config_.wo_.shmem_name_);
  main_alloc_ = mem_mngr->GetAllocator(main_alloc_id_);
  header_ = main_alloc_->GetCustomHeader<LabstorShm>();
}

/** Finalize Daemon mode */
void Labstor::FinalizeServer() {
}

/** Finalize client mode */
void Labstor::FinalizeClient() {
}

/**====================================
 * PUBLIC Finalize Operations
 * ===================================*/

/** Finalize Hermes explicitly */
void Labstor::Finalize() {
  if (!is_initialized_ || is_terminated_) {
    return;
  }
  switch (mode_) {
    case LabstorMode::kServer: {
      FinalizeServer();
      break;
    }
    case LabstorMode::kClient: {
      FinalizeClient();
      break;
    }
    default: {
      throw std::logic_error("Invalid LabstorMode to launch in");
    }
  }
  is_initialized_ = false;
  is_terminated_ = true;
}

/** Run the Hermes core Daemon */
void Labstor::RunDaemon() {
  // rpc_.RunDaemon();
}

/** Stop the Hermes core Daemon */
void Labstor::StopDaemon() {
  // rpc_.StopDaemon();
}



}  // namespace labstor
