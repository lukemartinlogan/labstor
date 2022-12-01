//
// Created by lukemartinlogan on 10/30/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_UNORDERED_MAP_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_UNORDERED_MAP_H_

#include <labstor/thread/thread_manager.h>
#include <labstor/thread/mutex.h>
#include <labstor/data_structures/lockless/vector.h>
#include <labstor/data_structures/lockless/list.h>
#include <labstor/data_structures/data_structure.h>

namespace labstor::ipc {
template<typename Key, typename T, class Hash>
class unordered_map;
}  // namespace labstor::ipc::lockless

namespace labstor::ipc {

template<typename Key, typename T, class Hash>
struct ShmArchive<unordered_map<Key, T, Hash>> {
  Pointer head_ptr_;
};

template<typename Key, typename T, class Hash>
struct ShmHeader<unordered_map<Key, T, Hash>> {
  ShmArchive<lockless::vector<T>> buckets_;
  size_t length_;
};

}  // namespace labstor::ipc

namespace labstor::ipc {

template<typename Key, typename T,
  class Hash = std::hash<Key>>
class unordered_map {
  SHM_DATA_STRUCTURE_TEMPLATE(unordered_map, Key, T, Hash)

 private:
  typedef SHM_T_OR_ARCHIVE(T) T_Ar;
  typedef SHM_T_OR_REF_T(T) T_Ref;

 public:
  unordered_map() = default;

  explicit unordered_map(Allocator *alloc) :
    ShmDataStructure<unordered_map<Key, T, Hash>>(alloc) {}

  void shm_destroy() {
  }

  void shm_serialize(ShmArchive<unordered_map<Key, T, Hash>> &ar) {
    ar.header_ptr_ = header_ptr_;
  }

  void shm_deserialize(ShmArchive<unordered_map<Key, T, Hash>> &ar) {
    InitDataStructure(ar.header_ptr_);
  }

  T_Ref operator[](const size_t i) {
  }

  template<typename ...Args>
  void emplace(Args&&... args) {
    lockless::vector<T> buckets(header_->buckets_);
    T obj(args...);
    size_t bkt_id = Hash(obj) % bucket;
  }

  void erase(unordered_map_iterator<T, T_Ref> first, unordered_map_iterator<T, T_Ref> last) {
  }

  void erase(Key &key) {
  }

  size_t size() const {
    if (header_ == nullptr) {
      return 0;
    }
    return header_->length_;
  }


};

}  // namespace labstor::ipc

#endif  // LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_UNORDERED_MAP_H_
