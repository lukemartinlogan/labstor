//
// Created by lukemartinlogan on 10/15/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_IPC_MANAGER_COMM_H_
#define LABSTOR_INCLUDE_LABSTOR_IPC_MANAGER_COMM_H_

#include <cstdlib>

namespace labstor {

enum class CommunicatorType {
  kUnixDomain
};

class Communicator {
 public:
  virtual ~Communicator() = default;
  virtual void Connect(const std::string &url) = 0;
  virtual bool IsConnected() = 0;
  virtual void Send(const char *buf, size_t size) = 0;
  virtual void Recv(char *buf, size_t size) = 0;
};

}

#endif //LABSTOR_INCLUDE_LABSTOR_IPC_MANAGER_COMM_H_
