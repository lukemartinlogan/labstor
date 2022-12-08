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

/** forward pointer for unordered_map */
template<typename Key, typename T, class Hash = std::hash<Key>>
class unordered_map;

/** forward pointer for unordered_map_bucket */
template<typename Key, typename T>
class unordered_map_bucket;

/**
 * The unordered_map shared-memory header
 * */
template<typename Key, typename T>
struct unordered_map_header {
  ShmArchive<vector<unordered_map_bucket<Key, T>>> buckets_;
  int max_collisions_;
  RealNumber growth_;
  std::atomic<size_t> length_;
  RwLock lock_;
};

/**
 * Represents a the combination of a Key and Value
 * */
template<typename Key, typename T>
struct unordered_map_pair {
 public:
  typedef SHM_T_OR_ARCHIVE(T) T_Ar;

 public:
  Key key_;
  T_Ar obj_;

 public:
  /** Constructor */
  template<typename ...Args>
  unordered_map_pair(const Key &key, Args ...args)
    : key_(key), obj_(args...) {
  }
};

/**
 * A bucket which contains a list of <Key, Obj> pairs
 * */
template<typename Key, typename T>
class unordered_map_bucket {
 public:
  typedef SHM_T_OR_ARCHIVE(T) T_Ar;
  typedef SHM_T_OR_REF_T(T) T_Ref;
  using COLLISION_T = unordered_map_pair<Key, T>;

 public:
  RwLock lock_;
  ShmArchive<list<unordered_map_pair<Key, T>>> collisions_;

  /** Constructs the collision list in shared memory */
  explicit unordered_map_bucket(Allocator *alloc) : collisions_(alloc) {}
};

/**
 * The unordered map iterator (bucket_iter, list_iter)
 * */
template<typename Key, typename T, class Hash>
struct unordered_map_iterator {
 public:
  typedef SHM_T_OR_ARCHIVE(T) T_Ar;
  typedef SHM_T_OR_REF_T(T) T_Ref;
  using BUCKET_T = unordered_map_bucket<Key, T>;
  using COLLISION_T = unordered_map_pair<Key, T>;

 public:
  unordered_map<Key, T, Hash> *map_;
  vector_iterator<BUCKET_T> bucket_;
  list_iterator<COLLISION_T> collision_;

  /** Default constructor */
  unordered_map_iterator() = default;

  /** Construct the end iterator  */
  explicit unordered_map_iterator(unordered_map<Key, T, Hash> *map,
                                  vector_iterator<BUCKET_T> &bucket)
    : map_(map), bucket_(bucket) {}

  /** Construct an iterator  */
  explicit unordered_map_iterator(unordered_map<Key, T, Hash> *map,
                                  vector_iterator<BUCKET_T> &bucket,
                                  list_iterator<COLLISION_T> &collision)
    : map_(map), bucket_(bucket), collision_(collision) {}

  /** Get the pointed object */
  COLLISION_T operator*() const {
    return (*collision_);
  }

  /** Go to the next object */
  unordered_map_iterator& operator++() {
    do {
      if (collision_ != (*bucket_).end()) {
        ++collision_;
      } else if (bucket_ != bucket_.end()) {
        ++bucket_;
        collision_ = (*bucket_).begin();
      } else {
        break;
      }
    } while (collision_ == (*bucket_).end());
    return *this;
  }

  /** Return the next iterator */
  unordered_map_iterator operator++(int) const {
    unordered_map_iterator next(*this);
    ++next;
    return next;
  }

  /** Assign one iterator into another */
  unordered_map_iterator<Key, T, Hash>&
  operator=(const unordered_map_iterator<Key, T, Hash> &other) {
    if (this != &other) {
      map_ = other.map_;
      bucket_ = other.bucket_;
      collision_ = other.collision_;
    }
    return *this;
  }

  /** Check if two iterators are equal */
  friend bool operator==(const unordered_map_iterator &a,
                         const unordered_map_iterator &b) {
    return (a.bucket_ == b.bucket_) && (a.collision_ == b.collision_);
  }

  /** Check if two iterators are inequal */
  friend bool operator!=(const unordered_map_iterator &a,
                         const unordered_map_iterator &b) {
    return (a.bucket_ != b.bucket_) || (a.collision_ != b.collision_);
  }
};

/**
 * MACROS to simplify the unordered_map namespace
 * */
#define CLASS_NAME unordered_map
#define TYPED_CLASS unordered_map<Key, T, Hash>
#define TYPED_HEADER unordered_map_header<Key, T>

/**
 * The unordered map implementation
 * */
