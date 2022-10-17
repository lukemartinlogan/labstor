#include <labstor/constants/macros.h>
#include <labstor/util/singleton.h>
#include <labstor/constants/singleton_macros.h>

template<> std::unique_ptr<labstor::IpcManager> scs::Singleton<labstor::IpcManager>::obj_ = nullptr;
template<> std::unique_ptr<labstor::ConfigurationManager> scs::Singleton<labstor::ConfigurationManager>::obj_ = nullptr;
template<> std::unique_ptr<labstor::SystemInfo> scs::Singleton<labstor::SystemInfo>::obj_ = nullptr;
