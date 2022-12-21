#include <labstor/constants/macros.h>
#include <labstor/util/singleton.h>
#include <labstor/thread/lock/mutex.h>
#include <labstor/constants/singleton_macros.h>

#include <labstor/ipc_manager/ipc_manager.h>
template<> std::unique_ptr<labstor::IpcManager> scs::Singleton<labstor::IpcManager>::obj_ = nullptr;
template<> labstor::Mutex scs::Singleton<labstor::IpcManager>::lock_ = labstor::Mutex();
#include <labstor/runtime/configuration_manager.h>
template<> std::unique_ptr<labstor::ConfigurationManager> scs::Singleton<labstor::ConfigurationManager>::obj_ = nullptr;
template<> labstor::Mutex scs::Singleton<labstor::ConfigurationManager>::lock_ = labstor::Mutex();
