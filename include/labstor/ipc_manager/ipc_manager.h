//
// Created by lukemartinlogan on 7/28/22.
//

#ifndef LABSTOR_IPC_MANAGER_H
#define LABSTOR_IPC_MANAGER_H

#include <labstor/types/basic.h>

#include <memory>
#include <unordered_map>
#include "communicator.h"

namespace labstor {

struct setup_reply {
};

struct ClientInfo {
  std::unique_ptr<Communicator> comm_;
  // std::unique_ptr<Memory> shmem_;
};

class IpcManager {
private:
  std::unique_ptr<Communicator> comm_;
  std::unordered_map<pid_t, ClientInfo> clients_;
public:
  void Connect();
  void Register(std::unique_ptr<Communicator> &comm, UserCredentials creds);
};

}

#endif //LABSTOR_IPC_MANAGER_H
