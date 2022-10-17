//
// Created by lukemartinlogan on 10/16/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_RUNTIME_UNIX_RUNTIME_H_
#define LABSTOR_INCLUDE_LABSTOR_RUNTIME_UNIX_RUNTIME_H_

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <labstor/daemon/daemon.h>
#include <labstor/constants/constants.h>
#include "labstor/types/basic.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/sysinfo.h>

namespace labstor {

class UnixAcceptDaemon : public Daemon {
 private:
  int fd_;

 public:
  explicit UnixAcceptDaemon(ThreadType type) : Daemon(type), fd_(-1) {}

 protected:
  void _Init() override {
    int optval = 1;
    int ret;
    struct sockaddr_un server_addr_;

    remove(kLabStorRuntimeUrl.c_str());

    fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if(fd_ < 0) {
      throw labstor::UNIX_SOCKET_FAILED.format(strerror(errno));
    }

    ret = setsockopt(fd_, SOL_SOCKET, SO_PASSCRED, (void*)&optval, sizeof(optval));
    if(ret < 0) {
      throw labstor::UNIX_SETSOCKOPT_FAILED.format(strerror(errno));
    }

    memset(&server_addr_, 0, sizeof(server_addr_));
    server_addr_.sun_family = AF_UNIX;
    strncpy(server_addr_.sun_path,
            kLabStorRuntimeUrl.c_str(), kLabStorRuntimeUrl.size());
    ret = bind(fd_, (struct sockaddr *)&server_addr_, SUN_LEN(&server_addr_));
    if(ret < 0) {
      throw labstor::UNIX_BIND_FAILED.format(strerror(errno));
    }

    ret = listen(fd_, 1024);
    if(ret < 0) {
      throw labstor::UNIX_LISTEN_FAILED.format(strerror(errno));
    }
  }

  void _DoWork() override {
    int ret;
    struct ucred ucred;
    UserCredentials creds;
    struct sockaddr_un client_addr_;
    socklen_t clilen, len;
    int client_fd_;

    //Accept client connection
    clilen = sizeof(client_addr_);
    client_fd_ = accept(fd_, (struct sockaddr *) &client_addr_, &clilen);
    if (client_fd_ < 0) {
      throw labstor::UNIX_ACCEPT_FAILED.format(strerror(errno));
    }

    printf("New client was accepted!\n");
    //Get the client's credentials
    len = sizeof(struct ucred);
    ret = getsockopt(client_fd_, SOL_SOCKET, SO_PEERCRED, &ucred, &len);
    if (ret < 0) {
      throw labstor::UNIX_GETSOCKOPT_FAILED.format(strerror(errno));
    }
    memcpy(&creds, &ucred, sizeof(ucred));

    //Register Client with LabStor
    LABSTOR_ERROR_HANDLE_TRY {
      //ipc_manager_->RegisterClient(client_fd_, creds);
    }
    LABSTOR_ERROR_HANDLE_CATCH {
      printf("Failed to accept client (pid=%d uid=%d gid=%d)\n", creds.pid_, creds.uid_, creds.gid_);
      LABSTOR_ERROR_PTR->print();
    };
    printf("New client (pid=%d uid=%d gid=%d) was accepted!\n", creds.pid_, creds.uid_, creds.gid_);
  }
};

}

#endif //LABSTOR_INCLUDE_LABSTOR_RUNTIME_UNIX_RUNTIME_H_
