//
// Created by lukemartinlogan on 11/26/21.
//

#ifndef LABSTOR_REGISTRAR_SERVER_H
#define LABSTOR_REGISTRAR_SERVER_H

#include "registrar.h"
#include <labstor/server/server.h>
#include <labstor/types/module.h>
#include <labstor/server/macros.h>
#include <labstor/server/module_manager.h>
#include <labstor/server/ipc_manager.h>
#include <labstor/server/namespace.h>

namespace labstor::Registrar {

class Server : public labstor::Module {
private:
    LABSTOR_MODULE_MANAGER_T module_manager_;
    LABSTOR_IPC_MANAGER_T ipc_manager_;
    LABSTOR_NAMESPACE_T namespace_;
public:
    Server() : labstor::Module(LABSTOR_REGISTRAR_MODULE_ID) {
        module_manager_ = LABSTOR_MODULE_MANAGER;
        ipc_manager_ = LABSTOR_IPC_MANAGER;
        namespace_ = LABSTOR_NAMESPACE;
    }
    bool Initialize(labstor::queue_pair *qp, labstor::ipc::request *request, labstor::credentials *creds);
    bool ProcessRequest(labstor::queue_pair *qp, labstor::ipc::request *request, labstor::credentials *creds);
};

}

#endif //LABSTOR_REGISTRAR_SERVER_H
