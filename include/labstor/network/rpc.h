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

#ifndef LABSTOR_RPC_H_
#define LABSTOR_RPC_H_

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string>
#include <vector>

#include "labstor/labstor_types.h"
#include "labstor/config/config_server.h"

namespace labstor {

/** RPC types */
enum class RpcType {
  kThallium
};

/** Uniquely identify a host machine */
struct HostInfo {
  u32 node_id_;           /**< Hermes-assigned node id */
  std::string hostname_;  /**< Host name */
  std::string ip_addr_;   /**< Host IP address */

  HostInfo() = default;
  explicit HostInfo(const std::string &hostname,
                    const std::string &ip_addr,
                    u32 node_id)
      : hostname_(hostname), ip_addr_(ip_addr), node_id_(node_id) {}
};

/** Fabric protocols */


/** A structure to represent RPC context. */
class RpcContext {
 public:
  ServerConfig *config_;
  int port_;  /**< port number */
  std::string protocol_;  /**< Libfabric provider */
  std::string domain_;    /**< Libfabric domain */
  u32 node_id_;           /**< the ID of this node */
  int num_threads_;       /**< Number of RPC threads */
  std::vector<HostInfo> hosts_; /**< Hostname and ip addr per-node */

 public:
  /** Default constructor */
  RpcContext() = default;

  /** Parse a hostfile */
  static std::vector<std::string> ParseHostfile(const std::string &path);

  /** Get the nubmer of hosts */
  HSHM_ALWAYS_INLINE
  size_t GetNumHosts() {
    return hosts_.size();
  }

  /** initialize host info list */
  void ServerInit(ServerConfig *config);

  /** get RPC address */
  std::string GetRpcAddress(const DomainId &domain_id, int port);

  /** Get RPC address for this node */
  std::string GetMyRpcAddress();

  /** get host name from node ID */
  std::string GetHostNameFromNodeId(const DomainId &domain_id);

  /** get host name from node ID */
  std::string GetIpAddressFromNodeId(const DomainId &domain_id);

  /** Get RPC protocol */
  std::string GetProtocol();

 private:
  /** Get the node ID of this machine according to hostfile */
  int _FindThisHost();

  /** Check if an IP address is local */
  bool _IsAddressLocal(const std::string &addr);

  /** Get IPv4 address from the host with "host_name" */
  std::string _GetIpAddress(const std::string &host_name);
};

}  // namespace hermes

#endif  // LABSTOR_RPC_H_
