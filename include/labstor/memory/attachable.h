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


#ifndef LABSTOR_INCLUDE_LABSTOR_MEMORY_ATTACHABLE_H_
#define LABSTOR_INCLUDE_LABSTOR_MEMORY_ATTACHABLE_H_

#include <string>

namespace labstor::memory {

class Attachable {
 public:
  virtual bool Create() = 0;
  virtual bool Attach() = 0;
  virtual void Detach() = 0;
  void Destroy() {
    Detach();
    _Destroy();
  }

 protected:
  virtual void _Destroy() = 0;
};

class BufferAttachable {
 public:
  virtual bool Create(void *buffer, size_t size) = 0;
  virtual bool Attach(void *buffer) = 0;
};

}  // namespace labstor::memory

#endif  // LABSTOR_INCLUDE_LABSTOR_MEMORY_ATTACHABLE_H_
