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


#ifndef LABSTOR_DATA_STRUCTURES_INTERNAL_SHM_DATA_STRUCTURE_H_
#define LABSTOR_DATA_STRUCTURES_INTERNAL_SHM_DATA_STRUCTURE_H_

#include "labstor/memory/memory.h"
#include "labstor/memory/allocator/allocator.h"
#include "labstor/memory/memory_manager.h"
#include <labstor/constants/data_structure_singleton_macros.h>

#include "labstor/data_structures/internal/shm_macros.h"
#include "labstor/data_structures/internal/shm_archive.h"
#include "labstor/data_structures/internal/shm_data_structure.h"
#include "labstor/data_structures/internal/shm_construct.h"

namespace labstor::ipc {

/**
 * The general base class of a shared-memory data structure
 *
 * There are no virtual functions, but all base classes must
 * implement certain methods which are indicated below in the
 * section "REQUIRED METHODS"
 * */
template<typename TYPED_CLASS, typename TYPED_HEADER>
class ShmContainer : public ShmDataStructure<TYPED_HEADER> {
 public:
  SHM_DATA_STRUCTURE_TEMPLATE(TYPED_HEADER)
  typedef TYPED_HEADER header_t;

 public:
  /** Default constructor */
  ShmContainer() = default;

 public:
  ////////////////////////////////
  ////////REQUIRED METHODS
  ///////////////////////////////

  /** Copy constructor */
  // void StrongCopy(const CLASS_NAME &other);
};

}  // namespace labstor::ipc

/**
 * Define various functions and variables common across all
 * SharedMemoryDataStructures.
 *
 * Variables which derived classes should see are not by default visible
 * due to the nature of c++ template resolution.
 *
 * 1. Create Move constructors + Move assignment operators.
 * 2. Create Copy constructors + Copy assignment operators.
 * 3. Create shm_serialize and shm_deserialize for archiving data structures.
 * */
#define SHM_CONTAINER_TEMPLATE(CLASS_NAME, TYPED_CLASS, TYPED_HEADER)\
  SHM_DATA_STRUCTURE_TEMPLATE(TYPED_HEADER)\
  SHM_INHERIT_MOVE_OPS(CLASS_NAME)\
  SHM_INHERIT_COPY_OPS(CLASS_NAME)\
  SHM_SERIALIZE_DESERIALIZE_WRAPPER(TYPED_CLASS)

/**
 * ShmContainers should define:
 * CLASS_NAME, TYPED_CLASS, and TYPED_HEADER macros and then
 * unset them in their respective header files.
 * */

#define BASIC_SHM_CONTAINER_TEMPLATE \
  SHM_CONTAINER_TEMPLATE(CLASS_NAME, \
    TYPE_WRAP(TYPED_CLASS), TYPE_WRAP(TYPED_HEADER))

#endif  // LABSTOR_DATA_STRUCTURES_INTERNAL_SHM_DATA_STRUCTURE_H_
