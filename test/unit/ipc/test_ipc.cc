//
// Created by llogan on 7/1/23.
//

#include "basic_test.h"
#include "labstor/api/labstor_client.h"
#include "labstor_admin/labstor_admin.h"

#include "small_message/small_message.h"
#include "hermes_shm/util/timer.h"
#include "labstor/work_orchestrator/affinity.h"

TEST_CASE("TestIpc") {
  labstor::small_message::Client client;
  LABSTOR_ADMIN->RegisterTaskLibraryRoot(labstor::DomainId::GetLocal(), "small_message");
  client.CreateRoot(labstor::DomainId::GetLocal(), "ipc_test");
  hshm::Timer t;

  int pid = getpid();
  ProcessAffiner::SetCpuAffinity(pid, 8);

  t.Resume();
  size_t ops = 1;
  for (size_t i = 0; i < ops; ++i) {
    int ret;
    HILOG(kInfo, "Sending message {}", i);
    // client.MdRoot(labstor::DomainId::GetNode(1));
    ret = client.IoRoot(labstor::DomainId::GetNode(1));
    // REQUIRE(ret == 1);
  }
  t.Pause();

  HILOG(kInfo, "Latency: {} MOps", ops / t.GetUsec());
}