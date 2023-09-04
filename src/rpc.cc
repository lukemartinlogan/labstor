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

#include "labstor/network/rpc.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <functional>
#include <iostream>
#include <fstream>

namespace labstor {

/**
 * initialize host info list
 * Requires the MetadataManager to be initialized.
 * */
void RpcContext::ServerInit(ServerConfig *config) {
  config_ = config;
  port_ = config_->rpc_.port_;
  protocol_ = config_->rpc_.protocol_;
  domain_ = config_->rpc_.domain_;
  num_threads_ = 4;  // TODO(llogan): Add to configuration
  if (hosts_.size()) { return; }
  // Uses hosts produced by host_names
  auto &hosts = config_->rpc_.host_names_;

  // Get all host info
  hosts_.reserve(hosts.size());
  u32 node_id = 1;
  for (const auto& name : hosts) {
    hosts_.emplace_back(name, _GetIpAddress(name), node_id++);
  }

  // Get id of current host
  node_id_ = _FindThisHost();
  if (node_id_ == 0 || node_id_ > (u32)hosts_.size()) {
    HELOG(kFatal, "Couldn't identify this host.")
  }
}

/** get RPC address */
std::string RpcContext::GetRpcAddress(const DomainId &domain_id, int port) {
  std::string result = config_->rpc_.protocol_ + "://";
  if (!config_->rpc_.domain_.empty()) {
    result += config_->rpc_.domain_ + "/";
  }
  std::string host_name = GetHostNameFromNodeId(domain_id);
  result += host_name + ":" + std::to_string(port);
  return result;
}

/** Get RPC address for this node */
std::string RpcContext::GetMyRpcAddress() {
  return GetRpcAddress(DomainId::GetNode(node_id_), port_);
}

/** get host name from node ID */
std::string RpcContext::GetHostNameFromNodeId(const DomainId &domain_id) {
  // NOTE(llogan): node_id 0 is reserved as the NULL node
  u32 node_id = domain_id.id_;
  if (node_id <= 0 || node_id > (i32)hosts_.size()) {
    HELOG(kFatal, "Attempted to get from node {}, which is out of "
          "the range 1-{}", node_id, hosts_.size() + 1)
  }
  u32 index = node_id - 1;
  return hosts_[index].hostname_;
}

/** get host name from node ID */
std::string RpcContext::GetIpAddressFromNodeId(const DomainId &domain_id) {
  // NOTE(llogan): node_id 0 is reserved as the NULL node
  u32 node_id = domain_id.id_;
  if (node_id <= 0 || node_id > (u32)hosts_.size()) {
    HELOG(kFatal, "Attempted to get from node {}, which is out of "
          "the range 1-{}", node_id, hosts_.size() + 1)
  }
  u32 index = node_id - 1;
  return hosts_[index].ip_addr_;
}

/** Get RPC protocol */
std::string RpcContext::GetProtocol() {
  return config_->rpc_.protocol_;
}

/** Get the node ID of this machine according to hostfile */
int RpcContext::_FindThisHost() {
  int node_id = 1;
  for (HostInfo &host : hosts_) {
    if (_IsAddressLocal(host.ip_addr_)) {
      return node_id;
    }
    ++node_id;
  }
  HELOG(kFatal, "Could not identify this host");
  return -1;
}

/** Check if an IP address is local to this machine */
bool RpcContext::_IsAddressLocal(const std::string &addr) {
  struct ifaddrs* ifAddrList = nullptr;
  bool found = false;

  if (getifaddrs(&ifAddrList) == -1) {
    perror("getifaddrs");
    return false;
  }

  for (struct ifaddrs* ifAddr = ifAddrList;
       ifAddr != nullptr; ifAddr = ifAddr->ifa_next) {
    if (ifAddr->ifa_addr == nullptr ||
        ifAddr->ifa_addr->sa_family != AF_INET) {
      continue;
    }

    struct sockaddr_in* sin =
        reinterpret_cast<struct sockaddr_in*>(ifAddr->ifa_addr);
    char ipAddress[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET, &(sin->sin_addr), ipAddress, INET_ADDRSTRLEN);

    if (addr == ipAddress) {
      found = true;
      break;
    }
  }

  freeifaddrs(ifAddrList);
  return found;
}

/** Get IPv4 address from the host with "host_name" */
std::string RpcContext::_GetIpAddress(const std::string &host_name) {
  struct hostent hostname_info = {};
  struct hostent *hostname_result;
  int hostname_error = 0;
  char hostname_buffer[4096] = {};
#ifdef __APPLE__
  hostname_result = gethostbyname(host_name.c_str());
  in_addr **addr_list = (struct in_addr **)hostname_result->h_addr_list;
#else
  int gethostbyname_result =
      gethostbyname_r(host_name.c_str(), &hostname_info, hostname_buffer,
                      4096, &hostname_result, &hostname_error);
  if (gethostbyname_result != 0) {
    HELOG(kFatal, hstrerror(h_errno))
  }
  in_addr **addr_list = (struct in_addr **)hostname_info.h_addr_list;
#endif
  if (!addr_list[0]) {
    HELOG(kFatal, hstrerror(h_errno))
  }

  char ip_address[INET_ADDRSTRLEN] = {0};
  const char *inet_result =
      inet_ntop(AF_INET, addr_list[0], ip_address, INET_ADDRSTRLEN);
  if (!inet_result) {
    perror("inet_ntop");
    HELOG(kFatal, "inet_ntop failed");
  }
  return ip_address;
}

}  // namespace hermes
