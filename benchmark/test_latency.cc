//
// Created by llogan on 7/1/23.
//

#include "basic_test.h"
#include "labstor/api/labstor_client.h"
#include "labstor_admin/labstor_admin.h"
#include "small_message/small_message.h"
#include "hermes_shm/util/timer.h"
#include "labstor/work_orchestrator/affinity.h"
#include <zmq.hpp>

/** The performance of getting a queue */
TEST_CASE("TestGetQueue") {
  labstor::QueueId qid(0, 3);
  LABSTOR_ADMIN->CreateQueue(0, qid,
                             16, 16, 256,
                             hshm::bitfield32_t(0));
  LABSTOR_QM_CLIENT->GetQueue(qid);

  hshm::Timer t;
  t.Resume();
  size_t ops = (1 << 20);
  for (size_t i = 0; i < ops; ++i) {
    labstor::MultiQueue *queue = LABSTOR_QM_CLIENT->GetQueue(qid);
    REQUIRE(queue->id_ == qid);
  }
  t.Pause();

  HILOG(kInfo, "Latency: {} MOps", ops / t.GetUsec());
}

/** Single-thread performance of allocating + freeing tasks */
TEST_CASE("TestHshmAllocateFree") {
  labstor::QueueId qid(0, 3);
  auto queue = hipc::make_uptr<labstor::MultiQueue>(
      qid, 16, 16, 256, hshm::bitfield32_t(0));


  hshm::Timer t;
  t.Resume();
  size_t ops = (1 << 20);
  for (size_t i = 0; i < ops; ++i) {
    hipc::Pointer p;
    queue->Allocate<labstor::Task>(LABSTOR_CLIENT->main_alloc_, p);
    queue->Free(LABSTOR_CLIENT->main_alloc_, p);
  }
  t.Pause();
  HILOG(kInfo, "Latency: {} MOps", ops / t.GetUsec());
}

/** Single-thread performance of emplacing, and popping a mpsc_ptr_queue */
TEST_CASE("TestPointerQueueEmplacePop") {
  auto queue_ptr = hipc::make_uptr<hipc::mpsc_ptr_queue<hipc::Pointer>>(256);
  auto queue = queue_ptr.get();
  hipc::Pointer p;

  hshm::Timer t;
  size_t ops = (1 << 20);
  t.Resume();
  for (size_t i = 0; i < ops; ++i) {
    queue->emplace(p);
    queue->pop(p);
  }
  t.Pause();

  HILOG(kInfo, "Latency: {} MOps", ops / t.GetUsec());
}

/** Single-thread performance of empacling + popping vec<mpsc_ptr_queue> */
TEST_CASE("TestPointerQueueVecEmplacePop") {
  auto queues_ptr = hipc::make_uptr<hipc::vector<hipc::mpsc_ptr_queue<hipc::Pointer>>>(16);
  auto queues = queues_ptr.get();
  hipc::Pointer p;

  hshm::Timer t;
  size_t ops = (1 << 20);
  for (size_t i = 0; i < ops; ++i) {
    t.Resume();
    auto &queue = (*queues)[0];
    queue.emplace(p);
    queue.pop(p);
    t.Pause();
  }

  HILOG(kInfo, "Latency: {} MOps", ops / t.GetUsec());
}

/** Single-thread performance of getting, emplacing, and popping a queue */
TEST_CASE("TestHshmQueueEmplacePop") {
  labstor::QueueId qid(0, 3);
  auto queue = hipc::make_uptr<labstor::MultiQueue>(
      qid, 16, 16, 256, hshm::bitfield32_t(0));
  hipc::Pointer p;
  auto *task = queue->Allocate<labstor::Task>(LABSTOR_CLIENT->main_alloc_, p);

  hshm::Timer t;
  t.Resume();
  size_t ops = (1 << 20);
  for (size_t i = 0; i < ops; ++i) {
    queue->Emplace(0, p);
    queue->Pop(0, task, p);
  }
  t.Pause();

  queue->Free(LABSTOR_CLIENT->main_alloc_, p);
  HILOG(kInfo, "Latency: {} MOps", ops / t.GetUsec());
}

