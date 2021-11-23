//
// Created by lukemartinlogan on 9/7/21.
//

#ifndef LABSTOR_SERVER_WORKER_H
#define LABSTOR_SERVER_WORKER_H

#ifdef __cplusplus

#include <thread>
#include <labstor/userspace_server/macros.h>
#include <labstor/userspace_server/namespace.h>
#include <labstor/types/daemon/daemon.h>
#include <labstor/types/data_structures/shmem_work_queue.h>

namespace labstor {

class Worker : DaemonWorker {
private:
    LABSTOR_NAMESPACE_T namespace_;
    labstor::ipc::work_queue work_queue_;
public:
    Worker() {
        namespace_ = LABSTOR_NAMESPACE;
    }
    void AssignQP(labstor::ipc::queue_pair qp) {
        work_queue_.Enqueue(qp);
    }
    void DoWork();
};

}

#endif

#ifndef __cplusplus
#endif

#endif //LABSTOR_SERVER_WORKER_H
