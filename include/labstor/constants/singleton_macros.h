#ifndef LABSTOR_INCLUDE_LABSTOR_CONSTANTS_SINGLETON_MACROS_H_H
#define LABSTOR_INCLUDE_LABSTOR_CONSTANTS_SINGLETON_MACROS_H_H

#include <labstor/util/singleton.h>

#define LABSTOR_IPC_MANAGER scs::Singleton<labstor::IpcManager>::GetInstance()
#define LABSTOR_IPC_MANAGER_T labstor::IpcManager*

#define LABSTOR_CONFIGURATION_MANAGER scs::Singleton<labstor::ConfigurationManager>::GetInstance()
#define LABSTOR_CONFIGURATION_MANAGER_T labstor::ConfigurationManager*

#endif  // include_labstor_constants_singleton_macros_h
