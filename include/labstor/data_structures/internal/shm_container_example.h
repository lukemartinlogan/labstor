
/**
 * MACRO vs TEMPLATE
 *
 * TEMPLATE:
 * - Can make shm_weak_move, shm_strong_copy
 * */

#include "shm_container.h"

namespace labstor::ipc {

template<typename T>
class CLASS_NAME;

template<typename T>
class TYPED_HEADER : public ShmBaseHeader {
};

#define CLASS_NAME ShmContainerExample
#define TYPED_CLASS ShmContainerExample<T>
#define TYPED_HEADER ShmHeader<CLASS_NAME>

template<typename T>
class ShmContainerExample : public ShmContainer {
 public:
  ////////////////////////
  /// TO IMPLEMENT
  ////////////////////////

  /** Default constructor */
  ShmContainerExample() = default;

  /** Default shm constructor */
  void shm_init_main(TYPED_HEADER *header,
                     Allocator *alloc) {
    shm_init_allocator(alloc);
    shm_init_header(header);
  }

  /** Weak move operator */
  void shm_weak_move_main(TYPED_HEADER *header,
                          Allocator *alloc,
                          CLASS_NAME &other) {
  }

  /** Strong copy operator */
  void shm_strong_copy_main(TYPED_HEADER *header,
                            Allocator *alloc,
                            const CLASS_NAME &other) {
  }

  /** Serialize main */
  void shm_serialize_main() const {
  }

  /** Deserialize main */
  void shm_deserialize_main() {
  }

  /** Destroy shared-memory data safely */
  void shm_destroy_main() {
  }

  ////////////////////////////
  /// INHERITANCE BEGINS
  ////////////////////////////

 public:
  ////////////////////////////
  /// CLASS VARIABLES
  ////////////////////////////
  typedef TYPED_HEADER header_t;
  header_t *header_;

 public:
  ///////////////////////////
  /// SHM Init Constructors
  ////////////////////////////

  /** Constructor. Allocate header with default allocator. */
  template<typename ...Args>
  explicit CLASS_NAME(Args&& ...args) {
    shm_init(std::forward<Args>(args)...);
  }

  /** Constructor. Allocate header with default allocator. */
  template<typename ...Args>
  void shm_init(Args&& ...args) {
    shm_destroy(false);
    shm_init_main(typed_nullptr<TYPED_HEADER>(),
                  typed_nullptr<Allocator>(),
                  std::forward<Args>(args)...);
  }

  /** Constructor. Allocate header with specific allocator. */
  template<typename ...Args>
  void shm_init(lipc::Allocator *alloc, Args&& ...args) {
    shm_destroy(false);
    shm_init_main(typed_nullptr<TYPED_HEADER>(),
                  alloc,
                  std::forward<Args>(args)...);
  }

  /** Constructor. Initialize an already-allocated header. */
  template<typename ...Args>
  void shm_init(TYPED_HEADER &header,
                lipc::Allocator *alloc, Args&& ...args) {
    shm_destroy(false);
    shm_init_main(&header, alloc, std::forward<Args>(args)...);
  }

  /**
   * Initialize a data structure's header.
   * A container will never re-set or re-allocate its header once it has
   * been set the first time.
   * */
  template<typename ...Args>
  void shm_init_header(TYPED_HEADER *header,
                       Args&& ...args) {
    if (IsValid()) {
      // The container already has a valid header
      header_->SetBits(SHM_CONTAINER_DATA_VALID);
    } else if (header == nullptr) {
      // Allocate and initialize a new header
      header_ = alloc_->template
        AllocateConstructObjs<TYPED_HEADER>(
        1, std::forward<Args>(args)...);
      header_->SetBits(
        SHM_CONTAINER_DATA_VALID |
          SHM_CONTAINER_HEADER_DESTRUCTABLE);
      flags_.SetBits(
        SHM_CONTAINER_VALID |
          SHM_CONTAINER_DESTRUCTABLE);
    } else {
      // Initialize the input header
      Pointer header_ptr;
      header_ = header;
      Allocator::ConstructObj<TYPED_HEADER>(
        *header_, std::forward<Args>(args)...);
      header_->SetBits(
        SHM_CONTAINER_DATA_VALID);
      flags_.SetBits(
        SHM_CONTAINER_VALID |
          SHM_CONTAINER_DESTRUCTABLE);
    }
  }

  ////////////////////////
  /// SHM Serialization
  ////////////////////////

  /** Serialize into a Pointer */
  void shm_serialize(TypedPointer<TYPED_CLASS> &ar) const {
    ar = alloc_->template
      Convert<TYPED_HEADER, Pointer>(header_);
    shm_serialize_main();
  }

  /** Serialize into an AtomicPointer */
  void shm_serialize(TypedAtomicPointer<TYPED_CLASS> &ar) const {
    ar = alloc_->template
      Convert<TYPED_HEADER, AtomicPointer>(header_);
    shm_serialize_main();
  }

  /** Override << operators */
  SHM_SERIALIZE_OPS((TYPED_CLASS))

