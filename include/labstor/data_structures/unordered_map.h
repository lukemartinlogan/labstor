//
// Created by lukemartinlogan on 10/30/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_UNORDERED_MAP_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_UNORDERED_MAP_H_

#include <labstor/thread/thread_manager.h>
#include <labstor/thread/mutex.h>
#include <labstor/data_structures/lockless/_queue.h>

namespace labstor::ipc {

template<typename T>
struct unordered_map_bucket {
  lockless::_queue<T> entries_;
};

struct unordered_map_op {
  pid_t pid_;
  tid_t tid_;
  time_t timestamp_;
};

template<typename Key, typename T,
  class Hash = std::hash<Key>,
  class KeyEqual = std::equal_to<Key>>
class unordered_map {
  /*
 private:
  vector<unordered_map_bucket> buckets_;
  array_queue<unordered_map_op> reqs_;

 public:
  template<typename ...Args>
  void emplace(Key key, Args ...args) {
    auto bkt_id = Hash(key) % buckets_.size();
    auto bkt = buckets_[bkt_id];
    auto scoped_lock = ScopedRwLock(bkt.lock_);
    scoped_lock.WriteLock();
    bkt.entries_.emplace_back(args...);
  }

  T operator[](Key key) {
    auto bkt_id = Hash(key) % buckets_.size();
    auto bkt = buckets_[bkt_id];
    auto scoped_lock = ScopedRwLock(bkt.lock_);
  }*/
};

}

#endif  // LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_UNORDERED_MAP_H_
