/*
 * Copyright (C) 2022  SCS Lab <scslab@iit.edu>,
 * Luke Logan <llogan@hawk.iit.edu>,
 * Jaime Cernuda Garcia <jcernudagarcia@hawk.iit.edu>
 * Jay Lofstead <gflofst@sandia.gov>,
 * Anthony Kougkas <akougkas@iit.edu>,
 * Xian-He Sun <sun@iit.edu>
 *
 * This file is part of LabStor
 *
 * LabStor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */


#ifndef LABSTOR_INCLUDE_LABSTOR_MEMORY_ALLOCATOR_ALLOCATOR_H_
#define LABSTOR_INCLUDE_LABSTOR_MEMORY_ALLOCATOR_ALLOCATOR_H_

#include <cstdint>
#include <labstor/memory/backend/memory_backend_factory.h>
#include <labstor/memory/memory.h>

namespace labstor::ipc {

enum class AllocatorType {
  kPageAllocator,
  kFixedFragmentationAllocator
};

struct AllocatorHeader {
  int allocator_type_;
  allocator_id_t allocator_id_;
  size_t custom_header_size_;

  AllocatorHeader() = default;

  // NOTE(llogan): allocator_id_ is set after construction
  explicit AllocatorHeader(AllocatorType type,
                           size_t custom_header_size) :
    allocator_type_(static_cast<int>(type)),
    custom_header_size_(custom_header_size) {}
};

class Allocator {
 protected:
  slot_id_t slot_id_;
  MemoryBackend *backend_;
  MemorySlot &slot_;

 public:
  explicit Allocator(slot_id_t slot_id, MemoryBackend *backend)
    : slot_id_(slot_id), backend_(backend),
      slot_(backend_->GetSlot(slot_id)) {}

  virtual ~Allocator() = default;

  virtual void Create(allocator_id_t id) = 0;
  virtual void Attach() = 0;

  /**
   * Allocate a region of memory of \a size size
   * */
  virtual Pointer Allocate(size_t size) = 0;

  /**
   * Reallocate \a pointer to \a new_size new size
   *
   * Returns true if p was modified.
   * If p.is_null(), will internally call Allocate.
   * */
  inline bool Reallocate(Pointer &p, size_t new_size) {
    if (p.is_null()) {
      p = Allocate(new_size);
      return true;
    }
    return ReallocateNoNullCheck(p, new_size);
  }

  /**
   * Reallocate \a pointer to \a new_size new size
   *
   * Returns true if p was modified.
   * Assumes that p is not kNullPointer.
   * */

  virtual bool ReallocateNoNullCheck(Pointer &p, size_t new_size) = 0;

  /**
   * Free the memory pointed to by \a ptr Pointer
   * */
  virtual void Free(Pointer &ptr) = 0;

  /**
   * Get the size of the user-defined custom header
   * */
  virtual size_t GetInternalHeaderSize() = 0;

  /**
   * Get the allocator identifier
   * */
  virtual allocator_id_t GetId() = 0;

  /**
   * Get the amount of memory that was allocated, but not yet freed.
   * Useful for memory leak checks.
   * */
  virtual size_t GetCurrentlyAllocatedSize() = 0;

  slot_id_t GetSlotId() {
    return slot_id_;
  }

  template<typename T>
  T* AllocatePtr(size_t size) {
    Pointer p;
    return AllocatePtr<T>(size, p);
  }

  template<typename T>
  T* AllocatePtr(size_t size, Pointer &p) {
    p = Allocate(size);
    if (p.is_null()) { return nullptr; }
    return reinterpret_cast<T*>(slot_.ptr_ + p.off_);
  }

  template<typename T>
  T* AllocateObjs(size_t count) {
    Pointer p;
    return AllocateObjs<T>(count, p);
  }

  template<typename T>
  T* AllocateObjs(size_t count, Pointer &p) {
    return AllocatePtr<T>(count * sizeof(T), p);
  }

