//
// Created by lukemartinlogan on 9/16/21.
//

#ifndef LABSTOR_WORKER_KERNEL_H
#define LABSTOR_WORKER_KERNEL_H

#include <labstor/types/basics.h>

#define WORKER_MODULE_ID "WORKER_ID"

enum {
    SPAWN_WORKERS,
    SET_WORKER_AFFINITY,
    PAUSE_WORKER,
    RESUME_WORKER,
};

struct spawn_worker_request {
    int num_workers;
    int region_id;
    size_t region_size;
    size_t time_slice_us;
};

struct set_worker_affinity_request {
    int worker_id;
    int cpu_id;
};

struct pause_worker_request {
    int worker_id;
};

struct resume_worker_request {
    int worker_id;
};

struct kernel_worker_request {
    int op;
    union {
        struct spawn_worker_request spawn;
        struct set_worker_affinity_request affinity;
        struct pause_worker_request pause;
        struct resume_worker_request resume;
    };
};

struct kernel_worker_request_netlink {
    struct labstor_netlink_header header;
    struct kernel_worker_request rq;
};

#ifdef KERNEL_BUILD

struct worker_queue {
    int nqueues;
    struct request_queue *queues;
};

#endif

#ifdef __cplusplus

namespace labstor {

struct worker_queue {
    int nqueues;
    labstor::ipc::request_map *queues;
};

}

#endif

#endif //LABSTOR_WORKER_KERNEL_H
