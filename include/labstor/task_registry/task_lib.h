//
// Created by lukemartinlogan on 6/23/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_TASK_TASK_H_
#define LABSTOR_INCLUDE_LABSTOR_TASK_TASK_H_

#include <dlfcn.h>
#include "labstor/labstor_types.h"
#include "labstor/queue_manager/queue_factory.h"
#include "task.h"

namespace labstor {

/**
 * Represents a custom operation to perform.
 * Tasks are independent of Hermes.
 * */
class TaskLib {
 public:
  TaskStateId id_;    /**< The unique name of a task_templ executor */
  std::string name_; /**< The unique semantic name of a task_templ executor */

  /** Default constructor */
  TaskLib() : id_(TaskStateId::GetNull()) {}

  /** Emplace Constructor */
  void Init(TaskStateId id, const std::string &name) {
    id_ = id;
    name_ = name;
  }

  /** Virtual destructor */
  virtual ~TaskLib() = default;

  /** Run a method of the task_templ */
  virtual void Run(MultiQueue *queue, u32 method, Task *task) = 0;
};

/** Represents a TaskLib in action */
typedef TaskLib TaskState;

extern "C" {
/** The two methods provided by all tasks */
typedef TaskState* (*create_executor_t)(Task *task);
/** Get the name of a task_templ */
typedef const char* (*get_task_lib_name_t)(void);
}  // extern c

/** Used internally by task_templ source file */
#define LABSTOR_TASK_CC(TRAIT_CLASS, TASK_NAME) \
    extern "C" {                              \
        void* create_executor(labstor::Task *task) { \
          labstor::TaskState *exec = new TYPE_UNWRAP(TRAIT_CLASS)(); \
          exec->Run(nullptr, labstor::TaskMethod::kConstruct, task); \
          return exec; \
        } \
        const char* get_task_lib_name(void) { return TASK_NAME; } \
        bool is_labstor_task_ = true; \
    }
}   // namespace labstor

#endif  // LABSTOR_INCLUDE_LABSTOR_TASK_TASK_H_
