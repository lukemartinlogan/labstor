//
// Created by lukemartinlogan on 1/21/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_THREAD_SAFE_LIST_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_THREAD_SAFE_LIST_H_

#include "locked_container.h"
#include <labstor/data_structures/thread_unsafe/list.h>

namespace labstor::ipc::lock {

template<typename T>
using list = locked_container<list<T>>;

}  // namespace labstor::ipc::lock

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_THREAD_SAFE_LIST_H_
