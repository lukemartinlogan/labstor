//
// Created by lukemartinlogan on 12/5/21.
//

#ifndef LABSTOR_SPDK_DRIVER_CLIENT_H
#define LABSTOR_SPDK_DRIVER_CLIENT_H

#include <labstor/client/client.h>
#include <labmods/spdk/spdk.h>
#include <labstor/constants/macros.h>
#include <labstor/constants/constants.h>
#include <labstor/types/module.h>
#include <labstor/client/macros.h>
#include <labstor/client/ipc_manager.h>
#include <labstor/client/namespace.h>

namespace labstor::SPDK {

class Client: public labstor::Module {
private:
    LABSTOR_IPC_MANAGER_T ipc_manager_;
    labstor::SPDK::Context context_;
    int spdk_queue_type_id_;
public:
    Client() : labstor::Module(SPDK_MODULE_ID) {
        ipc_manager_ = LABSTOR_IPC_MANAGER;
    }
    void Register(YAML::Node config) override;
    void Initialize(int ns_id) {}
    void Init(const std::string &traddr, int nvme_ns_id);
    void* Alloc(size_t size);
    void Free(void *mem);
    labstor::ipc::qtok_t AIO(Ops op, void *user_buf, size_t buf_size, size_t sector);
};

}

#endif //LABSTOR_SPDK_DRIVER_CLIENT_H