  template<
    typename T,
    typename T_Ar = SHM_T_OR_ARCHIVE(T),
    typename ...Args>
  T* AllocateConstructObjs(size_t count, Args ...args) {
    Pointer p;
    return AllocateConstructObjs<T>(count, p, args...);
  }

  template<
    typename T,
    typename T_Ar = SHM_T_OR_ARCHIVE(T),
    typename ...Args>
  T* AllocateConstructObjs(size_t count, Pointer &p, Args ...args) {
    T *ptr = AllocateObjs<T>(count, p);
    ConstructObjs<T, T_Ar>(ptr, 0, count, args...);
    return ptr;
  }

  template<typename T>
  T* ReallocatePtr(Pointer &p, size_t new_size, bool &modified) {
    modified = Reallocate(p, new_size);
    return Convert<T>(p);
  }

  template<typename T>
  T* ReallocatePtr(Pointer &p, size_t new_size) {
    Reallocate(p, new_size);
    return Convert<T>(p);
  }

  template<
    typename T,
    typename T_Ar = SHM_T_OR_ARCHIVE(T),
    typename ...Args>
  T_Ar* ReallocateConstructObjs(Pointer &p,
                                size_t old_count,
                                size_t new_count, Args ...args) {
    T_Ar *ptr = ReallocatePtr<T_Ar>(p, new_count*sizeof(T_Ar));
    ConstructObjs<T, T_Ar>(ptr, old_count, new_count, args...);
    return ptr;
  }

  template<typename T>
  void FreePtr(T *ptr) {
    Pointer p = Convert<T>(ptr);
    Free(p);
  }

  template<
    typename T,
    typename T_Ar = SHM_T_OR_ARCHIVE(T),
    typename ...Args>
  static void ConstructObjs(T_Ar *ptr,
                            size_t old_count,
                            size_t new_count, Args ...args) {
    for (size_t i = old_count; i < new_count; ++i) {
      ConstructObj<T, T_Ar>(*(ptr + i), args...);
    }
  }

  template<
    typename T,
    typename T_Ar = SHM_T_OR_ARCHIVE(T),
    typename ...Args>
  static void ConstructObj(T_Ar &obj_ar,
                           Args ...args) {
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      T obj(args...);
      obj >> obj_ar;
    } else {
      new(&obj_ar) T(args...);
    }
  }

  template<
    typename T,
    typename T_Ar = SHM_T_OR_ARCHIVE(T)>
  static void DestructObjs(T_Ar *ptr,
                           size_t count) {
    for (size_t i = 0; i < count; ++i) {
      DestructObj<T, T_Ar>((ptr + i));
    }
  }

  template<
    typename T,
    typename T_Ar = SHM_T_OR_ARCHIVE(T)>
  static void DestructObj(T_Ar &obj_ar) {
    if constexpr(IS_SHM_SERIALIZEABLE(T)) {
      T obj;
      obj << obj_ar;
      obj.shm_destroy();
    }
    obj_ar.~T_Ar();
  }

  template<typename HEAD>
  HEAD* GetCustomHeader() {
    return reinterpret_cast<HEAD*>(slot_.ptr_ + GetInternalHeaderSize());
  }

  template<typename T>
  T* Convert(Pointer &p) {
    if (p.is_null()) { return nullptr; }
    auto &slot = backend_->GetSlot(slot_id_);
    return reinterpret_cast<T*>(slot.ptr_ + p.off_);
  }

  template<typename T>
  Pointer Convert(T *ptr) {
    Pointer p;
    if (ptr == nullptr) { return kNullPointer; }
    p.off_ = reinterpret_cast<size_t>(ptr) -
             reinterpret_cast<size_t>(slot_.ptr_);
    p.allocator_id_ = GetId();
    return p;
  }

  template<typename T = void>
  bool ContainsPtr(T *ptr) {
    return  reinterpret_cast<size_t>(ptr) >=
            reinterpret_cast<size_t>(slot_.ptr_);
  }
};

}  // namespace labstor::ipc

#endif  // LABSTOR_INCLUDE_LABSTOR_MEMORY_ALLOCATOR_ALLOCATOR_H_
