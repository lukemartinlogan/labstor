//
// Created by lukemartinlogan on 11/26/21.
//

#ifndef LABSTOR_BLKDEV_TABLE_SERVER_H
#define LABSTOR_BLKDEV_TABLE_SERVER_H

#include <labstor/userspace/server/server.h>

#include "blkdev_table.h"
#include <labstor/userspace/types/module.h>
#include <labstor/userspace/server/macros.h>
#include <labstor/userspace/server/ipc_manager.h>
#include <labstor/userspace/server/namespace.h>

#include "labstor/types/data_structures/shmem_ring_buffer.h"

namespace labstor::BlkdevTable {

class Server : public labstor::Module {
private:
    LABSTOR_IPC_MANAGER_T ipc_manager_;
    labstor::ipc::mpmc::ring_buffer<uint32_t> dev_ids_;
public:
    Server() : labstor::Module(BLKDEV_TABLE_MODULE_ID) {
        ipc_manager_ = LABSTOR_IPC_MANAGER;
        uint32_t region_size = labstor::ipc::mpmc::ring_buffer<uint32_t>::GetSize(MAX_MOUNTED_BDEVS);
        void *region = malloc(region_size);
        dev_ids_.Init(region, region_size);
        for(int i = 0; i < MAX_MOUNTED_BDEVS; ++i) {
            dev_ids_.Enqueue(i);
        }
    }
    bool ProcessRequest(labstor::queue_pair *qp, labstor::ipc::request *request, labstor::credentials *creds) override;
    bool Initialize(labstor::queue_pair *qp, labstor::ipc::request *request, labstor::credentials *creds) override { return true; }
    bool RegisterBlkdev(labstor::queue_pair *qp, blkdev_table_register_request *rq, labstor::credentials *creds);
};

}

#endif //LABSTOR_BLKDEV_TABLE_SERVER_H
