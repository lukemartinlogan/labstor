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


#ifndef LABSTOR_INCLUDE_LABSTOR_THREAD_THREAD_FACTORY_H_
#define LABSTOR_INCLUDE_LABSTOR_THREAD_THREAD_FACTORY_H_

#include "thread.h"
#include "pthread.h"

namespace labstor {

template<typename BIND>
class ThreadFactory {
 private:
  ThreadType type_;
  BIND bind_;

 public:
  ThreadFactory(ThreadType type, BIND bind) : type_(type), bind_(bind) {}
  std::unique_ptr<Thread> Get() {
    switch(type_) {
      case ThreadType::kPthread: return std::make_unique<Pthread<BIND>>(bind_);
      default: return nullptr;
    }
  }
};

}

#endif //LABSTOR_INCLUDE_LABSTOR_THREAD_THREAD_FACTORY_H_
