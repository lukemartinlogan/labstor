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
  virtual std::vector<DataTransfer> SaveStart(u32 method, Task *task) {
    return {};
  }

  /** Deserialize a task when popping from remote queue */
  virtual void LoadStart(BinaryInputArchive<true> &ar, TaskPointer &task_ptr) {}

  /** Serialize a task when returning from remote queue */
  virtual std::vector<DataTransfer> SaveEnd(u32 method, Task *task) {
    return {};
  }

  /** Deserialize a task when returning from remote queue */
  virtual void LoadEnd(BinaryInputArchive<false> &ar, TaskPointer &task_ptr) {}
};

}  // namespace labstor

LABSTOR_TASK_CC(labstor::TASK_NAME::Server, "TASK_NAME");
