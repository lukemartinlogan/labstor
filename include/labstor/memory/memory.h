//
// Created by lukemartinlogan on 11/1/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_MEMORY_MEMORY_H_
#define LABSTOR_INCLUDE_LABSTOR_MEMORY_MEMORY_H_

#include <labstor/types/basic.h>
#include <labstor/constants/singleton_macros.h>
#include <labstor/introspect/system_info.h>

#define IS_SHM_SERIALIZEABLE(T) \
  std::is_base_of<T, ShmSerializeable<T>>::value

/**
 * SHM_PARAM: Determines the type of the internal pointer used
 * to store data in a shared-memory data structure. For example,
 * let's say there are two vectors: vector<int> V1 and vector<vector<int>> V2.
 * V1 should store internally a pointer int *vec_
 * V2 should store internally a pointer ShmArchive<vector<int>> *vec_ and
 * then deserialize this pointer at every index operation.
 * */

#define SHM_PARAM(T) \
  typename std::conditional<         \
    IS_SHM_SERIALIZEABLE(T), \
    ShmArchive<T>, T>::type

/**
 * SHM_RET: Determines the return value of an index operation on
 * a shared-memory data structure. For example, let's say there
 * are two vectors: vector<int> V1 and vector<vector<int>> V2.
 * V1[0] should return an int&
 * V2[1] should return a vector<int> (not &)
 * This is because V2 needs to shm_deserialize vector<int> from shared
 * memory.
 *
 * @T: The type being stored in the shmem data structure
 * */

#define SHM_RET(T) \
  typename std::conditional<         \
    IS_SHM_SERIALIZEABLE(T), \
    T, T&>::type

namespace labstor {

template<typename T>
class ShmHeader;

template<typename T>
class ShmArchive;

template<typename T>
class ShmSerializeable {
 public:
  virtual void shm_serialize(ShmArchive<T> &ar) = 0;
  virtual void shm_deserialize(ShmArchive<T> &ar) = 0;
};

}  // namespace labstor

namespace labstor::memory {

struct allocator_id_t {
  uint32_t id_;
  allocator_id_t() : id_(-1) {}
  allocator_id_t(uint32_t id_major, uint32_t id_minor) {
    id_ = id_major ^ id_minor;
  }
  bool is_null() const { return id_ == -1; }

  bool operator==(const allocator_id_t &other) const {
    return other.id_ == id_;
  }
};

typedef uint32_t slot_id_t;

struct Pointer {
  allocator_id_t allocator_id_;
  slot_id_t slot_id_;
  size_t off_;
};

}  // namespace labstor::memory

namespace std {
template <>
struct hash<labstor::memory::allocator_id_t> {
  std::size_t operator()(const labstor::memory::allocator_id_t &key) const {
    return std::hash<uint32_t>{}(key.id_);
  }
};
}  // namespace std

#endif  // LABSTOR_INCLUDE_LABSTOR_MEMORY_MEMORY_H_
