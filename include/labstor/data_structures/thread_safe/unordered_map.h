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
 public:
  using BUCKET_T = unordered_map_bucket<Key, T>;
 public:
  ShmArchive<vector<BUCKET_T>> buckets_;
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
  typedef SHM_T_OR_ARCHIVE(Key) Key_Ar;

 public:
  Key_Ar key_;
  T_Ar val_;

 public:
  /** Constructor */
  template<typename ...Args>
  unordered_map_pair(const Key &key, Args&& ...args)
  : key_(std::move(key)), val_(std::forward<Args>(args)...) {
  }

  /** Move constructor */
  unordered_map_pair(unordered_map_pair&& other)
  : key_(std::move(other.key_)), val_(std::move(other.val_)) {}

  /** Destructor */
  ~unordered_map_pair() {
    if constexpr(IS_SHM_SERIALIZEABLE(Key)) {
      Key key(key_);
      key.shm_destroy();
    }
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      T val(val_);
      val.shm_destroy();
    }
  }
};

/**
 * The return value of a "Get" operation
 * */
template<typename Key, typename T>
struct unordered_map_pair_ret {
 public:
  typedef SHM_T_OR_REF_T(T) T_Ref;
  typedef SHM_T_OR_REF_T(Key) Key_Ref;
  using COLLISION_T = unordered_map_pair<Key, T>;

 public:
  Key_Ref key_;
  T_Ref val_;