template<typename Key, typename T, class Hash>
class unordered_map : public ShmDataStructure<TYPED_CLASS, TYPED_HEADER> {
 public:
  SHM_DATA_STRUCTURE_TEMPLATE(CLASS_NAME, TYPED_CLASS, TYPED_HEADER)
  friend unordered_map_iterator<Key, T, Hash>;

 public:
  typedef SHM_T_OR_ARCHIVE(T) T_Ar;
  typedef SHM_T_OR_REF_T(T) T_Ref;
  using BUCKET_T = unordered_map_bucket<Key, T>;
  using COLLISION_T = unordered_map_pair<Key, T>;

 public:
  /** Default constructor */
  unordered_map() = default;

  /** Default shared-memory constructor */
  explicit unordered_map(Allocator *alloc) :
    ShmDataStructure<TYPED_CLASS, TYPED_HEADER>(alloc) {
    shm_init();
  }

  /**
   * Initialize unordered map to a default number of buckets and collisions
   * per-bucket.
   * */
  void shm_init() {
    shm_init(20, 4, RealNumber(5,4));
  }

  /**
   * Initialize unordered map
   *
   * @param num_buckets the number of buckets to create
   * @param max_collisions the maximum number of collisions per-bucket before
   * a growth is triggered
   * @param growth the multiplier to grow the bucket vector size
   * */
  void shm_init(int num_buckets, int max_collisions, RealNumber growth) {
    header_ = alloc_->template
      AllocateObjs<TYPED_HEADER>(1, header_ptr_);
    vector<BUCKET_T> buckets(num_buckets, alloc_, alloc_);
    buckets >> header_->buckets_;
    header_->length_ = 0;
    header_->max_collisions_ = max_collisions;
  }

  /** Destroy the unordered_map buckets */
  void shm_destroy() {
    vector<BUCKET_T> buckets(header_->buckets_);
    for (auto &bkt : buckets) {
      list<COLLISION_T> collisions(bkt.collisions_);
      collisions.shm_destroy();
    }
    buckets.shm_destroy();
    alloc_->template
      Free(header_ptr_);
  }

  /**
   * Construct an object directly in the map
   *
   * @param key the key to future index the map
   * @param args the arguments to construct the object
   * @return None
   * */
  template<bool growth=true, typename ...Args>
  void emplace(const Key &key, Args&&... args) {
    if (header_ == nullptr) { shm_init(); }
    // Prepare unordered_map for read
    ScopedRwReadLock header_lock(header_->lock_);
    header_lock.Lock();

    // Hash the key to a bucket
    vector<BUCKET_T> buckets(header_->buckets_);
    size_t bkt_id = Hash{}(key) % buckets.size();
    BUCKET_T &bkt = buckets[bkt_id];

    // Prepare bucket for write
    ScopedRwWriteLock bkt_lock(bkt.lock_);
    bkt_lock.Lock();

    // Create the unordered_map pair
    list<COLLISION_T> collisions(bkt.collisions_);
    collisions.emplace_back(key, args...);

    // Get the number of buckets now to ensure repetitive growths don't happen
    size_t cur_num_buckets;
    if constexpr(growth) {
      cur_num_buckets = buckets.size();
    }

    // Release the bucket + unordered_map locks
    bkt_lock.Unlock();
    header_lock.Unlock();

    // Grow vector if necessary
    if constexpr(growth) {
      if (collisions.size() > header_->max_collisions_) {
        grow_map(cur_num_buckets);
      }
    }
  }

  /*
  void erase(unordered_map_iterator<T, T_Ref> first, unordered_map_iterator<T, T_Ref> last) {
  }
  */

  /**
   * Erase an object indexable by \a key key
   * */
  void erase(const Key &key) {
    if (header_ == nullptr) { shm_init(); }
    ScopedRwReadLock header_lock(header_->lock_);
    header_lock.Lock();
    vector<BUCKET_T> buckets(header_->buckets_);
    size_t bkt_id = Hash{}(key) % buckets.size();
    BUCKET_T &bkt = buckets[bkt_id];
    ScopedRwWriteLock bkt_lock(bkt.lock_);
    bkt_lock.Lock();
    list<COLLISION_T> collisions(bkt.collisions_);
    collisions.erase(key);
    bkt_lock.Unlock();
    header_lock.Unlock();
  }

  /**
   * Erase an object at the iterator
   * */
  void erase() {
    find();
  }

  /**
   * Locate an entry in the unordered_map
   *
   * @return the object pointed by key
   * @exception UNORDERED_MAP_CANT_FIND the key was not in the map
   * */
  T_Ref operator[](Key &key) {
    auto iter = find(key);
    if (iter != end()) {
      (*iter);
    }
    throw UNORDERED_MAP_CANT_FIND.format();
  }

