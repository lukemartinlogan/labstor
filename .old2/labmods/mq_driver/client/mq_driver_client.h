//
// Created by lukemartinlogan on 12/5/21.
//

#ifndef LABSTOR_MQ_DRIVER_CLIENT_H
#define LABSTOR_MQ_DRIVER_CLIENT_H

#include <labstor/client/client.h>
#include <labmods/mq_driver/mq_driver.h>
#include <labstor/constants/macros.h>
#include <labstor/constants/constants.h>
#include <labstor/types/module.h>
#include <labstor/client/macros.h>
#include <labstor/client/ipc_manager.h>
#include <labstor/client/namespace.h>

namespace labstor::MQDriver {

class Client: public labstor::Module {
private:
    LABSTOR_IPC_MANAGER_T ipc_manager_;
    int dev_id_;
public:
    Client() : labstor::Module(MQ_DRIVER_MODULE_ID) {
        ipc_manager_ = LABSTOR_IPC_MANAGER;
    }
    void Initialize(int ns_id) override {}
    void Register(YAML::Node config) override;
    void IO(Ops op, void *user_buf, size_t buf_size, size_t sector, int hctx);
    inline void Read(void *user_buf, size_t buf_size, size_t sector, int hctx) {
        IO(Ops::kRead, user_buf, buf_size, sector, hctx);
    }
    inline void Write(void *user_buf, size_t buf_size, size_t sector, int hctx) {
        IO(Ops::kWrite, user_buf, buf_size, sector, hctx);
    }

    labstor::ipc::qtok_t AIO(Ops op, void *user_buf, size_t buf_size, size_t sector, int hctx);
    inline labstor::ipc::qtok_t ARead(void *user_buf, size_t buf_size, size_t sector, int hctx) {
        return AIO(Ops::kRead, user_buf, buf_size, sector, hctx);
    }
    inline labstor::ipc::qtok_t AWrite(void *user_buf, size_t buf_size, size_t sector, int hctx) {
        return AIO(Ops::kWrite, user_buf, buf_size, sector, hctx);
    }

    int GetNumHWQueues();
};

}

#endif //LABSTOR_MQ_DRIVER_CLIENT_H