/** Single-thread performance of getting a lane from a queue */
TEST_CASE("TestHshmQueueGetLane") {
  labstor::QueueId qid(0, 3);
  auto queue = hipc::make_uptr<labstor::MultiQueue>(
      qid, 16, 16, 256, hshm::bitfield32_t(0));

  hshm::Timer t;
  size_t ops = (1 << 20);
  t.Resume();
  for (size_t i = 0; i < ops; ++i) {
    queue->GetLane(i % queue->num_lanes_);
  }
  t.Pause();

  HILOG(kInfo, "Latency: {} MOps", ops / t.GetUsec());
}

/** Single-thread performance of getting, emplacing, and popping a queue */
TEST_CASE("TestHshmQueueAllocateEmplacePop") {
  labstor::QueueId qid(0, 3);
  auto queue = hipc::make_uptr<labstor::MultiQueue>(
      qid, 16, 16, 256, hshm::bitfield32_t(0));

  hshm::Timer t;
  size_t ops = (1 << 20);
  t.Resume();
  for (size_t i = 0; i < ops; ++i) {
    hipc::Pointer p;
    auto *task = queue->Allocate<labstor::Task>(LABSTOR_CLIENT->main_alloc_, p);
    queue->Emplace(0, p);
    queue->Pop(0, task, p);
    queue->Free(LABSTOR_CLIENT->main_alloc_, p);
  }
  t.Pause();

  HILOG(kInfo, "Latency: {} MOps", ops / t.GetUsec());
}

/** ZeroMQ allocate + free request */
TEST_CASE("TestZeromqAllocateFree") {
  zmq::context_t context(1);

  // Create a PUSH socket to push requests
  zmq::socket_t pushSocket(context, ZMQ_PUSH);

  // Bind the PUSH socket to a specific address
  pushSocket.bind("ipc:///tmp/shared_memory");

  // Create a PULL socket to receive requests
  zmq::socket_t pullSocket(context, ZMQ_PULL);

  // Connect the PULL socket to the same address
  pullSocket.connect("ipc:///tmp/shared_memory");

  hshm::Timer t;
  t.Resume();
  size_t ops = (1 << 20);
  for (size_t i = 0; i < ops; ++i) {
    zmq::message_t message(sizeof(labstor::Task));
  }
  t.Pause();

  HILOG(kInfo, "Latency: {} MOps", ops / t.GetUsec());
}

/** Single-thread performance of zeromq */
TEST_CASE("TestZeromqAllocateEmplacePop") {
  zmq::context_t context(1);

  // Create a PUSH socket to push requests
  zmq::socket_t pushSocket(context, ZMQ_PUSH);

  // Bind the PUSH socket to a specific address
  pushSocket.bind("ipc:///tmp/shared_memory");

  // Create a PULL socket to receive requests
  zmq::socket_t pullSocket(context, ZMQ_PULL);

  // Connect the PULL socket to the same address
  pullSocket.connect("ipc:///tmp/shared_memory");

  hshm::Timer t;
  t.Resume();
  size_t ops = (1 << 20);
  for (size_t i = 0; i < ops; ++i) {
    // Send a request
    zmq::message_t message(sizeof(labstor::Task));
    pushSocket.send(message, zmq::send_flags::none);

    // Receive the request
    zmq::message_t receivedMessage;
    zmq::recv_result_t result = pullSocket.recv(receivedMessage);
    REQUIRE(receivedMessage.size() == sizeof(labstor::Task));
  }
  t.Pause();

  HILOG(kInfo, "Latency: {} MOps", ops / t.GetUsec());
}

/** Time to process a request */
TEST_CASE("TestRoundTripLatency") {
  labstor::small_message::Client client;
  LABSTOR_ADMIN->RegisterTaskLibrary(0, "small_message");
  client.Create("ipc_test", 0);
  hshm::Timer t;

  int pid = getpid();
  ProcessAffiner::SetCpuAffinity(pid, 8);

  t.Resume();
  size_t ops = (1 << 20);
  for (size_t i = 0; i < ops; ++i) {
    client.Custom(0);
  }
  t.Pause();

  HILOG(kInfo, "Latency: {} MOps", ops / t.GetUsec());
}