  /**
   * Find an object in the unordered_map
   * */
  unordered_map_iterator<Key, T, Hash> find(const Key &key) {
    if (header_ == nullptr) { shm_init(); }
    ScopedRwReadLock header_lock(header_->lock_);
    header_lock.Lock();
    vector<BUCKET_T> buckets(header_->buckets_);
    size_t bkt_id = Hash{}(key) % buckets.size();
    auto bkt_iter = buckets.begin() + bkt_id;
    BUCKET_T &bkt = (*bkt_iter);
    ScopedRwReadLock bkt_lock(bkt.lock_);
    bkt_lock.Lock();
    list<COLLISION_T> collisions(bkt.collisions_);
    auto collision_iter = collisions.begin();
    for (; collision_iter != collisions.end(); ++collision_iter) {
      if ((*collision_iter).key_ == key) {
        break;
      }
    }
    return unordered_map_iterator<Key, T, Hash>(this,
                                                bkt_iter,
                                                collision_iter);
  }

  /** The number of entries in the map */
  size_t size() const {
    if (header_ == nullptr) {
      return 0;
    }
    return header_->length_;
  }

 public:

  /**
   * ITERATION
   * */

  /** The data required for safe iteration over this map */
  struct ScopedIterLock {
   private:
    unordered_map<Key, T, Hash> &map_;
    bool is_locked_;

   public:
    explicit ScopedIterLock(unordered_map<Key, T, Hash> &map)
      : map_(map), is_locked_(false) {}

    ~ScopedIterLock() { Unlock(); }

    /** Acquire all needed locks for iteration */
    void Lock() {
      if (!is_locked_) {
        map_.header_->lock_.ReadLock();
        vector<BUCKET_T> buckets(header_->buckets_);
        for (auto bkt : buckets) {
          bkt.lock_.ReadLock();
        }
        is_locked_ = true;
      }
    }

    /** Release all locks after iteration */
    void Unlock() {
      if (is_locked_) {
        map_.header_->lock_.ReadUnlock();
        vector<BUCKET_T> buckets(header_->buckets_);
        for (auto bkt : buckets) {
          bkt.lock_.ReadUnlock();
        }
        is_locked_ = false;
      }
    }
  };

  /** Read lock the unordered map for safe full iteration */
  inline ScopedIterLock iter_prep() {
    return ScopedIterLock(*this);
  }

  /** Forward iterator begin */
  inline unordered_map_iterator<Key, T, Hash> begin() {
    vector<BUCKET_T> buckets(header_->buckets_);
    auto bkt = buckets[0];
    list<COLLISION_T> collisions(bkt.collisions_);
    return unordered_map_iterator<Key, T, Hash>(this,
                                                buckets.begin(),
                                                collisions.begin());
  }

  /** Forward iterator end */
  inline unordered_map_iterator<Key, T, Hash> end() {
    vector<BUCKET_T> buckets(header_->buckets_);
    auto end = buckets.end();
    return unordered_map_iterator<Key, T, Hash>(this, end);
  }

 private:
  /** Grow a map from \a old_size to a new size */
  void grow_map(size_t old_size) {
    ScopedRwWriteLock scoped_lock(header_->lock_);
    scoped_lock.Lock();
    vector<BUCKET_T> buckets(header_->buckets_);
    size_t num_buckets = buckets.size();
    if (num_buckets != old_size) {
      return;
    }
    size_t new_num_buckets = get_new_size(num_buckets);
    buckets.resize(new_num_buckets, alloc_);
    for (size_t i = 0; i < num_buckets; ++i) {
      auto &bkt = buckets[i];
      list<COLLISION_T> collisions(bkt.collisions_);
      for (auto& entry : collisions) {
        emplace<false>(entry.key_, std::move(entry.obj_));
      }
    }
  }

  /** Calculate the new number of buckets based on growth rate */
  size_t get_new_size(size_t old_num_buckets) {
    RealNumber growth = header_->growth_;
    size_t new_num_buckets =
      old_num_buckets * growth.numerator_ / growth.denominator_;
    if (new_num_buckets - old_num_buckets < 10) {
      new_num_buckets = old_num_buckets + 10;
    }
    return new_num_buckets;
  }

 public:

};

}  // namespace labstor::ipc

#undef CLASS_NAME
#undef TYPED_CLASS
#undef TYPED_HEADER

#endif  // LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_UNORDERED_MAP_H_
