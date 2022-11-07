#ifndef LABSTOR_SINGLETON_MACROS_H
#define LABSTOR_SINGLETON_MACROS_H

#include <labstor/util/singleton.h>

#define LABSTOR_IPC_MANAGER scs::Singleton<labstor::IpcManager>::GetInstance()
#define LABSTOR_IPC_MANAGER_T labstor::IpcManager*

#define LABSTOR_CONFIGURATION_MANAGER scs::Singleton<labstor::ConfigurationManager>::GetInstance()
#define LABSTOR_CONFIGURATION_MANAGER_T labstor::ConfigurationManager*

#define LABSTOR_SYSTEM_INFO scs::Singleton<labstor::SystemInfo>::GetInstance()
#define LABSTOR_SYSTEM_INFO_T labstor::SystemInfo*

#define LABSTOR_MEMORY_MANAGER scs::Singleton<labstor::ipc::MemoryManager>::GetInstance()
#define LABSTOR_MEMORY_MANAGER_T labstor::ipc::MemoryManager*

#define LABSTOR_THREAD_MANAGER scs::Singleton<labstor::ThreadManager>::GetInstance()
#define LABSTOR_THREAD_MANAGER_T labstor::ThreadManager*

#endif  // LABSTOR_SINGLETON_MACROS_H
