//
// Created by lukemartinlogan on 6/29/23.
//

#include "labstor_admin/labstor_admin.h"
#include "labstor/api/labstor_runtime.h"
#include "TASK_NAME/TASK_NAME.h"

namespace labstor::TASK_NAME {

class Server : public TaskLib {
 public:
  Server() = default;

  void Construct(MultiQueue *queue, ConstructTask *task) {
    id_ = task->id_;
    task->SetComplete();
  }

  void Destruct(MultiQueue *queue, DestructTask *task) {
    task->SetComplete();
  }

  void Custom(MultiQueue *queue, CustomTask *task) {
    task->SetComplete();
  }

 public:
  /** Execute a task */
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
      case Method::kCustom: {
        Custom(queue, reinterpret_cast<CustomTask *>(task));
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
      case Method::kCustom: {
        ar << *reinterpret_cast<CustomTask*>(task);
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
      case Method::kCustom: {
        task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<CustomTask>(task_ptr.p_);
        ar >> *reinterpret_cast<CustomTask*>(task_ptr.task_);
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
      case Method::kCustom: {
        ar << *reinterpret_cast<CustomTask*>(task);
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
      case Method::kCustom: {
        ar >> *reinterpret_cast<CustomTask*>(task);
        break;
      }
    }
  }
};

}  // namespace labstor

LABSTOR_TASK_CC(labstor::TASK_NAME::Server, "TASK_NAME");
