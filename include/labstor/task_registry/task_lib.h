//
// Created by lukemartinlogan on 6/23/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_TASK_TASK_H_
#define LABSTOR_INCLUDE_LABSTOR_TASK_TASK_H_

#include <dlfcn.h>
#include "labstor/labstor_data_structures.h"
#include "task.h"

namespace labstor {

/**
 * Represents a custom operation to perform.
 * Tasks are independent of Hermes.
 * */
class TaskLib {
 public:
  TaskExecId id_;    /**< The unique name of a task executor */
  std::string name_; /**< The unique semantic name of a task executor */

  /** Default constructor */
  TaskLib() : id_(TaskExecId::GetNull()) {}

  /** Emplace Constructor */
  void Init(TaskExecId id, const std::string &name) {
    id_ = id;
    name_ = name;
  }

  /** Virtual destructor */
  virtual ~TaskLib() = default;

  /** Run a method of the task */
  virtual void Run(u32 method, Task *task) = 0;
};

/** Represents a TaskLib in action */
typedef TaskLib TaskExecutor;

extern "C" {
/** The two methods provided by all tasks */
typedef TaskExecutor* (*create_executor_t)(Task *task);
/** Get the name of a task */
typedef const char* (*get_task_lib_name_t)(void);
}  // extern c

/** Used internally by task source file */
#define LABSTOR_TASK_CC(TRAIT_CLASS, task_name) \
    extern "C" {                              \
        void* create_executor(Task *task) { \
          hermes::TaskExecutor *exec = new TYPE_UNWRAP(TRAIT_CLASS)(); \
          exec->Run(TaskMethods::kConstruct, task); \
          return exec; \
        } \
        const char* get_task_lib_name(void) { return task_name; } \
        bool is_labstor_task_ = true; \
    }
}   // namespace labstor

#endif  // LABSTOR_INCLUDE_LABSTOR_TASK_TASK_H_
