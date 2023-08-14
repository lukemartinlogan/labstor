//
// Created by lukemartinlogan on 6/29/23.
//

#include "labstor_admin/labstor_admin.h"
#include "labstor/api/labstor_runtime.h"
#include "small_message/small_message.h"

namespace labstor::small_message {

class Server : public TaskLib {
 public:
  int count_ = 0;

 public:
  void Construct(MultiQueue *queue, ConstructTask *task) {
    task->SetComplete();
  }

  void Destruct(MultiQueue *queue, DestructTask *task) {
    task->SetComplete();
  }

  void Md(MultiQueue *queue, MdTask *task) {
    task->ret_ = count_++;
    task->SetComplete();
  }

  void Io(MultiQueue *queue, IoTask *task) {
    task->ret_ = 1;
    for (int i = 0; i < 256; ++i) {
      if (task->data_[i] != 10) {
        task->ret_ = 0;
        break;
      }
    }
    task->SetComplete();
  }

 public:
  /** Run a method of the queue */
  void Run(MultiQueue *queue, u32 method, Task *task) override {
    switch (method) {
      case Method::kConstruct: {
        Construct(queue, reinterpret_cast<ConstructTask *>(task));
        break;
      }
      case Method::kDestruct: {
        Destruct(queue, reinterpret_cast<DestructTask *>(task));
        break;
      }
      case Method::kMd: {
        Md(queue, reinterpret_cast<MdTask *>(task));
        break;
      }
      case Method::kIo: {
        Io(queue, reinterpret_cast<IoTask *>(task));
        break;
      }
    }
  }

  /** Serialize a task when initially pushing into remote */
  std::vector<DataTransfer> SaveStart(u32 method, BinaryOutputArchive<true> &ar, Task *task) override {
    switch (method) {
      case Method::kConstruct: {
        ar << *reinterpret_cast<ConstructTask*>(task);
        break;
      }
      case Method::kDestruct: {
        ar << *reinterpret_cast<DestructTask*>(task);
        break;
      }
      case Method::kMd: {
        ar << *reinterpret_cast<MdTask*>(task);
        break;
      }
      case Method::kIo: {
        ar << *reinterpret_cast<IoTask*>(task);
        break;
      }
    }
    return ar.Get();
  }

  /** Deserialize a task when popping from remote queue */
  TaskPointer LoadStart(u32 method, BinaryInputArchive<true> &ar) override {
    TaskPointer task_ptr;
    switch (method) {
      case Method::kConstruct: {
        task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<ConstructTask>(task_ptr.p_);
        ar >> *reinterpret_cast<ConstructTask*>(task_ptr.task_);
        break;
      }
      case Method::kDestruct: {
        task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<DestructTask>(task_ptr.p_);
        ar >> *reinterpret_cast<DestructTask*>(task_ptr.task_);
        break;
      }
      case Method::kMd: {
        task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<MdTask>(task_ptr.p_);
        ar >> *reinterpret_cast<MdTask*>(task_ptr.task_);
        break;
      }
      case Method::kIo: {
        task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<IoTask>(task_ptr.p_);
        ar >> *reinterpret_cast<IoTask*>(task_ptr.task_);
        break;
      }
    }
    return task_ptr;
  }

  /** Serialize a task when returning from remote queue */
  std::vector<DataTransfer> SaveEnd(u32 method, BinaryOutputArchive<false> &ar, Task *task) override {
    switch (method) {
      case Method::kConstruct: {
        ar << *reinterpret_cast<ConstructTask*>(task);
        break;
      }
      case Method::kDestruct: {
        ar << *reinterpret_cast<DestructTask*>(task);
        break;
      }
      case Method::kMd: {
        ar << *reinterpret_cast<MdTask*>(task);
        break;
      }
      case Method::kIo: {
        ar << *reinterpret_cast<IoTask*>(task);
        break;
      }
    }
    return ar.Get();
  }

  /** Deserialize a task when returning from remote queue */
  void LoadEnd(u32 method, BinaryInputArchive<false> &ar, Task *task) override {
    switch (method) {
      case Method::kConstruct: {
        ar >> *reinterpret_cast<ConstructTask*>(task);
        break;
      }
      case Method::kDestruct: {
        ar >> *reinterpret_cast<DestructTask*>(task);
        break;
      }
      case Method::kMd: {
        ar >> *reinterpret_cast<MdTask*>(task);
        break;
      }
      case Method::kIo: {
        ar >> *reinterpret_cast<IoTask*>(task);
        break;
      }
    }
  }
};

}  // namespace labstor

LABSTOR_TASK_CC(labstor::small_message::Server, "small_message");
