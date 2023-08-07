//
// Created by lukemartinlogan on 6/23/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_TASK_TASK_H_
#define LABSTOR_INCLUDE_LABSTOR_TASK_TASK_H_

#include <dlfcn.h>
#include "labstor/labstor_types.h"
#include "labstor/queue_manager/queue_factory.h"
#include "labstor/network/serialize.h"
#include "task.h"

namespace labstor {

/**
 * Represents a custom operation to perform.
 * Tasks are independent of Hermes.
 * */
class TaskLib {
 public:
  TaskStateId id_;    /**< The unique name of a task state */
  std::string name_; /**< The unique semantic name of a task state */

  /** Default constructor */
  TaskLib() : id_(TaskStateId::GetNull()) {}

  /** Emplace Constructor */
  void Init(TaskStateId id, const std::string &name) {
    id_ = id;
    name_ = name;
  }

  /** Virtual destructor */
  virtual ~TaskLib() = default;

  /** Run a method of the task */
  virtual void Run(MultiQueue *queue, u32 method, Task *task) = 0;

  /** Serialize a task */
  virtual std::vector<DataTransfer> Serialize(u32 method, Task *task) {
    return {};
  }

  /** Deserialize a task */
  virtual Task* Deserialize(const std::vector<DataTransfer> &xfer) {
    return nullptr;
  }
};

/** Represents a TaskLib in action */
typedef TaskLib TaskState;

extern "C" {
/** The two methods provided by all tasks */
typedef TaskState* (*create_state_t)(Task *task);
/** Get the name of a task */
typedef const char* (*get_task_lib_name_t)(void);
}  // extern c

/** Used internally by task source file */
#define LABSTOR_TASK_CC(TRAIT_CLASS, TASK_NAME) \
    extern "C" {                              \
        void* create_state(labstor::Task *task) { \
          labstor::TaskState *exec = reinterpret_cast<labstor::TaskState*>( \
            new TYPE_UNWRAP(TRAIT_CLASS)()); \
          exec->Run(nullptr, labstor::TaskMethod::kConstruct, task); \
          return exec; \
        } \
        const char* get_task_lib_name(void) { return TASK_NAME; } \
        bool is_labstor_task_ = true; \
    }
}   // namespace labstor

#endif  // LABSTOR_INCLUDE_LABSTOR_TASK_TASK_H_
