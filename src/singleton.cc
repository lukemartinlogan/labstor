#include <labstor/constants/macros.h>
#include <labstor/util/singleton.h>
#include <labstor/thread/lock/mutex.h>
#include <labstor/constants/singleton_macros.h>

#include <labstor/ipc_manager/ipc_manager.h>
template<> labstor::IpcManager scs::Singleton<labstor::IpcManager>::obj_ = labstor::IpcManager();
#include <labstor/runtime/configuration_manager.h>
template<> labstor::ConfigurationManager scs::Singleton<labstor::ConfigurationManager>::obj_ = labstor::ConfigurationManager();
