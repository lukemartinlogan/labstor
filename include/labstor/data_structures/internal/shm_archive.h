//
// Created by lukemartinlogan on 12/16/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_SHM_ARCHIVE_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_SHM_ARCHIVE_H_

#include "labstor/memory/memory_manager.h"
#include "shm_macros.h"
#include "shm_serialize.h"

namespace labstor::ipc {

/**
 * A wrapper around a process-independent pointer for storing
 * a single complex shared-memory data structure
 * */
template<typename T>
struct ShmArchive {
 public:
  Pointer header_ptr_;

  /** Default constructor */
  ShmArchive() = default;

  /** Get the process-independent pointer */
  inline Pointer& Get() {
    return header_ptr_;
  }

  /** Get the process-independent pointer */
  inline const Pointer& GetConst() {
    return header_ptr_;
  }

  /** Creates a ShmArchive from a header pointer */
  explicit ShmArchive(Pointer &ptr)
    : header_ptr_(ptr) {
  }

  /** Creates a ShmArchive from a header pointer */
  explicit ShmArchive(const Pointer &ptr)
    : header_ptr_(ptr) {
  }

  /** Copies a ShmArchive into another */
  ShmArchive(const ShmArchive &other)
    : header_ptr_(other.header_ptr_) {
  }

  /** Moves the data from one archive into another */
  ShmArchive(ShmArchive&& source) noexcept
    : header_ptr_(source.header_ptr_) {
    source.header_ptr_.set_null();
  }
};

}

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_SHM_ARCHIVE_H_
