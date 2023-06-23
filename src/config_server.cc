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

#include <string.h>
#include <yaml-cpp/yaml.h>
#include <ostream>
#include "hermes_shm/util/logging.h"
#include "hermes_shm/util/config_parse.h"
#include "labstor/config/config.h"
#include "labstor/config/config_server.h"
#include "labstor/config/config_server_default.h"

namespace labstor::config {

/** parse work orchestrator info from YAML config */
void ServerConfig::ParseWorkOrchestrator(YAML::Node yaml_conf) {
  if (yaml_conf["max_workers"]) {
    wo_.max_workers_ = yaml_conf["max_workers"].as<size_t>();
  }
  if (yaml_conf["request_unit"]) {
    wo_.request_unit_ = yaml_conf["request_unit"].as<size_t>();
  }
  if (yaml_conf["queue_depth"]) {
    wo_.queue_depth_ = yaml_conf["queue_depth"].as<size_t>();
  }
  if (yaml_conf["shm_allocator"]) {
    wo_.shm_allocator_ = yaml_conf["shm_allocator"].as<std::string>();
  }
  if (yaml_conf["shmem_name"]) {
    wo_.shmem_name_ = yaml_conf["shmem_name"].as<std::string>();
  }
  if (yaml_conf["shmem_size"]) {
    wo_.shmem_size_ = hshm::ConfigParse::ParseSize(
      yaml_conf["shmem_size"].as<std::string>());
  }
}

/** parse RPC information from YAML config */
void ServerConfig::ParseRpcInfo(YAML::Node yaml_conf) {
  std::string suffix;

  if (yaml_conf["host_file"]) {
    rpc_.host_file_ =
        hshm::ConfigParse::ExpandPath(yaml_conf["host_file"].as<std::string>());
    rpc_.host_names_.clear();
  }
  if (yaml_conf["host_names"] && rpc_.host_file_.size() == 0) {
    // NOTE(llogan): host file is prioritized
    rpc_.host_names_.clear();
    for (YAML::Node host_name_gen : yaml_conf["host_names"]) {
      std::string host_names = host_name_gen.as<std::string>();
      hshm::ConfigParse::ParseHostNameString(host_names, rpc_.host_names_);
    }
  }
  if (yaml_conf["domain"]) {
    rpc_.domain_ = yaml_conf["domain"].as<std::string>();
  }
  if (yaml_conf["protocol"]) {
    rpc_.protocol_ = yaml_conf["protocol"].as<std::string>();
  }
  if (yaml_conf["port"]) {
    rpc_.port_ = yaml_conf["port"].as<int>();
  }
}

/** parse the YAML node */
void ServerConfig::ParseYAML(YAML::Node &yaml_conf) {
  if (yaml_conf["work_orchestrator"]) {
    ParseWorkOrchestrator(yaml_conf["work_orchestrator"]);
  }
  if (yaml_conf["rpc"]) {
    ParseRpcInfo(yaml_conf["rpc"]);
  }
}

/** Load the default configuration */
void ServerConfig::LoadDefault() {
  LoadText(kServerDefaultConfigStr, false);
}

}  // namespace labstor::config
