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

#ifndef HERMES_SRC_CONFIG_SERVER_H_
#define HERMES_SRC_CONFIG_SERVER_H_

#include "config.h"

namespace labstor::config {

/**
 * Work orchestrator information defined in server config
 * */
struct WorkOrchestratorInfo {
  /** Maximum number of workers to spawn */
  size_t max_workers_;
  /** Maximum request size unit */
  size_t request_unit_;
  /** Maximum depth of IPC queues */
  size_t queue_depth_;
  /** Shared memory allocator */
  std::string shm_allocator_;
  /** Shared memory region name */
  std::string shmem_name_;
  /** Shared memory region size */
  size_t shmem_size_;
};

/**
 * RPC information defined in server config
 * */
struct RpcInfo {
  /** The name of a file that contains host names, 1 per line */
  std::string host_file_;
  /** The parsed hostnames from the hermes conf */
  std::vector<std::string> host_names_;
  /** The RPC protocol to be used. */
  std::string protocol_;
  /** The RPC domain name for verbs transport. */
  std::string domain_;
  /** The RPC port number. */
  int port_;
};

/**
 * System configuration for Hermes
 */
class ServerConfig : public BaseConfig {
 public:
  /** Work orchestrator info */
  WorkOrchestratorInfo wo_;
  /** The RPC information */
  RpcInfo rpc_;

 public:
  ServerConfig() = default;
  void LoadDefault();

 private:
  void ParseYAML(YAML::Node &yaml_conf);
  void ParseWorkOrchestrator(YAML::Node yaml_conf);
  void ParseRpcInfo(YAML::Node yaml_conf);
};

}  // namespace labstor::config

namespace labstor {
using ServerConfig = config::ServerConfig;
}  // namespace labstor

#endif  // HERMES_SRC_CONFIG_SERVER_H_
