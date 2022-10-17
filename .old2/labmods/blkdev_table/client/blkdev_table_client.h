//
// Created by lukemartinlogan on 12/3/21.
//

#ifndef LABSTOR_BLKDEV_TABLE_CLIENT_H
#define LABSTOR_BLKDEV_TABLE_CLIENT_H

#include <labstor/client/client.h>
#include <labmods/blkdev_table/blkdev_table.h>
#include <labstor/constants/macros.h>
#include <labstor/constants/constants.h>
#include <labstor/types/module.h>
#include <labstor/client/macros.h>
#include <labstor/client/ipc_manager.h>
#include <labstor/client/namespace.h>

namespace labstor::BlkdevTable {

class Client: public labstor::Module {
private:
    LABSTOR_IPC_MANAGER_T ipc_manager_;
public:
    Client() : labstor::Module(BLKDEV_TABLE_MODULE_ID) {
        ipc_manager_ = LABSTOR_IPC_MANAGER;
    }
    void Initialize(int ns_id) override {}
    void Register(YAML::Node config) override;
    int RegisterBlkdev(std::string path);
};

}

#endif //LABSTOR_BLKDEV_TABLE_CLIENT_H
