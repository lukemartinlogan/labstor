#ifndef LABSTOR_proc_queue_METHODS_H_
#define LABSTOR_proc_queue_METHODS_H_

/** The set of methods in the admin task */
struct Method : public TaskMethod {
  TASK_METHOD_T kCustom = kLast + 0;
};

#endif  // LABSTOR_proc_queue_METHODS_H_