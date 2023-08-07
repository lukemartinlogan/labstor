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
  /*labstor::QueueId qid(0, 3);
  LABSTOR_ADMIN->CreateQueueRoot(labstor::DomainId::GetLocal(), qid,
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

  HILOG(kInfo, "Latency: {} MOps", ops / t.GetUsec());*/
}

/** Single-thread performance of allocating + freeing tasks */
TEST_CASE("TestHshmAllocateFree") {
  labstor::QueueId qid(0, 3);
  auto queue = hipc::make_uptr<labstor::MultiQueue>(
      qid, 16, 16, 256, hshm::bitfield32_t(0));


  hshm::Timer t;
  t.Resume();
  size_t ops = (1 << 20);
  size_t count = (1 << 8);
  size_t reps = ops / count;
  for (size_t i = 0; i < reps; ++i) {
    std::vector<labstor::Task*> tasks(count);
    for (size_t j = 0; j < count; ++j) {
      hipc::Pointer p;
      tasks[j] = LABSTOR_CLIENT->NewTaskRoot<labstor::Task>(p);
    }
    for (size_t j = 0; j < count; ++j) {
      LABSTOR_CLIENT->DelTask(tasks[j]);
    }
  }
  t.Pause();
  HILOG(kInfo, "Latency: {} MOps", ops / t.GetUsec());
}

/** Single-thread performance of emplacing, and popping a mpsc_ptr_queue */
TEST_CASE("TestPointerQueueEmplacePop") {
  size_t ops = (1 << 20);
  auto queue_ptr = hipc::make_uptr<hipc::mpsc_ptr_queue<hipc::Pointer>>(ops);
  auto queue = queue_ptr.get();
  hipc::Pointer p;

  hshm::Timer t;
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
  size_t ops = (1 << 20);
  auto queue = hipc::make_uptr<labstor::MultiQueue>(
      qid, 16, 16, ops, hshm::bitfield32_t(0));
  hipc::Pointer p;
  auto *task = LABSTOR_CLIENT->NewTaskRoot<labstor::Task>(p);

  hshm::Timer t;
  t.Resume();
  for (size_t i = 0; i < ops; ++i) {
    queue->Emplace(0, p);
    queue->Pop(0, task, p);
  }
  t.Pause();

  LABSTOR_CLIENT->DelTask(task);
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
    auto *task = LABSTOR_CLIENT->NewTaskRoot<labstor::Task>(p);
    queue->Emplace(0, p);
    queue->Pop(0, task, p);
    LABSTOR_CLIENT->DelTask(task);
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
  LABSTOR_ADMIN->RegisterTaskLibraryRoot(labstor::DomainId::GetLocal(), "small_message");
  client.CreateRoot(labstor::DomainId::GetLocal(), "ipc_test");
  hshm::Timer t;

  int pid = getpid();
  ProcessAffiner::SetCpuAffinity(pid, 8);

  t.Resume();
  size_t ops = (1 << 20);
  for (size_t i = 0; i < ops; ++i) {
    client.CustomRoot(labstor::DomainId::GetLocal());
  }
  t.Pause();

  HILOG(kInfo, "Latency: {} MOps", ops / t.GetUsec());
}

#include "hermes/hermes.h"
#include "hermes_adapters/filesystem.h"

/** Time to process a request */
TEST_CASE("TestHermesFsLatency") {
  HERMES->ClientInit();
  hshm::Timer t;

  int pid = getpid();
  ProcessAffiner::SetCpuAffinity(pid, 8);
  hermes::Bucket bkt = HERMES_FILESYSTEM_API->Open("/home/lukemartinlogan/hi.txt");

  t.Resume();
  size_t ops = (1 << 20);
  hermes::Context ctx;
  ctx.page_size_ = 4096;
  std::string data(ctx.page_size_, 0);
  for (size_t i = 0; i < ops; ++i) {
    HERMES_FILESYSTEM_API->Write(bkt, data.data(), 0, data.size(), false, ctx);
  }
  t.Pause();

  HILOG(kInfo, "Latency: {} MBps", ops * 4096 / t.GetUsec());
}