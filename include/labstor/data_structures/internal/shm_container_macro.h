#ifndef LABSTOR_DATA_STRUCTURES_INTERNAL_SHM_CONTAINER_MACRO_H_
#define LABSTOR_DATA_STRUCTURES_INTERNAL_SHM_CONTAINER_MACRO_H_
#define SHM_CONTAINER_TEMPLATE(CLASS_NAME,TYPED_CLASS,TYPED_HEADER)\
public:\
typedef TYPE_UNWRAP(TYPED_HEADER) header_t;\
header_t *header_;\
\
public:\
/** Constructor. Allocate header with default allocator. */\
template<typename ...Args>\
explicit TYPE_UNWRAP(CLASS_NAME)(Args&& ...args) {\
  shm_init(std::forward<Args>(args)...);\
}\
\
/** Constructor. Allocate header with default allocator. */\
template<typename ...Args>\
void shm_init(Args&& ...args) {\
  shm_destroy(false);\
  shm_init_main(typed_nullptr<TYPE_UNWRAP(TYPED_HEADER)>(),\
                typed_nullptr<Allocator>(),\
                std::forward<Args>(args)...);\
}\
\
/** Constructor. Allocate header with specific allocator. */\
template<typename ...Args>\
void shm_init(lipc::Allocator *alloc, Args&& ...args) {\
  shm_destroy(false);\
  shm_init_main(typed_nullptr<TYPE_UNWRAP(TYPED_HEADER)>(),\
                alloc,\
                std::forward<Args>(args)...);\
}\
\
/** Constructor. Initialize an already-allocated header. */\
template<typename ...Args>\
void shm_init(TYPE_UNWRAP(TYPED_HEADER) &header,\
              lipc::Allocator *alloc, Args&& ...args) {\
  shm_destroy(false);\
  shm_init_main(&header, alloc, std::forward<Args>(args)...);\
}\
\
/**\
 * Initialize a data structure's header.\
 * A container will never re-set or re-allocate its header once it has\
 * been set the first time.\
 * */\
