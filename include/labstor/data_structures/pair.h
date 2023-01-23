//
// Created by lukemartinlogan on 1/15/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_PAIR_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_PAIR_H_

#include "data_structure.h"
#include "internal/shm_archive_or_t.h"

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
#define TYPED_HEADER ShmHeader<TYPED_CLASS>

/** pair shared-memory header */
template<typename FirstT, typename SecondT>
struct ShmHeader<TYPED_CLASS> : public ShmBaseHeader {
  ShmHeaderOrT<FirstT> first_;
  ShmHeaderOrT<SecondT> second_;

  ShmHeader() = default;

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

  void shm_destroy(Allocator *alloc) {
    first_.shm_destroy(alloc);
    second_.shm_destroy(alloc);
  }
};

/**
 * A string of characters.
 * */
template<typename FirstT, typename SecondT>
class pair : public ShmContainer {
 public:
  SHM_CONTAINER_TEMPLATE((CLASS_NAME), (TYPED_CLASS), (TYPED_HEADER))

 public:
  lipc::Ref<FirstT> first_;
  lipc::Ref<SecondT> second_;

  public:
  /** Default constructor */
  pair() = default;

  /** Default shm constructor */
  void shm_init_main(TYPED_HEADER *header, Allocator *alloc) {
    shm_init_allocator(alloc);
    shm_init_header(header, alloc);
  }

  /** Construct pair by forwarding parameters */
  void shm_init_main(TYPED_HEADER *header,
                     Allocator *alloc,
                     FirstT &&first, SecondT &&second) {
    shm_init_allocator(alloc);
    shm_init_header(header,
                    alloc_,
                    std::forward<FirstT>(first),
                    std::forward<SecondT>(second));
    first_ = lipc::Ref<FirstT>(header_->first_.internal_ref(alloc_));
    second_ = lipc::Ref<SecondT>(header_->second_.internal_ref(alloc_));
  }

  /** Construct pair by copying parameters */
  void shm_init_main(TYPED_HEADER *header,
                     Allocator *alloc,
                     const FirstT &first, const SecondT &second) {
    shm_init_allocator(alloc);
    shm_init_header(header,
                    alloc_, first, second);
    first_ = lipc::Ref<FirstT>(header_->first_.internal_ref(alloc_));
    second_ = lipc::Ref<SecondT>(header_->second_.internal_ref(alloc_));
  }

  /** Move constructor */
  void shm_weak_move_main(TYPED_HEADER *header,
                          Allocator *alloc, pair &other) {
    shm_init_main(header,
                  alloc,
                  std::move(*other.first_),
                  std::move(*other.second_));
  }

  /** Copy constructor */
  void shm_strong_copy_main(TYPED_HEADER *header,
                            Allocator *alloc, const pair &other) {
    shm_init_main(header,
                  alloc, *other.first_, *other.second_);
  }

  /**
   * Destroy the shared-memory data.
   * */
  void shm_destroy_main() {
    header_->shm_destroy(alloc_);
  }

  /** Store into shared memory */
  void shm_serialize_main() const {}

  /** Load from shared memory */
  void shm_deserialize_main() {
    first_ = lipc::Ref<FirstT>(header_->first_.internal_ref(alloc_));
    second_ = lipc::Ref<SecondT>(header_->second_.internal_ref(alloc_));
  }

  /** Get the first object */
  FirstT& GetFirst() { return *first_; }

  /** Get the first object (const) */
  FirstT& GetFirst() const { return *first_; }

  /** Get the second object */
  SecondT& GetSecond() { return *second_; }

  /** Get the second object (const) */
  SecondT& GetSecond() const { return *second_; }

  /** Get the first object (treated as key) */
  FirstT& GetKey() { return *first_; }

  /** Get the first object (treated as key) (const) */
  FirstT& GetKey() const { return *first_; }

  /** Get the second object (treated as value) */
  SecondT& GetVal() { return *second_; }

  /** Get the second object (treated as value) (const) */
  SecondT& GetVal() const { return *second_; }
};

#undef CLASS_NAME
#undef TYPED_CLASS
#undef TYPED_HEADER

}  // namespace labstor::ipc

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_PAIR_H_