 public:
  /** Constructor */
  explicit unordered_map_pair_ret(COLLISION_T &pair)
    : key_(pair.key_), val_(pair.val_) {
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
  ShmArchive<list<COLLISION_T>> collisions_;

  /** Constructs the collision list in shared memory */
  explicit unordered_map_bucket(Allocator *alloc) : collisions_(alloc) {}

  ~unordered_map_bucket() {
    list<COLLISION_T> collisions(collisions_);
    collisions.shm_destroy();
  }
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
  using COLLISION_RET_T = unordered_map_pair_ret<Key, T>;

 public:
  unordered_map<Key, T, Hash> *map_;
  vector<BUCKET_T> buckets_;
  list<COLLISION_T> collisions_;
  vector_iterator<BUCKET_T> bucket_;
  list_iterator<COLLISION_T> collision_;

  /** Default constructor */
  unordered_map_iterator() = default;

  /** Construct the iterator  */
  explicit unordered_map_iterator(unordered_map<Key, T, Hash> *map)
    : map_(map) {
  }

  /** Copy an iterator  */
  unordered_map_iterator(const unordered_map_iterator &other) {
    map_ = other.map_;
    bucket_ = other.bucket_;
    collision_ = other.collision_;
    bucket_.change_pointer(&buckets_);
    collision_.change_pointer(&collisions_);
  }

  /** Assign one iterator into another */
  unordered_map_iterator<Key, T, Hash>&
  operator=(const unordered_map_iterator<Key, T, Hash> &other) {
    if (this != &other) {
      map_ = other.map_;
      buckets_ = other.buckets_;
      collisions_ = other.collisions_;
      bucket_ = other.bucket_;
      collision_ = other.collision_;
      bucket_.change_pointer(&buckets_);
      collision_.change_pointer(&collisions_);
    }
    return *this;
  }

  /** Get the pointed object */
  COLLISION_RET_T operator*() const {
    return COLLISION_RET_T(*collision_);
  }

  /** Go to the next object */
  unordered_map_iterator& operator++() {
    ++collision_;
    make_correct();
    return *this;
  }

  /** Return the next iterator */
  unordered_map_iterator operator++(int) const {
    unordered_map_iterator next(*this);
    ++next;
    return next;
  }

  /**
   * Shifts bucket and collision iterator until there is a valid element.
   * Returns true if such an element is found, and false otherwise.
   * */
  bool make_correct() {
    do {
      if (bucket_ == buckets_.end()) {
        return false;
      }
      auto &bkt = (*bucket_);
      collisions_ << bkt.collisions_;
      if (collision_ != collisions_.end()) {
        return true;
      } else {
        ++bucket_;
        collisions_ << (*bucket_).collisions_;
        collision_ = collisions_.begin();
      }
    } while (true);
  }

  /** Check if two iterators are equal */
  friend bool operator==(const unordered_map_iterator &a,
                         const unordered_map_iterator &b) {
    if (a.is_end() && b.is_end()) {
      return true;
    }
    return (a.bucket_ == b.bucket_) && (a.collision_ == b.collision_);
  }

  /** Check if two iterators are inequal */
  friend bool operator!=(const unordered_map_iterator &a,
                         const unordered_map_iterator &b) {
    if (a.is_end() && b.is_end()) {
      return false;
    }
    return (a.bucket_ != b.bucket_) || (a.collision_ != b.collision_);
  }

  /** Determine whether this iterator is the end iterator */
  bool is_end() const {
    return bucket_.is_end();
  }

  /** Set this iterator to the end iterator */
  void set_end() {
    bucket_.set_end();
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
  typedef SHM_T_OR_ARCHIVE(Key) Key_Ar;
  typedef SHM_T_OR_REF_T(Key) Key_Ref;
  using BUCKET_T = unordered_map_bucket<Key, T>;
  using COLLISION_T = unordered_map_pair<Key, T>;
  using COLLISION_RET_T = unordered_map_pair_ret<Key, T>;

 public:
  /** Default constructor */
  unordered_map() = default;

  /** Construct the unordered_map in shared-memory */
  template<typename ...Args>
  explicit unordered_map(Allocator *alloc, Args&& ...args) :
    ShmDataStructure<TYPED_CLASS, TYPED_HEADER>(alloc) {
    shm_init(std::forward<Args>(args)...);
  }

  /** Moves one unordered_map into another */
  unordered_map(unordered_map&& source) {
    header_ptr_ = source.header_ptr_;
    header_ = source.header_;
    source.header_ptr_.set_null();
  }

  /** Disable copying  */
  unordered_map(const unordered_map &other) = delete;

  /** Assign one map into another */
  unordered_map&
  operator=(const unordered_map &other) {
    if (this != &other) {
      header_ptr_ = other.header_ptr_;
      header_ = other.header_;
    }
    return *this;
  }

  /**
   * Initialize unordered map
   *
   * @param num_buckets the number of buckets to create
   * @param max_collisions the maximum number of collisions per-bucket before
   * a growth is triggered
   * @param growth the multiplier to grow the bucket vector size
   * */
  void shm_init(int num_buckets = 20,
                int max_collisions = 4,
                RealNumber growth = RealNumber(5,4)) {
    header_ = alloc_->template
      AllocateObjs<TYPED_HEADER>(1, header_ptr_);
    vector<BUCKET_T> buckets(num_buckets, alloc_, alloc_);
    buckets >>  header_->buckets_;
    header_->length_ = 0;
    header_->max_collisions_ = max_collisions;
    header_->growth_ = growth;
  }

  /** Destroy the unordered_map buckets */
  void shm_destroy() {
    if (header_ptr_.is_null()) { return; }
    vector<BUCKET_T> buckets(header_->buckets_);
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
    COLLISION_T entry_shm(key, std::forward<Args>(args)...);
    insert(entry_shm);
  }

  /**
   * Emplace a serialized (key, value) pair in the map
   *
   * @param entry_shm the (key,value) pair shared-memory serialized
   * @return None
   * */
  template<bool growth=true, typename ...Args>
  void insert(COLLISION_T &&entry_shm) {
    insert(entry_shm);
  }

  /**
   * Emplace a serialized (key, value) pair in the map
   *
   * @param entry_shm the (key,value) pair shared-memory serialized
   * @return None
   * */
  template<bool growth=true, typename ...Args>
  void insert(COLLISION_T &entry_shm) {
    if (header_ == nullptr) { shm_init(); }
    COLLISION_RET_T entry(entry_shm);
    auto &key = entry.key_;
    auto &val = entry.val_;

    // Acquire the header lock for a read (not modifying bucket vec)
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
    collisions.emplace_back(std::move(entry_shm));

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

    // Increment the size of the map
    ++header_->length_;
  }

  /**
   * Erase an object indexable by \a key key
   * */
  void erase(const Key &key) {
    if (header_ == nullptr) { shm_init(); }
    // Acquire the header lock for a read (not modifying bucket vec)
    ScopedRwReadLock header_lock(header_->lock_);
    header_lock.Lock();

    // Get the bucket the key belongs to
    vector<BUCKET_T> buckets(header_->buckets_);
    size_t bkt_id = Hash{}(key) % buckets.size();
    BUCKET_T &bkt = buckets[bkt_id];
    ScopedRwWriteLock bkt_lock(bkt.lock_);
    bkt_lock.Lock();

    // Find and remove key from collision list
    list<COLLISION_T> collisions(bkt.collisions_);
    auto iter = find_collision(key, collisions);
    if (iter.is_end()) {
      return;
    }
    collisions.erase(iter);

    // Decrement the size of the map
    --header_->length_;
  }

  /**
   * Erase an object at the iterator
   * */
  void erase(unordered_map_iterator<Key, T, Hash> &iter) {
    if (iter == end()) return;
    // Acquire the header lock for reads (not modifying bucket vec)
    ScopedRwReadLock header_lock(header_->lock_);
    header_lock.Lock();

    // Acquire the bucket lock for a write (modifying collisions)
    auto &bkt = (*iter.bucket_);
    ScopedRwWriteLock bkt_lock(bkt.lock_);
    bkt_lock.Lock();

    // Erase the element from the collision list
    list<COLLISION_T> collisions(bkt.collisions_);
    collisions.erase(iter.collision_);

    // Decrement the size of the map
    --header_->length_;
  }

  /**
   * Erase the entire map
   * */
  void clear() {
    vector<BUCKET_T> buckets(header_->buckets_);
    size_t num_buckets = buckets.size();
    buckets.clear();
    buckets.resize(num_buckets, alloc_);
    header_->length_ = 0;
  }

  /**
   * Locate an entry in the unordered_map
   *
   * @return the object pointed by key
   * @exception UNORDERED_MAP_CANT_FIND the key was not in the map
   * */
  T_Ref operator[](const Key &key) {
    auto iter = find(key);
    if (iter != end()) {
      return (*iter).val_;
    }
    throw UNORDERED_MAP_CANT_FIND.format();
  }

  /**
   * Find an object in the unordered_map
   * */
  unordered_map_iterator<Key, T, Hash> find(const Key &key) {
    if (header_ == nullptr) { shm_init(); }
    unordered_map_iterator<Key, T, Hash> iter(this);
    ScopedRwReadLock header_lock(header_->lock_);
    header_lock.Lock();
    iter.buckets_ << header_->buckets_;
    size_t bkt_id = Hash{}(key) % iter.buckets_.size();
    iter.bucket_ = iter.buckets_.begin() + bkt_id;
    BUCKET_T &bkt = (*iter.bucket_);
    ScopedRwReadLock bkt_lock(bkt.lock_);
    bkt_lock.Lock();
    iter.collisions_ << bkt.collisions_;
    iter.collision_ = find_collision(key, iter.collisions_);
    if (iter.collision_.is_end()) {
      iter.set_end();
    }
    return iter;
  }

  /** The number of entries in the map */
  size_t size() const {
    if (header_ == nullptr) {
      return 0;
    }
    return header_->length_;
  }

 private:

  /**
   * Find a key in the collision list
   * */
  list_iterator<COLLISION_T>
  find_collision(const Key &key, list<COLLISION_T> &collisions) {
    auto iter = collisions.begin();
    auto iter_end = collisions.end();
    for (; iter != iter_end; ++iter) {
      COLLISION_RET_T entry(*iter);
      if (entry.key_ == key) {
        return iter;
      }
    }
    return iter_end;
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
        vector<BUCKET_T> buckets(map_.header_->buckets_);
        for (auto &bkt : buckets) {
          bkt.lock_.ReadLock();
        }
        is_locked_ = true;
      }
    }

    /** Release all locks after iteration */
    void Unlock() {
      if (is_locked_) {
        map_.header_->lock_.ReadUnlock();
        vector<BUCKET_T> buckets(map_.header_->buckets_);
        for (auto &bkt : buckets) {
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
    unordered_map_iterator<Key, T, Hash> iter(this);
    iter.buckets_ << header_->buckets_;
    if (iter.buckets_.size() == 0) {
      return iter;
    }
    auto &bkt = iter.buckets_[0];
    iter.collisions_ << bkt.collisions_;
    iter.bucket_ = iter.buckets_.begin();
    iter.collision_ = iter.collisions_.begin();
    iter.make_correct();
    return iter;
  }

  /** Forward iterator end */
  inline unordered_map_iterator<Key, T, Hash> end() {
    unordered_map_iterator<Key, T, Hash> iter(this);
    iter.buckets_ << header_->buckets_;
    iter.bucket_ = iter.buckets_.end();
    return iter;
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
    unordered_map new_map(alloc_,
                          new_num_buckets,
                          header_->max_collisions_, header_->growth_);
    for (size_t i = 0; i < num_buckets; ++i) {
      auto &bkt = buckets[i];
      list<COLLISION_T> collisions(bkt.collisions_);
      for (auto& entry : collisions) {
        new_map.insert<false>(std::move(entry));
      }
    }
    shm_destroy();
    (*this) = new_map;
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
};

}  // namespace labstor::ipc

#undef CLASS_NAME
#undef TYPED_CLASS
#undef TYPED_HEADER

#endif  // LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_UNORDERED_MAP_H_
