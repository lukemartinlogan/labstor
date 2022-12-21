#include <labstor/constants/macros.h>
#include <labstor/util/singleton.h>
#include <labstor/thread/lock/mutex.h>
#include <labstor/constants/data_structure_singleton_macros.h>

#include <labstor/introspect/system_info.h>
template<> labstor::SystemInfo scs::Singleton<labstor::SystemInfo>::obj_ = labstor::SystemInfo();
#include <labstor/memory/memory_manager.h>
template<> labstor::ipc::MemoryManager scs::Singleton<labstor::ipc::MemoryManager>::obj_ = labstor::ipc::MemoryManager();
#include <labstor/thread/thread_manager.h>
template<> labstor::ThreadManager scs::Singleton<labstor::ThreadManager>::obj_ = labstor::ThreadManager();
