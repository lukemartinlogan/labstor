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


#ifndef LABSTOR_DATA_STRUCTURES_SHM_CONSTRUCT_H_
#define LABSTOR_DATA_STRUCTURES_SHM_CONSTRUCT_H_

#include "shm_archive.h"
#include "shm_struct.h"
#include "shm_smart_ptr.h"

namespace labstor::ipc {

/** Constructs and archives an object */
template<typename T, typename ...Args>
static ShmArchive<T> make_shm_ar(Args&& ...args) {
  ShmArchive<T> ar;
  if constexpr(IS_SHM_SERIALIZEABLE(T)) {
    T obj;
    obj.shm_init_main(std::forward<Args>(args)...);
    if constexpr(!IS_SHM_SMART_POINTER(T)) {
      obj.UnsetDestructable();
    }
    obj >> ar;
  } else {
    ShmStruct<T> obj;
    obj.shm_init(std::forward<Args>(args)...);
    obj >> ar;
  }
  return ar;
}

}  // namespace labstor::ipc

#endif  // LABSTOR_DATA_STRUCTURES_SHM_CONSTRUCT_H_
