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


#include <iostream>
#include <pthread.h>
#include <memory>
#include <yaml-cpp/yaml.h>

#include <labstor/runtime/accept_unix.h>
#include <labstor/constants/singleton_macros.h>
#include <labstor/types/basic.h>
#include <labstor/util/errors.h>
#include <labstor/util/debug.h>

#include <labstor/runtime/configuration_manager.h>
#include "labstor/ipc_manager/ipc_manager.h"

int main(int argc, char **argv) {
  AUTO_TRACE("")
  if (argc != 2) {
    printf("USAGE: ./server [config.yaml]\n");
    exit(1);
  }

  LABSTOR_ERROR_HANDLE_START()
    // Initialize labstor configuration
    auto labstor_config_ = LABSTOR_CONFIGURATION_MANAGER;
    labstor_config_->LoadConfig(argv[1]);

    // Create the thread for accepting connections
    labstor::UnixAcceptDaemon accept_daemon(labstor::ThreadType::kPthread);
    accept_daemon.Start();
    printf("LabStor server has started\n");
    accept_daemon.Wait();
  LABSTOR_ERROR_HANDLE_END()
}
