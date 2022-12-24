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


#ifndef LABSTOR_MEMORY_SHM_MACROS_H_
#define LABSTOR_MEMORY_SHM_MACROS_H_

#include <labstor/constants/macros.h>

/**
 * Determine whether or not \a T type is a SHM serializeable data structure
 * */

#define IS_SHM_SERIALIZEABLE(T) \
  std::is_base_of<labstor::ipc::ShmArchiveable, T>::value

/**
 * Determine whether or not \a T type is a SHM smart pointer
 * */

#define IS_SHM_SMART_POINTER(T) \
  std::is_base_of<labstor::ipc::ShmSmartPointer, T>::value

/**
 * SHM_X_OR_Y: X if T is SHM_SERIALIZEABLE, Y otherwise
 * */

#define SHM_X_OR_Y(T, X, Y) \
  typename std::conditional<         \
    IS_SHM_SERIALIZEABLE(T), \
    X, Y>::type

/**
 * SHM_T_OR_ARCHIVE: Determines the type of the internal pointer used
 * to store data in a shared-memory data structure. For example,
 * let's say there are two vectors: vector<int> V1 and vector<vector<int>> V2.
 * V1 should store internally a pointer int *vec_
 * V2 should store internally a pointer ShmArchive<vector<int>> *vec_ and
 * then deserialize this pointer at every index operation.
 * */

#define SHM_T_OR_ARCHIVE(T) \
  SHM_X_OR_Y(T, ShmArchive<T>, T)

/**
 * SHM_T_OR_REF_T: Determines the return value of an index operation on
 * a shared-memory data structure. For example, let's say there
 * are two vectors: vector<int> V1 and vector<vector<int>> V2.
 * V1[0] should return an int&
 * V2[1] should return a vector<int> (not &)
 * This is because V2 needs to shm_deserialize vector<int> from shared
 * memory.
 *
 * @T: The type being stored in the shmem data structure
 * */

#define SHM_T_OR_REF_T(T) \
  SHM_X_OR_Y(T, T, T&)
/**
 * SHM_T_OR_PTR_T: Used by shm_ref.
 *
 * @T: The type being stored in the shmem data structure
 * */

#define SHM_T_OR_PTR_T(T) \
  SHM_X_OR_Y(T, T, T*)

/**
 * SHM_T_OR_PTR_T: Used by unique_ptr and shared_ptr to determine how to
 * best store the underlying object.
 *
 * @T: The type being stored in the shmem data structure
 * */

#define SHM_T_OR_SHM_PTR_T(T) \
  SHM_X_OR_Y(T, T, ShmStruct<T>)

/**
 * SHM_T_OR_CONST_T: Determines whether or not an object should be
 * a constant or not.
 * */

#define SHM_CONST_T_OR_T(T, IS_CONST) \
  typename std::conditional<         \
    IS_CONST, \
    const T, T>::type

#endif  // LABSTOR_MEMORY_SHM_MACROS_H_
