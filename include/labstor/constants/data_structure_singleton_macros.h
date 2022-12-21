#ifndef LABSTOR_INCLUDE_LABSTOR_CONSTANTS_DATA_STRUCTURE_SINGLETON_MACROS_H_H
#define LABSTOR_INCLUDE_LABSTOR_CONSTANTS_DATA_STRUCTURE_SINGLETON_MACROS_H_H

#include <labstor/util/singleton.h>

#define LABSTOR_SYSTEM_INFO scs::Singleton<labstor::SystemInfo>::GetInstance()
#define LABSTOR_SYSTEM_INFO_T labstor::SystemInfo*

#define LABSTOR_MEMORY_MANAGER scs::Singleton<labstor::ipc::MemoryManager>::GetInstance()
#define LABSTOR_MEMORY_MANAGER_T labstor::ipc::MemoryManager*

#define LABSTOR_THREAD_MANAGER scs::Singleton<labstor::ThreadManager>::GetInstance()
#define LABSTOR_THREAD_MANAGER_T labstor::ThreadManager*

#endif  // include_labstor_constants_data_structure_singleton_macros_h
