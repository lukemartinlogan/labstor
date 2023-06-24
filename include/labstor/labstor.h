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

#ifndef LABSTOR_SRC_API_LABSTOR_H_
#define LABSTOR_SRC_API_LABSTOR_H_

#include "config/config_client.h"
#include "config/config_server.h"
#include "labstor_data_structures.h"
#include "labstor_constants.h"

// Singleton macros
#define LABSTOR hshm::Singleton<labstor::Labstor>::GetInstance()
#define LABSTOR_T labstor::Labstor*

namespace lab = labstor;

namespace labstor {

/** Determine the mode that LabStor is initialized for */
enum class LabstorMode {
  kNone,
  kClient,
  kServer
};

/** Shared-memory header for LabStor */
struct LabstorShm {
  i32 node_id_;
};

/**
 * An index into all Hermes-related data structures.
 * */
class Labstor {
 public:
  LabstorMode mode_;
  LabstorShm *header_;
  config::ServerConfig server_config_;
  config::ClientConfig client_config_;
  hipc::allocator_id_t main_alloc_id_;
  hipc::Allocator *main_alloc_;
  bool is_being_initialized_;
  bool is_initialized_;
  bool is_terminated_;
  bool is_transparent_;
  hshm::Mutex lock_;

 public:
  /**====================================
   * PUBLIC Init Operations
   * ===================================*/

  /** Default constructor */
  Labstor() : is_being_initialized_(false),
              is_initialized_(false),
              is_terminated_(false),
              is_transparent_(false) {}

  /** Destructor */
  ~Labstor() {}

  /** Whether or not Labstor is currently being initialized */
  bool IsBeingInitialized() { return is_being_initialized_; }

  /** Whether or not Labstor is initialized */
  bool IsInitialized() { return is_initialized_; }

  /** Whether or not Labstor is finalized */
  bool IsTerminated() { return is_terminated_; }

  /** Initialize Labstor explicitly */
  static Labstor* Create(LabstorMode mode = LabstorMode::kClient,
                        std::string server_config_path = "",
                        std::string client_config_path = "") {
    auto labstor = LABSTOR;
    labstor->Init(mode, server_config_path, client_config_path);
    return labstor;
  }

 public:
  /**====================================
   * PUBLIC Finalize Operations
   * ===================================*/

  /** Finalize Hermes explicitly */
  void Finalize();

  /** Run the Hermes core Daemon */
  void RunDaemon();

  /** Stop the Hermes core Daemon */
  void StopDaemon();

 private:
  /**====================================
   * PRIVATE Init + Finalize Operations
   * ===================================*/

  /** Internal initialization of Hermes */
  void Init(LabstorMode mode = LabstorMode::kClient,
            std::string server_config_path = "",
            std::string client_config_path = "");

  /** Initialize Hermes as a server */
  void InitServer(std::string server_config_path);

  /** Initialize Hermes as a client to the daemon */
  void InitClient(std::string server_config_path,
                  std::string client_config_path);

  /** Load the server-side configuration */
  void LoadServerConfig(std::string config_path);

  /** Load the client-side configuration */
  void LoadClientConfig(std::string config_path);

  /** Initialize shared-memory between daemon and client */
  void InitSharedMemory();

  /** Connect to a Daemon's shared memory */
  void LoadSharedMemory();

  /** Finalize Daemon mode */
  void FinalizeServer();

  /** Finalize client mode */
  void FinalizeClient();
};

#define TRANSPARENT_LABSTOR\
  if (!LABSTOR->IsInitialized() && \
      !LABSTOR->IsBeingInitialized() && \
      !LABSTOR->IsTerminated()) {\
    LABSTOR->Create(hermes::LabstorMode::kClient);\
    LABSTOR->is_transparent_ = true;\
  }

}  // namespace labstor

#endif  // LABSTOR_SRC_API_LABSTOR_H_
