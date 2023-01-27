//
// Created by lukemartinlogan on 1/26/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_TUPLE_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_TUPLE_H_

#include "labstor/data_structures/internal/shm_container.h"
#include "labstor/data_structures/internal/shm_archive_or_t.h"
#include "labstor/types/argpack.h"


#define CLASS_NAME tuple
#define TYPED_CLASS tuple<Containers...>
#define TYPED_HEADER ShmHeader<tuple<Containers...>>

namespace labstor::ipc {

/** ShmHeaderRecur forward declaration */
template<
  size_t idx,
  typename ContainerT=EndTemplateRecurrence,
  typename ...Containers>
struct ShmHeaderRecur;

/** Tuple forward declaration */
template<typename ...Containers>
class tuple;

/** Tuple SHM header */
template<typename ...Containers>
class ShmHeader<tuple<Containers...>> {
  /**< All object headers */
  labstor::tuple_wrap<ShmHeaderOrT, Containers...> hdrs_;

  /** Get the internal reference to the ith object */
  template<size_t i>
  decltype(auto) internal_ref(Allocator *alloc) {
    return hdrs_.template Get<i>().internal_ref(alloc);
  }

  /** Destructor */
  void shm_destroy(Allocator *alloc) {
    labstor::ForwardIterateTuple::Apply(
      hdrs_,
      [alloc](size_t i, auto &obj_hdr) constexpr {
        obj_hdr->shm_destroy(alloc);
      }
    );
  }
};

/** A tuple of objects to store in shared memory */
template<typename ...Containers>
class tuple : public ShmContainer {
 public:
  /**====================================
   * Variables & Types
   *===================================*/

  typedef TYPED_HEADER header_t; /**< Index to header type */
  header_t *header_; /**< The shared-memory header */
  labstor::tuple_wrap<lipc::ShmRef, Containers...> objs_; /**< Constructed objects */

 public:
  /**====================================
   * Shm Overrides
   * ===================================*/

  /** Default constructor */
  CLASS_NAME() = default;

  /** Default shm constructor */
  void shm_init_main(TYPED_HEADER *header,
                     Allocator *alloc) {
  }

  /** Move constructor */
  void shm_weak_move_main(TYPED_HEADER *header,
                          Allocator *alloc, CLASS_NAME &other) {
  }

  /** Copy constructor */
  void shm_strong_copy_main(TYPED_HEADER *header,
                            Allocator *alloc, const CLASS_NAME &other) {
  }

  /** Destroy the shared-memory data. */
  void shm_destroy_main() {}

  /** Store into shared memory */
  void shm_serialize_main() const {}

  /** Load from shared memory */
  void shm_deserialize_main() {}

  /**====================================
   * Constructors
   * ===================================*/

  /** Constructor. Allocate header with default allocator. */
  template<typename ...Args>
  explicit CLASS_NAME(Args &&...args) {
    shm_init(std::forward<Args>(args)...);
  }

  /** Constructor. Allocate header with default allocator. */
  template<typename ...Args>
  void shm_init(Args &&...args) {
  }

  /**====================================
   * Serialization
   * ===================================*/

  /** Serialize into a Pointer */
  void shm_serialize(TypedPointer<TYPED_CLASS> &ar) const {
    ar = GetAllocator()->template
      Convert<TYPED_HEADER, Pointer>(header_);
    shm_serialize_main();
  }

  /** Serialize into an AtomicPointer */
  void shm_serialize(TypedAtomicPointer<TYPED_CLASS> &ar) const {
    ar = GetAllocator()->template
      Convert<TYPED_HEADER, AtomicPointer>(header_);
    shm_serialize_main();
  }

  /** Override << operators */
  SHM_SERIALIZE_OPS((TYPED_CLASS))

  /**====================================
   * Deserialization
   * ===================================*/

  /** Deserialize object from a raw pointer */
  bool shm_deserialize(const TypedPointer<TYPED_CLASS> &ar) {
    return shm_deserialize(
      LABSTOR_MEMORY_MANAGER->GetAllocator(ar.allocator_id_),
      ar.ToOffsetPointer()
    );
  }

  /** Deserialize object from allocator + offset */
  bool shm_deserialize(Allocator *alloc, OffsetPointer header_ptr) {
    if (header_ptr.IsNull()) { return false; }
    return shm_deserialize(alloc,
                           alloc->Convert<
                             TYPED_HEADER,
                             OffsetPointer>(header_ptr));
  }

  /** Deserialize object from another object (weak copy) */
  bool shm_deserialize(const CLASS_NAME &other) {
    if (other.IsNull()) { return false; }
    return shm_deserialize(other.GetAllocator(), other.header_);
  }

  /** Deserialize object from allocator + header */
  bool shm_deserialize(Allocator *alloc,
                       TYPED_HEADER *header) {
    /*header_ = header;
    labstor::ForwardIterateTuple::Apply(
      objs_,
      [this, alloc](size_t i, auto &obj_) constexpr {
        obj_->shm_deserialize(alloc, this->header_->template Get<i>());
      }
    );*/
    shm_deserialize_main();
    return true;
  }

