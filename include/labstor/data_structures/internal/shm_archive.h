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


#ifndef LABSTOR_DATA_STRUCTURES_SHM_ARCHIVE_H_
#define LABSTOR_DATA_STRUCTURES_SHM_ARCHIVE_H_

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
  ShmArchive(ShmArchive&& other) noexcept
    : header_ptr_(other.header_ptr_) {
    other.header_ptr_.set_null();
  }
};

}  // namespace labstor::ipc

#endif  // LABSTOR_DATA_STRUCTURES_SHM_ARCHIVE_H_
