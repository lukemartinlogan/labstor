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
    task->ret_ = 1;
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
#include "small_message/small_message_lib_exec.h"
};

}  // namespace labstor

LABSTOR_TASK_CC(labstor::small_message::Server, "small_message");
