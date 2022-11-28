//
// Created by lukemartinlogan on 11/6/22.
//

#include "basic_test.h"
#include <mpi.h>
#include "test_init.h"

int main(int argc, char **argv) {
  int rc;
  MPI_Init(&argc, &argv);
  Catch::Session session;
  auto cli = session.cli();
  session.cli(cli);
  rc = session.applyCommandLine(argc, argv);
  if (rc != 0) return rc;
  Pretest(AllocatorType::kPageAllocator);
  int test_return_code = session.run();
  Posttest();
  if (rc != 0) return rc;
  MPI_Finalize();
  return test_return_code;
}