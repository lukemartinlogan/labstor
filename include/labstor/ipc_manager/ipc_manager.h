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

#ifndef LABSTOR_IPC_MANAGER_H
#define LABSTOR_IPC_MANAGER_H

#include <labstor/types/basic.h>

#include <memory>
#include <unordered_map>
#include "communicator.h"

namespace labstor {

struct setup_reply {
};

struct ClientInfo {
  std::unique_ptr<Communicator> comm_;
  // std::unique_ptr<Memory> shmem_;
};

class IpcManager {
private:
  std::unique_ptr<Communicator> comm_;
  std::unordered_map<pid_t, ClientInfo> clients_;
public:
  void Connect();
  void Register(std::unique_ptr<Communicator> &comm, UserCredentials creds);
};

}

#endif //LABSTOR_IPC_MANAGER_H
