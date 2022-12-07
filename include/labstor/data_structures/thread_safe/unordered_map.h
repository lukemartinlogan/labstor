//
// Created by lukemartinlogan on 10/30/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_UNORDERED_MAP_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_UNORDERED_MAP_H_

#include "labstor/thread/thread_manager.h"
#include "labstor/thread/mutex.h"
#include "labstor/data_structures/thread_unsafe/vector.h"
#include "labstor/data_structures/thread_unsafe/list.h"
#include "labstor/data_structures/data_structure.h"

namespace labstor::ipc {
template<typename Key, typename T, class Hash>
class unordered_map;

template<typename T>
class unordered_map_bucket;

template<typename T>
struct unordered_map_header {
  ShmArchive<vector<unordered_map_bucket<T>>> buckets_;
  int max_collide_;
  RealNumber growth_;
  std::atomic<size_t> length_;
  RwLock lock_;
};

template<typename T>
class unordered_map_bucket {
 private:
  typedef SHM_T_OR_ARCHIVE(T) T_Ar;
  typedef SHM_T_OR_REF_T(T) T_Ref;

 public:
  RwLock lock_;
  ShmArchive<list<T>> entries_;
};

#define CLASS_NAME unordered_map
#define TYPED_CLASS unordered_map<Key, T, Hash>
#define TYPED_HEADER unordered_map_header<T>

template<typename Key, typename T,
  class Hash = std::hash<Key>>
class unordered_map : public ShmDataStructure<TYPED_CLASS, TYPED_HEADER> {
 SHM_DATA_STRUCTURE_TEMPLATE(CLASS_NAME, TYPED_CLASS, TYPED_HEADER)

 private:
  typedef SHM_T_OR_ARCHIVE(T) T_Ar;
  typedef SHM_T_OR_REF_T(T) T_Ref;
  using bucket = unordered_map_bucket<T>;

 public:
  unordered_map() = default;

  explicit unordered_map(Allocator *alloc) :
    ShmDataStructure<TYPED_CLASS, TYPED_HEADER>(alloc) {}

  void shm_init() {
    shm_init(20, 4, RealNumber(5,4));
  }

  void shm_init(int num_buckets, int max_collide, RealNumber growth) {
    header_ = alloc_->template
      AllocateObjs<TYPED_HEADER>(1, header_ptr_);
    vector<bucket> buckets(num_buckets, alloc_);
    buckets >> header_->buckets_;
    header_->length_ = 0;
  }

  void shm_destroy() {
    vector<bucket> buckets(header_->buckets_);
    for (auto bucket : buckets) {
      bucket.lock_.WriteLock();
      bucket.collisions_.shm_destroy();
      bucket.lock_.WriteUnlock();
    }
  }

  T_Ref operator[](Key &key) {
    vector<bucket> buckets(header_->buckets_);
  }

  template<typename ...Args, bool growth>
  void emplace(Key &key, Args&&... args) {
    if (header_ == nullptr) { shm_init(); }
    ScopedRwReadLock header_lock(header_->lock_);
    header_lock.Lock();
    vector<bucket> buckets(header_->buckets_);
    size_t bkt_id = Hash(key) % buckets.size();
    bucket bkt = buckets[bkt_id];
    ScopedRwWriteLock bkt_lock(bkt.lock_);
    bkt_lock.Lock();
    list<T> collisions(bkt.collisions_);
    collisions.emplace_back(args...);
    buckets[bkt_id].lock_.WriteUnlock();
    header_lock.Unlock();
    if constexpr(growth) {
      if (collisions.size() > header_->max_collisions) {
      }
    }
  }

  /*
  void erase(unordered_map_iterator<T, T_Ref> first, unordered_map_iterator<T, T_Ref> last) {
  }
  */

  void erase(Key &key) {
  }

  size_t size() const {
    if (header_ == nullptr) {
      return 0;
    }
    return header_->length_;
  }

 private:
  void grow_map(size_t old_size) {
    ScopedRwWriteLock scoped_lock(header_->lock);
    vector<T> buckets(header_->buckets_);
    size_t num_buckets = buckets.size();
    if (num_buckets != old_size) {
      return;
    }
    size_t new_num_buckets = get_new_size();
    buckets.resize(new_num_buckets);
    for (size_t i = 0; i < num_buckets; ++i) {
      auto bucket = buckets[i];
      list<T> collisions(bucket);
      for (auto entry : collisions) {
        emplace(entry);
      }
    }
  }

  size_t get_new_size(size_t num_buckets) {
    RealNumber growth = header_->growth_;
    size_t new_num_buckets =
      num_buckets * growth.numerator_ / growth.denominator_;
    if (new_num_buckets - num_buckets < 10) {
      new_num_buckets = num_buckets + 10;
    }
    return new_num_buckets;
  }
};

}  // namespace labstor::ipc

#undef CLASS_NAME
#undef TYPED_CLASS
#undef TYPED_HEADER

#endif  // LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_UNORDERED_MAP_H_
