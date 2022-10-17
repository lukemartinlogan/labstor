//
// Created by lukemartinlogan on 9/7/21.
//

#include <memory>
#include "labstor/types/basic.h"
#include "labstor/ipc_manager/ipc_manager.h"
#include "labstor/util/debug.h"
#include "labstor/ipc_manager/communicator_factory.h"
#include "labstor/constants/constants.h"

void labstor::IpcManager::Connect() {
    AUTO_TRACE("")
    if(comm_ && comm_->IsConnected()) {
        return;
    }

    // Use OS-specific inter-process communicator
    comm_ = CommunicatorFactory::Get(CommunicatorType::kUnixDomain);
    comm_->Connect(kLabStorRuntimeUrl);

    //Receive SHMEM region
    //labstor::setup_reply reply;
    //comm_->Recv(reinterpret_cast<char*>(&reply), sizeof(reply));
}

void labstor::IpcManager::Register(std::unique_ptr<Communicator> &comm, UserCredentials creds) {
}