  ////////////////////////
  /// SHM Deserialization
  ////////////////////////

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
    shm_deserialize(alloc,
                    alloc->Convert<
                      TYPED_HEADER,
                      OffsetPointer>(header_ptr));
  }

  /** Deserialize object from allocator + header */
  bool shm_deserialize(Allocator *alloc,
                       TYPED_HEADER *header) {
    flags_.UnsetBits(SHM_CONTAINER_VALID | SHM_CONTAINER_DESTRUCTABLE);
    alloc_ = alloc;
    header_ = header;
    flags_.SetBits(SHM_CONTAINER_VALID);
    shm_deserialize_main();
    return true;
  }

  /** Constructor. Deserialize the object from the reference. */
  template<typename ...Args>
  void shm_init(lipc::Ref<TYPED_CLASS> &ref) {
    shm_destroy(false);
    shm_deserialize(ref.obj_.GetAllocator(), ref.obj_.header_);
  }

  /** Override >> operators */
  SHM_DESERIALIZE_OPS((TYPED_CLASS))

  ////////////////////////
  /// Destructors
  ////////////////////////

  /** Destructor */
  ~CLASS_NAME() {
    if (IsDestructable()) {
      shm_destroy(true);
    }
  }

  /** Shm Destructor */
  void shm_destroy(bool destroy_header = true) {
    if (!IsValid()) { return; }
    if (IsDataValid()) {
      shm_destroy_main();
    }
    UnsetDataValid();
    if (destroy_header &&
      header_->OrBits(SHM_CONTAINER_HEADER_DESTRUCTABLE)) {
      alloc_->FreePtr<TYPED_HEADER>(header_);
      UnsetValid();
    }
  }

  ////////////////////////
  /// Move Constructors
  ////////////////////////

  /** Move constructor */
  CLASS_NAME(CLASS_NAME &&other) noexcept {
    shm_weak_move(typed_nullptr<TYPED_HEADER>(),
                  typed_nullptr<Allocator>(),
                  other);
  }

  /** Move assignment operator */
  CLASS_NAME& operator=(CLASS_NAME &&other) noexcept {
    if (this != &other) {
      shm_weak_move(typed_nullptr<TYPED_HEADER>(),
                    typed_nullptr<Allocator>(),
                    other);
    }
    return *this;
  }

  /** Move shm_init constructor */
  void shm_init_main(TYPED_HEADER *header,
                     lipc::Allocator *alloc,
                     CLASS_NAME &&other) noexcept {
    shm_weak_move(typed_nullptr<TYPED_HEADER>(),
                  typed_nullptr<Allocator>(),
                  other);
  }

  /** Move operation */
  void shm_weak_move(TYPED_HEADER *header,
                     lipc::Allocator *alloc,
                     CLASS_NAME &other) {
    if (other.IsNull()) { return; }
    shm_weak_move_main(header, alloc, other);
    if (!other.IsDestructable()) {
      UnsetDestructable();
    }
    other.UnsetDataValid();
    other.shm_destroy(true);
  }

  ////////////////////////
  /// Copy Constructors
  ////////////////////////

  /** Copy constructor */
  CLASS_NAME(const CLASS_NAME &other) noexcept {
    shm_init(other);
  }

  /** Copy assignment constructor */
  CLASS_NAME& operator=(const CLASS_NAME &other) {
    if (this != &other) {
      shm_strong_copy(typed_nullptr<TYPED_HEADER>(),
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
  void shm_strong_copy(TYPED_HEADER *header,
                       lipc::Allocator *alloc,
                       const CLASS_NAME &other) {
    if (other.IsNull()) { return; }
    shm_strong_copy_main(header, alloc, other);
    if (!other.IsDestructable()) {
      UnsetDestructable();
    }
    other.UnsetDataValid();
  }

  /////////////////////
  /// Flag Operations
  /////////////////////

  /** Sets this object as destructable */
  void SetDestructable() {
    flags_.SetBits(SHM_CONTAINER_DESTRUCTABLE);
  }

  /** Sets this object as indestructable */
  void UnsetDestructable() {
    flags_.UnsetBits(SHM_CONTAINER_DESTRUCTABLE);
  }

  /** Check if this container is destructable */
  bool IsDestructable() const {
    return flags_.OrBits(SHM_CONTAINER_DESTRUCTABLE);
  }

  /** Check if header's data is valid */
  bool IsDataValid() const {
    return header_->OrBits(SHM_CONTAINER_DATA_VALID);
  }

  /** Check if header's data is valid */
  void UnsetDataValid() const {
    header_->UnsetBits(SHM_CONTAINER_DATA_VALID);
  }

  /** Check if container has a valid header */
  bool IsValid() const {
    return flags_.OrBits(SHM_CONTAINER_VALID);
  }

  /** Set container header invalid */
  void UnsetValid() {
    flags_.UnsetBits(SHM_CONTAINER_VALID | SHM_CONTAINER_DESTRUCTABLE);
  }

  /** Check if null */
  bool IsNull() const {
    return !IsValid() || !IsDataValid();
  }
};

}