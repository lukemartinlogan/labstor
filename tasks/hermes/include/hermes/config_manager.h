//
// Created by lukemartinlogan on 7/9/23.
//

#ifndef LABSTOR_TASKS_HERMES_INCLUDE_HERMES_CONFIG_MANAGER_H_
#define LABSTOR_TASKS_HERMES_INCLUDE_HERMES_CONFIG_MANAGER_H_

#include "hermes_types.h"
#include "labstor_admin/labstor_admin.h"
#include "hermes_mdm/hermes_mdm.h"
#include "hermes/config_client.h"
#include "hermes/config_server.h"

namespace hermes {

class ConfigurationManager {
 public:
  mdm::Client mdm_;
  ServerConfig server_config_;
  ClientConfig client_config_;

 public:
  ConfigurationManager() = default;

  void ClientInit() {
    // Create connection to MDM
    std::string config_path = "";
    LoadClientConfig(config_path);
    LoadServerConfig(config_path);
    mdm_.CreateRoot(DomainId::GetGlobal(), "hermes_mdm");
  }

  void ServerInit() {
    // Create connection to MDM
    mdm_.CreateRoot(DomainId::GetGlobal(), "hermes_mdm");
  }

  void LoadClientConfig(std::string &config_path) {
    // Load hermes config
    if (config_path.empty()) {
      config_path = GetEnvSafe(Constant::kHermesServerConf);
    }
    HILOG(kInfo, "Loading client configuration: {}", config_path)
    client_config_.LoadFromFile(config_path);
  }

  void LoadServerConfig(std::string &config_path) {
    // Load hermes config
    if (config_path.empty()) {
      config_path = GetEnvSafe(Constant::kHermesServerConf);
    }
    HILOG(kInfo, "Loading server configuration: {}", config_path)
    server_config_.LoadFromFile(config_path);
  }
};

}  // namespace hermes

#define HERMES hshm::Singleton<hermes::ConfigurationManager>::GetInstance()

#endif  // LABSTOR_TASKS_HERMES_INCLUDE_HERMES_CONFIG_MANAGER_H_
