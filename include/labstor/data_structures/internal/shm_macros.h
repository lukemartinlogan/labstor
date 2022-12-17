//
// Created by lukemartinlogan on 12/8/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_MEMORY_SHM_MACROS_H_
#define LABSTOR_INCLUDE_LABSTOR_MEMORY_SHM_MACROS_H_

#include <labstor/constants/macros.h>

/**
 * Determine whether or not \a T type is a SHM serializeable data structure
 * */

#define IS_SHM_SERIALIZEABLE(T) \
  std::is_base_of<labstor::ipc::ShmSerializeable, T>::value

/**
 * Determine whether or not \a T type is a SHM smart pointer
 * */

#define IS_SHM_SMART_POINTER(T) \
  std::is_base_of<labstor::ipc::ShmSmartPointer, T>::value

/**
 * SHM_X_OR_Y: X if T is SHM_SERIALIZEABLE, Y otherwise
 * */

#define SHM_X_OR_Y(T, X, Y) \
  typename std::conditional<         \
    IS_SHM_SERIALIZEABLE(T), \
    X, Y>::type

/**
 * SHM_T_OR_ARCHIVE: Determines the type of the internal pointer used
 * to store data in a shared-memory data structure. For example,
 * let's say there are two vectors: vector<int> V1 and vector<vector<int>> V2.
 * V1 should store internally a pointer int *vec_
 * V2 should store internally a pointer ShmArchive<vector<int>> *vec_ and
 * then deserialize this pointer at every index operation.
 * */

#define SHM_T_OR_ARCHIVE(T) \
  SHM_X_OR_Y(T, ShmArchive<T>, T)

/**
 * SHM_T_OR_REF_T: Determines the return value of an index operation on
 * a shared-memory data structure. For example, let's say there
 * are two vectors: vector<int> V1 and vector<vector<int>> V2.
 * V1[0] should return an int&
 * V2[1] should return a vector<int> (not &)
 * This is because V2 needs to shm_deserialize vector<int> from shared
 * memory.
 *
 * @T: The type being stored in the shmem data structure
 * */

#define SHM_T_OR_REF_T(T) \
  SHM_X_OR_Y(T, T, T&)
/**
 * SHM_T_OR_PTR_T: Used by shm_ref.
 *
 * @T: The type being stored in the shmem data structure
 * */

#define SHM_T_OR_PTR_T(T) \
  SHM_X_OR_Y(T, T, T*)

/**
 * SHM_T_OR_PTR_T: Used by unique_ptr and shared_ptr to determine how to
 * best store the underlying object.
 *
 * @T: The type being stored in the shmem data structure
 * */

#define SHM_T_OR_SHM_PTR_T(T) \
  SHM_X_OR_Y(T, T, ShmPointer<T>)

/**
 * SHM_T_OR_CONST_T: Determines whether or not an object should be
 * a constant or not.
 * */

#define SHM_CONST_T_OR_T(T, IS_CONST) \
  typename std::conditional<         \
    IS_CONST, \
    const T, T>::type

/**
 * Enables generic serialization of a class
 * */
#define SHM_GENERIC_SERIALIZE()

/**
 * Enables generic deserialization of a class
 * */
#define SHM_GENERIC_DESERIALIZE()


/**
 * Enables a specific ShmArchive type to be serialized
 * */
#define SHM_SERIALIZE_WRAPPER(AR_TYPE)\
  void shm_serialize(ShmArchive<TYPE_UNWRAP(AR_TYPE)> &ar) const {\
    shm_serialize(ar.header_ptr_);\
  }\
  void operator>>(ShmArchive<TYPE_UNWRAP(AR_TYPE)> &ar) const {\
    shm_serialize(ar.header_ptr_);\
  }

/**
 * Enables a specific ShmArchive type to be deserialized
 * */
#define SHM_DESERIALIZE_WRAPPER(AR_TYPE)\
  void shm_deserialize(const ShmArchive<TYPE_UNWRAP(AR_TYPE)> &ar) {\
    shm_deserialize(ar.header_ptr_);\
  }\
  void operator<<(const ShmArchive<TYPE_UNWRAP(AR_TYPE)> &ar) {\
    shm_deserialize(ar.header_ptr_);\
  }

/** Enables serialization + deserialization for data structures */
#define SHM_SERIALIZE_DESERIALIZE_WRAPPER(AR_TYPE)\
  SHM_SERIALIZE_WRAPPER(AR_TYPE)\
  SHM_DESERIALIZE_WRAPPER(AR_TYPE)

#endif //LABSTOR_INCLUDE_LABSTOR_MEMORY_SHM_MACROS_H_
