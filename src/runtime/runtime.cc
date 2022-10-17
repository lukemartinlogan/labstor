//
// Created by lukemartinlogan on 8/4/21.
//

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
  if(argc != 2) {
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
