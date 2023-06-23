//
// Created by lukemartinlogan on 6/17/23.
//

#include "labstor/labstor.h"

int main(int argc, char **argv) {
  LABSTOR->Create(labstor::LabstorMode::kServer);
  LABSTOR->RunDaemon();
  LABSTOR->Finalize();
  return 0;
}