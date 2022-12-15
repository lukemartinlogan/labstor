//
// Created by lukemartinlogan on 12/14/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_SERIALIZATION_THALLIUM_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_SERIALIZATION_THALLIUM_H_

#include <thallium.hpp>
#include <labstor/data_structures/string.h>
#include <labstor/data_structures/thread_unsafe/vector.h>
#include <labstor/data_structures/thread_unsafe/list.h>
#include <labstor/data_structures/thread_safe/unordered_map.h>

namespace thallium {

namespace lipc = labstor::ipc;

/**
 *  Lets Thallium know how to serialize an lipc::vector.
 *
 * This function is called implicitly by Thallium.
 *
 * @param ar An archive provided by Thallium.
 * @param vec The vector to serialize.
 */
template <typename A, typename T>
void save(A &ar, lipc::vector<T> &vec) {
  for (auto &entry : vec) {
    ar << entry;
  }
}

/**
 *  Lets Thallium know how to serialize an lipc::vector.
 *
 * This function is called implicitly by Thallium.
 *
 * @param ar An archive provided by Thallium.
 * @param target_id The vector to serialize.
 */
template <typename A, typename T>
void load(A &ar, lipc::vector<T> &vec) {
  for (auto &entry : vec) {
    ar &entry;
  }
}

/**
 *  Lets Thallium know how to serialize an lipc::string.
 *
 * This function is called implicitly by Thallium.
 *
 * @param ar An archive provided by Thallium.
 * @param text The string to serialize
 */
template <typename A>
void save(A &ar, lipc::string &text) {
  ar.write(text.GetAllocator()->GetId());
  ar.write(text.size());
  ar.write(text.data_mutable(), text.size());
}

/**
 *  Lets Thallium know how to deserialize an lipc::string.
 *
 * This function is called implicitly by Thallium.
 *
 * @param ar An archive provided by Thallium.
 * @param target_id The string to deserialize.
 */
template <typename A>
void load(A &ar, lipc::string &text) {
  lipc::allocator_id_t alloc_id;
  size_t size;
  ar.
  ar.read(size);
  text.shm_init(size);
  ar.read(text.data_mutable(), size);
}

}  // namespace thallium

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_SERIALIZATION_THALLIUM_H_