template<typename ...Args>\
void shm_init_header(TYPE_UNWRAP(TYPED_HEADER) *header,\
                     Args&& ...args) {\
  if (IsValid()) {\
    header_->SetBits(SHM_CONTAINER_DATA_VALID);\
  } else if (header == nullptr) {\
    Pointer p;\
    header_ = alloc_->template\
      AllocateConstructObjs<TYPE_UNWRAP(TYPED_HEADER)>(\
      1, p, std::forward<Args>(args)...);\
    header_->SetBits(\
      SHM_CONTAINER_DATA_VALID |\
        SHM_CONTAINER_HEADER_DESTRUCTABLE);\
    flags_.SetBits(\
      SHM_CONTAINER_VALID |\
        SHM_CONTAINER_DESTRUCTABLE);\
  } else {\
    Pointer header_ptr;\
    header_ = header;\
    Allocator::ConstructObj<TYPE_UNWRAP(TYPED_HEADER)>(\
      *header_, std::forward<Args>(args)...);\
    header_->SetBits(\
      SHM_CONTAINER_DATA_VALID);\
    flags_.SetBits(\
      SHM_CONTAINER_VALID |\
        SHM_CONTAINER_DESTRUCTABLE);\
  }\
}\
\
/** Serialize into a Pointer */\
void shm_serialize(TypedPointer<TYPE_UNWRAP(TYPED_CLASS)> &ar) const {\
  ar = alloc_->template\
    Convert<TYPE_UNWRAP(TYPED_HEADER), Pointer>(header_);\
  shm_serialize_main();\
}\
\
/** Serialize into an AtomicPointer */\
void shm_serialize(TypedAtomicPointer<TYPE_UNWRAP(TYPED_CLASS)> &ar) const {\
  ar = alloc_->template\
    Convert<TYPE_UNWRAP(TYPED_HEADER), AtomicPointer>(header_);\
  shm_serialize_main();\
}\
\
/** Override << operators */\
SHM_SERIALIZE_OPS((TYPE_UNWRAP(TYPED_CLASS)))\
\
/** Deserialize object from a raw pointer */\
bool shm_deserialize(const TypedPointer<TYPE_UNWRAP(TYPED_CLASS)> &ar) {\
  return shm_deserialize(\
    LABSTOR_MEMORY_MANAGER->GetAllocator(ar.allocator_id_),\
    ar.ToOffsetPointer()\
  );\
}\
\
/** Deserialize object from allocator + offset */\
bool shm_deserialize(Allocator *alloc, OffsetPointer header_ptr) {\
  if (header_ptr.IsNull()) { return false; }\
  return shm_deserialize(alloc,\
                         alloc->Convert<\
                           TYPE_UNWRAP(TYPED_HEADER),\
                           OffsetPointer>(header_ptr));\
}\
\
/** Deserialize object from another object (weak copy) */\
bool shm_deserialize(const TYPE_UNWRAP(CLASS_NAME) &other) {\
  return shm_deserialize(other.GetAllocator(), other.header_);\
}\
\
/** Deserialize object from allocator + header */\
bool shm_deserialize(Allocator *alloc,\
                     TYPE_UNWRAP(TYPED_HEADER) *header) {\
  flags_.UnsetBits(SHM_CONTAINER_VALID | SHM_CONTAINER_DESTRUCTABLE);\
  alloc_ = alloc;\
  header_ = header;\
  flags_.SetBits(SHM_CONTAINER_VALID);\
  shm_deserialize_main();\
  return true;\
}\
\
/** Constructor. Deserialize the object from the reference. */\
template<typename ...Args>\
void shm_init(lipc::Ref<TYPE_UNWRAP(TYPED_CLASS)> &obj) {\
  shm_destroy(false);\
  shm_deserialize(obj->GetAllocator(), obj->header_);\
}\
\
/** Override >> operators */\
SHM_DESERIALIZE_OPS((TYPE_UNWRAP(TYPED_CLASS)))\
\
/** Destructor */\
~TYPE_UNWRAP(CLASS_NAME)() {\
  if (IsDestructable()) {\
    shm_destroy(true);\
  }\
}\
\
/** Shm Destructor */\
void shm_destroy(bool destroy_header = true) {\
  if (!IsValid()) { return; }\
  if (IsDataValid()) {\
    shm_destroy_main();\
  }\
  UnsetDataValid();\
  if (destroy_header &&\
    header_->OrBits(SHM_CONTAINER_HEADER_DESTRUCTABLE)) {\
    alloc_->FreePtr<TYPE_UNWRAP(TYPED_HEADER)>(header_);\
    UnsetValid();\
  }\
}\
\
/** Move constructor */\
TYPE_UNWRAP(CLASS_NAME)(TYPE_UNWRAP(CLASS_NAME) &&other) noexcept {\
  shm_weak_move(typed_nullptr<TYPE_UNWRAP(TYPED_HEADER)>(),\
    typed_nullptr<Allocator>(),\
    other);\
}\
\
/** Move assignment operator */\
TYPE_UNWRAP(CLASS_NAME)& operator=(TYPE_UNWRAP(CLASS_NAME) &&other) noexcept {\
if (this != &other) {\
shm_weak_move(typed_nullptr<TYPE_UNWRAP(TYPED_HEADER)>(),\
  typed_nullptr<Allocator>(),\
  other);\
}\
return *this;\
}\
\
/** Move shm_init constructor */\
void shm_init_main(TYPE_UNWRAP(TYPED_HEADER) *header,\
                   lipc::Allocator *alloc,\
                   TYPE_UNWRAP(CLASS_NAME) &&other) noexcept {\
  shm_weak_move(header, alloc, other);\
}\
\
/** Move operation */\
void shm_weak_move(TYPE_UNWRAP(TYPED_HEADER) *header,\
                   lipc::Allocator *alloc,\
                   TYPE_UNWRAP(CLASS_NAME) &other) {\
  if (other.IsNull()) { return; }\
  shm_destroy(false);\
  shm_weak_move_main(header, alloc, other);\
  if (!other.IsDestructable()) {\
    UnsetDestructable();\
  }\
  other.UnsetDataValid();\
  other.shm_destroy(true);\
}\
\
/** Copy constructor */\
TYPE_UNWRAP(CLASS_NAME)(const TYPE_UNWRAP(CLASS_NAME) &other) noexcept {\
  shm_init(other);\
}\
\
/** Copy assignment constructor */\
TYPE_UNWRAP(CLASS_NAME)& operator=(const TYPE_UNWRAP(CLASS_NAME) &other) {\
  if (this != &other) {\
    shm_strong_copy(typed_nullptr<TYPE_UNWRAP(TYPED_HEADER)>(),\
                    typed_nullptr<Allocator>(),\
                    other);\
  }\
  return *this;\
}\
\
/** Copy shm_init constructor */\
void shm_init_main(TYPE_UNWRAP(TYPED_HEADER) *header,\
                   lipc::Allocator *alloc,\
                   const TYPE_UNWRAP(CLASS_NAME) &other) {\
  shm_strong_copy(header, alloc, other);\
}\
\
/** Strong Copy operation */\
void shm_strong_copy(TYPE_UNWRAP(TYPED_HEADER) *header,\
                     lipc::Allocator *alloc,\
                     const TYPE_UNWRAP(CLASS_NAME) &other) {\
  if (other.IsNull()) { return; }\
  shm_destroy(false);\
  shm_strong_copy_main(header, alloc, other);\
  if (!other.IsDestructable()) {\
    UnsetDestructable();\
  }\
}\
\
/** Sets this object as destructable */\
void SetDestructable() {\
  flags_.SetBits(SHM_CONTAINER_DESTRUCTABLE);\
}\
\
/** Sets this object as indestructable */\
void UnsetDestructable() {\
  flags_.UnsetBits(SHM_CONTAINER_DESTRUCTABLE);\
}\
\
/** Check if this container is destructable */\
bool IsDestructable() const {\
  return flags_.OrBits(SHM_CONTAINER_DESTRUCTABLE);\
}\
\
/** Check if header's data is valid */\
bool IsDataValid() const {\
  return header_->OrBits(SHM_CONTAINER_DATA_VALID);\
}\
\
/** Check if header's data is valid */\
void UnsetDataValid() const {\
  header_->UnsetBits(SHM_CONTAINER_DATA_VALID);\
}\
\
/** Check if container has a valid header */\
bool IsValid() const {\
  return flags_.OrBits(SHM_CONTAINER_VALID);\
}\
\
/** Set container header invalid */\
void UnsetValid() {\
  flags_.UnsetBits(SHM_CONTAINER_VALID | SHM_CONTAINER_DESTRUCTABLE);\
}\
\
/** Check if null */\
bool IsNull() const {\
  return !IsValid() || !IsDataValid();\
}\
\
/** Get a typed pointer to the object */\
template<typename POINTER_T>\
POINTER_T GetShmPointer() const {\
  return alloc_->Convert<TYPE_UNWRAP(TYPED_HEADER), POINTER_T>(header_);\
}\

#endif  // LABSTOR_DATA_STRUCTURES_INTERNAL_SHM_CONTAINER_MACRO_H_
