//
// Created by lukemartinlogan on 11/28/21.
//

#include <labstor/client/client.h>
#include <labstor/client/macros.h>
#include <labstor/client/ipc_manager.h>
#include <labstor/client/namespace.h>
#include <labstor/client/module_manager.h>
#include <labmods/registrar/client/registrar_client.h>

namespace labstor {
    uint32_t thread_local_counter_ = 0;
    thread_local uint32_t thread_local_tid_;
    thread_local uint32_t thread_local_initialized_;
}

DEFINE_SINGLETON(IPC_MANAGER)
DEFINE_SINGLETON(MODULE_MANAGER)
DEFINE_SINGLETON(NAMESPACE)
DEFINE_SINGLETON(REGISTRAR)