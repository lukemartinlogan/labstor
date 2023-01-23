//
// Created by lukemartinlogan on 1/21/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_THREAD_SAFE_LOCKED_CONTAINER_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_THREAD_SAFE_LOCKED_CONTAINER_H_

#include <labstor/data_structures/internal/shm_container.h>

namespace labstor::ipc {

template<typename CONTAINER_T>
class locked_container;

template<typename CONTAINER_T>
class ShmHeader<locked_container<CONTAINER_T>> : public CONTAINER_T::header_t {
 public:
  typedef typename CONTAINER_T::header_t sub_header_t;

 public:
  RwLock lock_;
  using sub_header_t::sub_header_t;
};

#define CLASS_NAME locked_container
#define TYPED_CLASS locked_container<CONTAINER_T>
#define TYPED_HEADER ShmHeader<locked_container<CONTAINER_T>>

template<typename CONTAINER_T>
class locked_container : public ShmContainer {
 public:
  typedef typename CONTAINER_T::header_t sub_header_t;
  SHM_CONTAINER_TEMPLATE((CLASS_NAME), (TYPED_CLASS), (TYPED_HEADER))
  CONTAINER_T obj_;

 public:
  ////////////////////////////
  /// SHM Overrides
  ////////////////////////////
  locked_container() = default;

  template<typename ...Args>
  void shm_init_main(TYPED_HEADER *header,
                     Allocator *alloc,
                     Args&& ...args) {
    shm_init_allocator(alloc);
    shm_init_header(header, std::forward<Args>(args)...);
    obj_.shm_deserialize(alloc_, reinterpret_cast<sub_header_t*>(header_));
    obj_.SetBaseClass();
  }

  template<typename ...Args>
  void shm_weak_move_main(TYPED_HEADER *header,
                          Allocator *alloc,
                          Args&& ...args) {
    shm_init_allocator(alloc);
    shm_init_header(header, std::forward<Args>(args)...);
    obj_.shm_weak_move_main(reinterpret_cast<sub_header_t*>(header_),
                            alloc_, std::forward<Args>(args)...);
  }

  template<typename ...Args>
  void shm_strong_copy_main(TYPED_HEADER *header,
                            Allocator *alloc,
                            Args&& ...args) {
    shm_init_allocator(alloc);
    shm_init_header(header, std::forward<Args>(args)...);
    obj_.shm_strong_copy_main(reinterpret_cast<sub_header_t*>(header_),
                              alloc_, std::forward<Args>(args)...);
  }

  void shm_serialize_main() const {
    obj_.shm_serialize_main();
  }

  void shm_deserialize_main() {
    obj_.shm_deserialize_main();
  }

  void shm_destroy_main() {}

 public:
  ////////////////////////////
  /// Locked Container Ops
  ////////////////////////////
  RwLock& GetLock() {
    return header_->lock_;
  }

  CONTAINER_T& GetContainer() {
    return obj_;
  }
};

#undef CLASS_NAME
#undef TYPED_CLASS
#undef TYPED_HEADER

}  // namespace labstor::ipc

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_THREAD_SAFE_LOCKED_CONTAINER_H_
