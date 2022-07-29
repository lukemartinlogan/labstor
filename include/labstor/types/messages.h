//
// Created by lukemartinlogan on 11/18/21.
//

#ifndef LABSTOR_MESSAGES_H
#define LABSTOR_MESSAGES_H

namespace labstor::ipc {

enum {
    LABSTOR_ADMIN_REGISTER_QP
};

struct admin_request {
    int op_;
    admin_request() {}
    admin_request(int op) : op_(op) {}
};

struct admin_reply {
    int code_;
    admin_reply() {}
    admin_reply(int code) : code_(code) {}
};

struct setup_reply : public admin_reply {
    uint32_t region_id_;
    uint32_t region_size_;
    uint32_t request_region_size_;
    uint32_t request_unit_;
    uint32_t queue_region_size_;
    uint32_t queue_depth_;
    uint32_t num_queues_;
    uint32_t namespace_region_id_;
    uint32_t namespace_region_size_;
    uint32_t namespace_max_entries_;
};

}

#endif //LABSTOR_MESSAGES_H
