//
// Created by lukemartinlogan on 12/5/21.
//

#ifndef LABSTOR_MQ_DRIVER_SERVER_H
#define LABSTOR_MQ_DRIVER_SERVER_H

#include <labstor/server/server.h>
#include <labstor/server/macros.h>
#include <labstor/server/ipc_manager.h>
#include <labstor/types/module.h>
#include <labmods/generic_queue/server/generic_queue_server.h>
#include "mq_driver.h"

namespace labstor::MQDriver {

class Server : public labstor::GenericQueue::Server {
private:
    LABSTOR_IPC_MANAGER_T ipc_manager_;
    int dev_id_;
public:
    Server() : labstor::GenericQueue::Server(MQ_DRIVER_MODULE_ID) {
        ipc_manager_ = LABSTOR_IPC_MANAGER;
    }
    bool ProcessRequest(labstor::queue_pair *qp, labstor::ipc::request *request, labstor::credentials *creds);
    bool Initialize(labstor::queue_pair *qp, labstor::ipc::request *request, labstor::credentials *creds) override;
    bool IO(labstor::queue_pair *qp, io_request *rq_submit, labstor::credentials *creds);
    bool GetStatistics(labstor::queue_pair *qp, labstor::GenericQueue::stats_request *rq_submit, labstor::credentials *creds);
};

}

#endif //LABSTOR_MQ_DRIVER_SERVER_H
