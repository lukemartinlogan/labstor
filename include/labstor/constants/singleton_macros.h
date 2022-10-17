#ifndef LABSTOR_SINGLETON_MACROS_H
#define LABSTOR_SINGLETON_MACROS_H

#include <labstor/util/singleton.h>

#include <labstor/ipc_manager/ipc_manager.h>
#define LABSTOR_IPC_MANAGER scs::Singleton<labstor::IpcManager>::GetInstance()
#define LABSTOR_IPC_MANAGER_T labstor::IpcManager*

#include <labstor/runtime/configuration_manager.h>
#define LABSTOR_CONFIGURATION_MANAGER scs::Singleton<labstor::ConfigurationManager>::GetInstance()
#define LABSTOR_CONFIGURATION_MANAGER_T labstor::ConfigurationManager*

#endif  // LABSTOR_SINGLETON_MACROS_H
