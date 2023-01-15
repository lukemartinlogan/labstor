//
// Created by lukemartinlogan on 1/15/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_PAIR_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_PAIR_H_

#include "data_structure.h"
#include "internal/shm_archive_or_t.h"
#include "labstor/data_structures/smart_ptr/manual_ptr.h"

namespace labstor::ipc {

/** forward declaration for string */
template<typename FirstT, typename SecondT>
class pair;

/**
 * MACROS used to simplify the string namespace
 * Used as inputs to the SHM_CONTAINER_TEMPLATE
 * */
#define CLASS_NAME pair
#define TYPED_CLASS pair<FirstT, SecondT>

/** pair shared-memory header */
template<typename FirstT, typename SecondT>
struct ShmHeader<TYPED_CLASS> : public ShmBaseHeader {
  ShmArchiveOrT<FirstT> first_;
  ShmArchiveOrT<SecondT> second_;

  explicit ShmHeader(Allocator *alloc)
  : first_(alloc), second_(alloc) {}

  explicit ShmHeader(Allocator *alloc,
                     FirstT &&first,
                     SecondT &&second)
  : first_(alloc, std::forward<FirstT>(first)),
    second_(alloc, std::forward<SecondT>(second)) {}

  explicit ShmHeader(Allocator *alloc,
                     const FirstT &first,
                     const SecondT &second)
    : first_(alloc, first), second_(alloc, second) {}
};

/**
 * A string of characters.
 * */
template<typename FirstT, typename SecondT>
class pair : public SHM_CONTAINER(TYPE_WRAP(TYPED_CLASS)) {
  public:
  BASIC_SHM_CONTAINER_TEMPLATE
  lipc::Ref<FirstT> first_;
  lipc::Ref<SecondT> second_;

  public:
  /** Default constructor */
  pair() = default;

  /** Default shm constructor */
  void shm_init_main(ShmArchive<TYPED_CLASS> *ar, Allocator *alloc) {
    shm_init_header(ar, alloc, alloc);
  }

  /** Construct pair by forwarding parameters */
  void shm_init_main(ShmArchive<TYPED_CLASS> *ar,
                     Allocator *alloc,
                     FirstT &&first, SecondT &&second) {
    shm_init_header(ar, alloc,
                    alloc,
                    std::forward<FirstT>(first),
                    std::forward<SecondT>(second));
  }

  /** Construct pair by copying parameters */
  void shm_init_main(ShmArchive<TYPED_CLASS> *ar,
                     Allocator *alloc,
                     const FirstT &first, const SecondT &second) {
    shm_init_header(ar, alloc,
                    alloc, first, second);
    first_ = lipc::Ref<FirstT>(header_->first_.internal_ref(alloc_));
    second_ = lipc::Ref<SecondT>(header_->second_.internal_ref(alloc_));
  }

  /** Move constructor */
  void WeakMove(ShmArchive<TYPED_CLASS> *ar,
                Allocator *alloc, pair &other) {
    SHM_WEAK_MOVE_START(SHM_WEAK_MOVE_DEFAULT(TYPED_CLASS),
                        std::move(*other.first_),
                        std::move(*other.second_))
    SHM_WEAK_MOVE_END()
  }

  /** Copy constructor */
  void StrongCopy(ShmArchive<TYPED_CLASS> *ar,
                  Allocator *alloc, const pair &other) {
    SHM_STRONG_COPY_START(SHM_STRONG_COPY_DEFAULT(TYPED_CLASS),
                          *other.first_, *other.second_)
    SHM_STRONG_COPY_END()
  }

  /**
   * Destroy the shared-memory data.
   * */
  void shm_destroy(bool destroy_header = true) {
    SHM_DESTROY_DATA_START
    SHM_DESTROY_DATA_END
    SHM_DESTROY_END
  }

  /** Store into shared memory */
  void shm_serialize(ShmArchive<TYPED_CLASS> &ar) const {
    shm_serialize_header(ar.header_ptr_);
  }

  /** Load from shared memory */
  void shm_deserialize(const ShmArchive<TYPED_CLASS> &ar) {
    if(!shm_deserialize_header(ar.header_ptr_)) { return; }
    first_ = lipc::Ref<FirstT>(header_->first_.internal_ref(alloc_));
    second_ = lipc::Ref<SecondT>(header_->first_.internal_ref(alloc_));
  }
};

#undef CLASS_NAME
#undef TYPED_CLASS

}  // namespace labstor::ipc

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_PAIR_H_
