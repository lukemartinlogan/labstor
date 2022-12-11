//
// Created by lukemartinlogan on 12/8/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_MEMORY_SHM_MACROS_H_
#define LABSTOR_INCLUDE_LABSTOR_MEMORY_SHM_MACROS_H_

/**
 * Determine whether or not \a T type is a SHM serializeable data structure
 * */

#define IS_SHM_SERIALIZEABLE(T) \
  std::is_base_of<labstor::ipc::ShmSerializeable, T>::value

/**
 * SHM_T_OR_ARCHIVE: Determines the type of the internal pointer used
 * to store data in a shared-memory data structure. For example,
 * let's say there are two vectors: vector<int> V1 and vector<vector<int>> V2.
 * V1 should store internally a pointer int *vec_
 * V2 should store internally a pointer ShmArchive<vector<int>> *vec_ and
 * then deserialize this pointer at every index operation.
 * */

#define SHM_T_OR_ARCHIVE(T) \
  typename std::conditional<         \
    IS_SHM_SERIALIZEABLE(T), \
    ShmArchive<T>, T>::type

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
  typename std::conditional<         \
    IS_SHM_SERIALIZEABLE(T), \
    T, T&>::type

/**
 * SHM_T_PTR_OR_ARCHIVE: Whether or not to store a T* pointer or a
 * ShmArchive.
 *
 * Used for unique_ptr and shared_ptr.
 * */

#define SHM_T_PTR_OR_ARCHIVE(T) \
  typename std::conditional<         \
    IS_SHM_SERIALIZEABLE(T), \
    ShmArchive<T>, T*>::type

#endif //LABSTOR_INCLUDE_LABSTOR_MEMORY_SHM_MACROS_H_
