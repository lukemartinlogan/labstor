//
// Created by lukemartinlogan on 11/1/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_MEMORY_MEMORY_H_
#define LABSTOR_INCLUDE_LABSTOR_MEMORY_MEMORY_H_

#include <labstor/types/basic.h>
#include <labstor/constants/singleton_macros.h>
#include <labstor/introspect/system_info.h>

#define IS_SHM_SERIALIZEABLE(T) \
  std::is_base_of<labstor::ipc::ShmSerializeable<T>, T>::value

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
    labstor::ipc::ShmArchive<T>, T>::type

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

namespace labstor::ipc {

template<typename T>
class ShmHeader;

template<typename T>
class ShmArchive;

template<typename T>
class ShmSerializeable {
 public:
  // ShmSerializeable(ShmArchive<T> &ar);
  // virtual void shm_destroy() = 0;
  // virtual void shm_serialize(ShmArchive<T> &ar) = 0;
  // virtual void shm_deserialize(ShmArchive<T> &ar) = 0;
  // void operator>>(ShmArchive<TYPED_CLASS> &ar);
  // void operator<<(ShmArchive<TYPED_CLASS> &ar)
};

#define SHM_SERIALIZEABLE_TEMPLATE0(CLASS_NAME)\
 public:\
  explicit CLASS_NAME(ShmArchive<CLASS_NAME> &ar) {\
    shm_deserialize(ar);\
  }\
  void operator>>(ShmArchive<CLASS_NAME> &ar) {\
    shm_serialize(ar);\
  }\
  void operator<<(ShmArchive<CLASS_NAME> &ar) {\
    shm_deserialize(ar);\
  }\
 private:\
  void _check_template() {\
    shm_destroy();\
  }

#define SHM_SERIALIZEABLE_TEMPLATE(CLASS_NAME, ...)\
 public:\
  explicit CLASS_NAME(ShmArchive<CLASS_NAME<__VA_ARGS__>> &ar) {\
    shm_deserialize(ar);\
  }\
  void operator>>(ShmArchive<CLASS_NAME<__VA_ARGS__>> &ar) {\
    shm_serialize(ar);\
  }\
  void operator<<(ShmArchive<CLASS_NAME<__VA_ARGS__>> &ar) {\
    shm_deserialize(ar);\
  }\
 private:\
  void _check_template() {\
    shm_destroy();\
  }

union allocator_id_t {
  struct {
    uint32_t major_;
    uint32_t minor_;
  } bits;
  uint64_t int_;

  allocator_id_t() : int_(-1) {}
  explicit allocator_id_t(uint32_t major, uint32_t minor) {
    bits.major_ = major;
    bits.minor_ = minor;
  }
  bool is_null() const { return int_ == -1; }

  bool operator==(const allocator_id_t &other) const {
    return other.int_ == int_;
  }

  bool operator!=(const allocator_id_t &other) const {
    return other.int_ != int_;
  }
};

typedef uint32_t slot_id_t;

struct Pointer {
  allocator_id_t allocator_id_;
  size_t off_;

  Pointer() : off_(0) {}
  explicit Pointer(allocator_id_t id, size_t off) :
    allocator_id_(id), off_(off) {}
  bool is_null() {
    return (off_ == 0 && allocator_id_.is_null());
  }

  bool operator==(const Pointer &other) const {
    return (other.allocator_id_ == allocator_id_ && other.off_ == off_);
  }

  bool operator !=(const Pointer &other) const {
    return (other.allocator_id_ != allocator_id_ || other.off_ != off_);
  }
};

static const Pointer kNullPointer;

}  // namespace labstor::ipc

namespace std {
template <>
struct hash<labstor::ipc::allocator_id_t> {
  std::size_t operator()(const labstor::ipc::allocator_id_t &key) const {
    return std::hash<uint64_t>{}(key.int_);
  }
};
}  // namespace std

#endif  // LABSTOR_INCLUDE_LABSTOR_MEMORY_MEMORY_H_
