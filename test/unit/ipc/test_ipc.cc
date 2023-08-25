//
// Created by llogan on 7/1/23.
//

#include "basic_test.h"
#include <mpi.h>
#include "labstor/api/labstor_client.h"
#include "labstor_admin/labstor_admin.h"

#include "small_message/small_message.h"
#include "hermes_shm/util/timer.h"
#include "labstor/work_orchestrator/affinity.h"

TEST_CASE("TestIpc") {
  int rank, nprocs;
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  labstor::small_message::Client client;
  if (rank == 0) {
    LABSTOR_ADMIN->RegisterTaskLibraryRoot(labstor::DomainId::GetGlobal(), "small_message");
    client.CreateRoot(labstor::DomainId::GetGlobal(), "ipc_test");
  }
  MPI_Barrier(MPI_COMM_WORLD);
  hshm::Timer t;

  int pid = getpid();
  ProcessAffiner::SetCpuAffinity(pid, 8);

  t.Resume();
  size_t ops = 16;
  for (size_t i = 0; i < ops; ++i) {
    int ret;
    HILOG(kInfo, "Sending message {}", i);
    int node_id = 1 + ((rank + 1) % nprocs);
    ret = client.MdRoot(labstor::DomainId::GetNode(node_id));
    REQUIRE(ret == 1);
  }
  t.Pause();

  HILOG(kInfo, "Latency: {} MOps", ops / t.GetUsec());
}

TEST_CASE("TestIO") {
  int rank, nprocs;
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  labstor::small_message::Client client;
  if (rank == 0) {
    LABSTOR_ADMIN->RegisterTaskLibraryRoot(labstor::DomainId::GetGlobal(), "small_message");
    client.CreateRoot(labstor::DomainId::GetGlobal(), "ipc_test");
  }
  MPI_Barrier(MPI_COMM_WORLD);
  hshm::Timer t;

  int pid = getpid();
  ProcessAffiner::SetCpuAffinity(pid, 8);

  t.Resume();
  size_t ops = 16;
  for (size_t i = 0; i < ops; ++i) {
    int ret;
    HILOG(kInfo, "Sending message {}", i);
    int node_id = 1 + ((rank + 1) % nprocs);
    ret = client.MdRoot(labstor::DomainId::GetNode(node_id));
    REQUIRE(ret == 1);
  }
  t.Pause();

  HILOG(kInfo, "Latency: {} MOps", ops / t.GetUsec());
}