  /** Constructor. Deserialize the object from the reference. */
  template<typename ...Args>
  void shm_init(lipc::ShmRef<TYPED_CLASS> &obj) {
    shm_deserialize(obj->GetAllocator(), obj->header_);
  }

  /** Override >> operators */
  SHM_DESERIALIZE_OPS ((TYPED_CLASS))

  /**====================================
   * Destructors
   * ===================================*/

  /** Destructor */
  ~CLASS_NAME() {
    /*labstor::ForwardIterateTuple::Apply(
      objs_,
      [](size_t i, auto &obj_) constexpr {
        obj_.shm_destroy(true);
      }
    );*/
  }

  /** Shm Destructor */
  void shm_destroy(bool destroy_header = true) {
    /*if (!IsValid()) { return; }
    if (IsDataValid()) {
      labstor::ForwardIterateTuple::Apply(
        objs_,
        [](size_t i, auto &obj_) constexpr {
          obj_.shm_destroy(false);
        }
      );
      shm_destroy_main();
    }*/
  }

  /**====================================
   * Move Operations
   * ===================================*/

  /** Move constructor */
  CLASS_NAME(CLASS_NAME &&other) noexcept {
  }

  /** Move assignment operator */
  CLASS_NAME& operator=(CLASS_NAME &&other) noexcept {
    /*labstor::ForwardIterateTuple::Apply(
      objs_,
      [other](size_t i, auto &obj_) constexpr {
        obj_ = std::move(other.obj_);
      }
    );*/
    return *this;
  }

  /** Move shm_init constructor */
  void shm_init_main(TYPED_HEADER *header,
                     lipc::Allocator *alloc,
                     CLASS_NAME &&other) noexcept {
    shm_weak_move(header, alloc, other);
  }

  /** Move operation */
  void shm_weak_move(TYPED_HEADER *header,
                     lipc::Allocator *alloc,
                     CLASS_NAME &other) {
    /*labstor::ForwardIterateTuple::Apply(
      objs_,
      [header, alloc, other](size_t i, auto &obj_) constexpr {
        obj_.shm_weak_move(header, alloc, other.objs_.template Get<i>());
      }
    );*/
  }

  /**====================================
   * Copy Operations
   * ===================================*/

  /** Copy constructor */
  CLASS_NAME(const CLASS_NAME &other) noexcept {
    shm_init(other);
  }

  /** Copy assignment constructor */
  CLASS_NAME &operator=(const CLASS_NAME &other) {
    if (this != &other) {
      shm_strong_copy(
        typed_nullptr<TYPED_HEADER >(),
        typed_nullptr<Allocator>(),
        other);
    }
    return *this;
  }

  /** Copy shm_init constructor */
  void shm_init_main(TYPED_HEADER *header,
                     lipc::Allocator *alloc,
                     const CLASS_NAME &other) {
    shm_strong_copy(header, alloc, other);
  }

  /** Strong Copy operation */
  void shm_strong_copy(TYPED_HEADER *header, lipc::Allocator *alloc,
                       const CLASS_NAME &other) {
    if (other.IsNull()) { return; }
    shm_destroy(false);
    shm_strong_copy_main(header, alloc, other);
    SetDestructable();
  }

  /**====================================
   * Container Flag Operations
   * ===================================*/

  /** Sets this object as destructable */
  void SetDestructable() {
    return Get<0>()->SetDestructable();
  }

  /** Sets this object as not destructable */
  void UnsetDestructable() {
    return Get<0>()->UnsetDestructable();
  }

  /** Check if this container is destructable */
  bool IsDestructable() const {
    return Get<0>()->IsDestructable();
  }

  /** Check if container has a valid header */
  bool IsValid() const {
    return Get<0>()->IsValid();
  }

  /** Set container header invalid */
  void UnsetValid() {
    Get<0>()->UnsetValid();
  }

  /**====================================
   * Header Flag Operations
   * ===================================*/

  /** Check if header's data is valid */
  bool IsDataValid() const {
    return Get<0>()->IsDataValid();
  }

  /** Check if header's data is valid */
  void UnsetDataValid() const {
    return Get<0>()->UnsetDataValid();
  }

  /** Check if null */
  bool IsNull() const {
    return Get<0>()->IsNull();
  }

  /** Get a typed pointer to the object */
  template<typename POINTER_T>
  POINTER_T GetShmPointer() const {
    return GetAllocator()->template
      Convert<TYPED_HEADER, POINTER_T>(header_);
  }

  /**====================================
   * Query Operations
   * ===================================*/

  /** Get the allocator for this container */
  Allocator* GetAllocator() {
    return Get<0>()->GetAllocator();
  }

  /** Get the allocator for this container */
  Allocator* GetAllocator() const {
    return Get<0>()->GetAllocator();
  }

  /** Get the shared-memory allocator id */
  allocator_id_t GetAllocatorId() const {
    return GetAllocator()->GetId();
  }

  /** Get the ith constructed container in the tuple */
  template<size_t i>
  auto& Get() {
    return objs_.template Get<i>();
  }
};

}  // namespace labstor::ipc

#undef CLASS_NAME
#undef TYPED_CLASS
#undef TYPED_HEADER

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_TUPLE_H_
