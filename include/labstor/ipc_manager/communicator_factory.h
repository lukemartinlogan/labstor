//
// Created by lukemartinlogan on 10/15/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_IPC_MANAGER_COMMUNICATOR_FACTORY_H_
#define LABSTOR_INCLUDE_LABSTOR_IPC_MANAGER_COMMUNICATOR_FACTORY_H_

#include <memory>
#include "communicator.h"
#include "unix_domain_socket.h"

namespace labstor {

class CommunicatorFactory {
 public:
  static std::unique_ptr<Communicator> Get(CommunicatorType type) {
    switch(type) {
      case CommunicatorType::kUnixDomain: return std::make_unique<UnixDomainSocket>();
      default: return nullptr;
    }
  }

};

}

#endif //LABSTOR_INCLUDE_LABSTOR_IPC_MANAGER_COMMUNICATOR_FACTORY_H_
