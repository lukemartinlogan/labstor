#include <labstor/constants/macros.h>
#include <labstor/util/singleton.h>
#include <labstor/constants/data_structure_singleton_macros.h>

#include <labstor/introspect/system_info.h>
template<> std::unique_ptr<labstor::SystemInfo> scs::Singleton<labstor::SystemInfo>::obj_ = nullptr;
#include <labstor/memory/memory_manager.h>
template<> std::unique_ptr<labstor::ipc::MemoryManager> scs::Singleton<labstor::ipc::MemoryManager>::obj_ = nullptr;
#include <labstor/thread/thread_manager.h>
template<> std::unique_ptr<labstor::ThreadManager> scs::Singleton<labstor::ThreadManager>::obj_ = nullptr;
