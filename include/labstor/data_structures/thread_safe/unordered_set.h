//
// Created by lukemartinlogan on 1/3/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_THREAD_SAFE_UNORDERED_SET_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_THREAD_SAFE_UNORDERED_SET_H_

#include "unordered_map.h"

namespace labstor::ipc {

/**
 * MACROS to simplify the unordered_map namespace
 * Used as inputs to the SHM_CONTAINER_TEMPLATE
 * */

#define CLASS_NAME unordered_map
#define TYPED_CLASS unordered_map<Key, T, Hash>

/** An unordered_set implementation */
template<typename T, typename Hash>
class unordered_set : public SHM_CONTAINER(unord) {
 public:
};

}  // namespace labstor::ipc

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_THREAD_SAFE_UNORDERED_SET_H_
