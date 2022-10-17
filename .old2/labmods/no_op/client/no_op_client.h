//
// Created by lukemartinlogan on 12/5/21.
//

#ifndef LABSTOR_NO_OP_IOSCHED_CLIENT_H
#define LABSTOR_NO_OP_IOSCHED_CLIENT_H

#include "labstor/client/client.h"
#include "labmods/no_op/no_op.h"
#include "labstor/constants/macros.h"
#include "labstor/constants/constants.h"
#include "labstor/types/module.h"
#include "labstor/client/macros.h"
#include "labstor/client/ipc_manager.h"
#include "labstor/client/namespace.h"
#include <labmods/generic_block/client/generic_block_client.h>

namespace labstor::iosched::NoOp {

class Client: public labstor::GenericBlock::Client {
private:
    LABSTOR_IPC_MANAGER_T ipc_manager_;
public:
    Client() : labstor::GenericBlock::Client(NO_OP_IOSCHED_MODULE_ID) {
        ipc_manager_ = LABSTOR_IPC_MANAGER;
    }
    void Register(YAML::Node config) override;
    void Initialize(int ns_id) override {}
    labstor::ipc::qtok_t AIO(void *buf, size_t size, size_t off, labstor::GenericBlock::Ops op) override;
};

}

#endif //LABSTOR_NO_OP_IOSCHED_CLIENT_H
