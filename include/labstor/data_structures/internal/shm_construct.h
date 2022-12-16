//
// Created by lukemartinlogan on 12/16/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_SHM_CONSTRUCT_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_SHM_CONSTRUCT_H_

#include "shm_archive.h"
#include "shm_pointer.h"

namespace labstor::ipc {

/** Constructs and archives an object */
template<typename T, typename ...Args>
static ShmArchive<T> make_shm_ar(Args&& ...args) {
  ShmArchive<T> ar;
  if constexpr(IS_SHM_SERIALIZEABLE(T)) {
    T obj;
    obj.shm_init(std::forward<Args>(args)...);
    if constexpr(!IS_SHM_SMART_POINTER(T)) {
      obj.UnsetDestructable();
    }
    obj >> ar;
  } else {
    ShmPointer<T> obj;
    obj.shm_init(std::forward<Args>(args)...);
    obj >> ar;
  }
  return ar;
}

}  // namespace labstor::ipc

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_SHM_CONSTRUCT_H_
