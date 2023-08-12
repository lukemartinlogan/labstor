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

  void Custom(MultiQueue *queue, CustomTask *task) {
    task->ret_ = count_++;
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

LABSTOR_TASK_CC(labstor::small_message::Server, "small_message");
