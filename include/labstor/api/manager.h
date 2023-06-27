//
// Created by lukemartinlogan on 6/27/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_MANAGER_MANAGER_H_
#define LABSTOR_INCLUDE_LABSTOR_MANAGER_MANAGER_H_

#include "labstor/labstor_types.h"
#include "labstor/labstor_constants.h"
#include "labstor/config/config_client.h"
#include "labstor/config/config_server.h"

namespace labstor {

/** Shared-memory header for LabStor */
struct LabstorShm {
  i32 node_id_;
};

/** The configuration used inherited by runtime + client */
class ConfigurationManager {
 public:
  LabstorMode mode_;
  LabstorShm *header_;
  ClientConfig client_config_;
  ServerConfig server_config_;
  hipc::allocator_id_t main_alloc_id_;
  hipc::Allocator *main_alloc_;
  bool is_being_initialized_;
  bool is_initialized_;
  bool is_terminated_;
  bool is_transparent_;
  hshm::Mutex lock_;

  /** Default constructor */
  ConfigurationManager() : is_being_initialized_(false),
                           is_initialized_(false),
                           is_terminated_(false),
                           is_transparent_(false) {}

  /** Destructor */
  ~ConfigurationManager() {}

  /** Whether or not Labstor is currently being initialized */
  bool IsBeingInitialized() { return is_being_initialized_; }

  /** Whether or not Labstor is initialized */
  bool IsInitialized() { return is_initialized_; }

  /** Whether or not Labstor is finalized */
  bool IsTerminated() { return is_terminated_; }

  /** Load the server-side configuration */
  void LoadServerConfig(std::string config_path) {
    if (config_path.empty()) {
      config_path = Constants::GetEnvSafe(Constants::kServerConfEnv);
    }
    if (mode_ == LabstorMode::kServer) {
      HILOG(kInfo, "Loading server configuration: {}", config_path);
    }
    server_config_.LoadFromFile(config_path);
  }

  /** Load the client-side configuration */
  void LoadClientConfig(std::string config_path) {
    if (config_path.empty()) {
      config_path = Constants::GetEnvSafe(Constants::kClientConfEnv);
    }
    HILOG(kDebug, "Loading client configuration: {}", config_path);
    client_config_.LoadFromFile(config_path);
  }
};

}  // namespace labstor

#endif  // LABSTOR_INCLUDE_LABSTOR_MANAGER_MANAGER_H_
