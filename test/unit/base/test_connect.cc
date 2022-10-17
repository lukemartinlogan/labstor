//
// Created by lukemartinlogan on 10/16/22.
//

#include "labstor/constants/singleton_macros.h"
#include "labstor/ipc_manager/ipc_manager.h"

int main() {
  LABSTOR_IPC_MANAGER_T ipc_manager = LABSTOR_IPC_MANAGER;
  ipc_manager->Connect();
  printf("Connect did not fail!\n");
}