//
// Created by lukemartinlogan on 11/28/21.
//

#include <labstor/runtime/configuration_manager.h>
#include <labstor/runtime/macros.h>
#include "labstor/ipc_manager/ipc_manager.h"

namespace labstor {
    uint32_t thread_local_counter_ = 0;
    thread_local uint32_t thread_local_tid_;
    thread_local uint32_t thread_local_initialized_;
}

DEFINE_SINGLETON(CONFIGURATION_MANAGER)
DEFINE_SINGLETON(IPC_MANAGER)