//
// Created by lukemartinlogan on 11/26/21.
//

#include "dummy_server.h"

void labstor::test::Dummy::Server::ProcessRequest(labstor::ipc::queue_pair *qp, labstor::ipc::request *request, labstor::credentials *creds) {
    AUTO_TRACE("")
    //AUTO_TRACE(request->op_, request->req_id_)
    switch(static_cast<Ops>(request->op_)) {
        case Ops::kGetValue: {
            dummy_request *rq = reinterpret_cast<dummy_request*>(request);
            rq->Complete(5543);
            qp->Complete(rq);
            break;
        }
    }
}