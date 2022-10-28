/*
 * Copyright (C) 2022  SCS Lab <scslab@iit.edu>,
 * Luke Logan <llogan@hawk.iit.edu>,
 * Jaime Cernuda Garcia <jcernudagarcia@hawk.iit.edu>
 * Jay Lofstead <gflofst@sandia.gov>,
 * Anthony Kougkas <akougkas@iit.edu>,
 * Xian-He Sun <sun@iit.edu>
 *
 * This file is part of LabStor
 *
 * LabStor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef LABSTOR_SOCKET_H
#define LABSTOR_SOCKET_H

#include "labstor/util/errors.h"
#include "communicator.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/sysinfo.h>
#include <fcntl.h>
#include <unistd.h>

namespace labstor {

class UnixDomainSocket : public Communicator {
 private:
  int fd_;

 public:
  UnixDomainSocket() : fd_(-1) {}
  ~UnixDomainSocket() {
    if (fd_ < 0) return;
    close(fd_);
  }

  void Connect(const std::string &url) override {
    struct sockaddr_un client_addr;
    struct sockaddr_un server_addr;

    // Create UDP socket
    memset(&client_addr, 0x0, sizeof(struct sockaddr_un));
    client_addr.sun_family = AF_UNIX;
    fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd_ == -1) {
      throw UNIX_SOCKET_FAILED.format(strerror(errno));
    }
    if (bind(fd_, (struct sockaddr *) &client_addr,
      sizeof(struct sockaddr_un)) == -1) {
      close(fd_);
      throw UNIX_BIND_FAILED.format(strerror(errno));
    }

    // Set server address & connect
    memset(&server_addr, 0x0, sizeof(struct sockaddr_un));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, url.c_str(), sizeof(server_addr.sun_path)-1);
    if (connect(fd_, (struct sockaddr *) &server_addr,
      sizeof(struct sockaddr_un)) == -1) {
      close(fd_);
      throw UNIX_CONNECT_FAILED.format(strerror(errno));
    }
  }

  bool IsConnected() override {
    return fd_ >= 0;
  }

  void Send(const char *buf, size_t size) override {
    size_t net = 0;
    while (net < size) {
      net += safe_send((char *) buf + net, size - net, 0);
    }
  }

  void Recv(char *buf, size_t size) override {
    size_t net = 0;
    while (net < size) {
      net += safe_recv((char *) buf + net, size - net, 0);
    }
  }

 private:
  inline int safe_recv(void *buf, size_t size, int flags) {
    int ret = recv(fd_, buf, size, flags);
    if (ret > 0) { return ret; }
    if (errno == EAGAIN || errno == EWOULDBLOCK) { return 0; }
    throw UNIX_RECVMSG_FAILED.format(strerror(errno));
  }

  inline int safe_send(void *buf, size_t size, int flags) {
    int ret = send(fd_, buf, size, flags);
    if (ret > 0) { return ret;  }
    if (errno == EAGAIN || errno == EWOULDBLOCK) { return 0; }
    throw UNIX_SENDMSG_FAILED.format(strerror(errno));
  }
};

}  // namespace labstor

#endif  // LABSTOR_SOCKET_H
