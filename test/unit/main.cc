//
// Created by lukemartinlogan on 11/6/22.
//

#include "basic_test.h"
#include <mpi.h>

int main(int argc, char **argv) {
  int rc;
  Catch::Session session;
  auto cli = session.cli();
  session.cli(cli);
  rc = session.applyCommandLine(argc, argv);
  if (rc != 0) return rc;
  MainPretest();
  rc = session.run();
  MainPosttest();
  if (rc != 0) return rc;
  return rc;
}