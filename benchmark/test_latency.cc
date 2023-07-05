//
// Created by llogan on 7/1/23.
//

#include "basic_test.h"
#include "labstor/api/labstor_client.h"
#include "labstor_admin/labstor_admin.h"
#include "small_message/small_message.h"
#include "hermes_shm/util/timer.h"

TEST_CASE("TestLatency") {
  labstor::small_message::Client client;
  LABSTOR_ADMIN->RegisterTaskLibrary(0, "small_message");
  client.Create("ipc_test", 0);
  hshm::Timer t;

  t.Resume();
  size_t ops = 1024;
  for (size_t i = 0; i < ops; ++i) {
    int ret = client.Custom(0);
    REQUIRE(ret == i);
  }
  t.Pause();

  HILOG(kInfo, "Latency: {}", ops / t.GetUsec());
}