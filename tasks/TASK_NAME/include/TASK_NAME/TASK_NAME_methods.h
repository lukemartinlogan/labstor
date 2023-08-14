#ifndef LABSTOR_TASK_NAME_METHODS_H_
#define LABSTOR_TASK_NAME_METHODS_H_

using labstor::TaskMethod;
using labstor::BinaryOutputArchive;
using labstor::BinaryInputArchive;
using labstor::Task;

/** The set of methods in the admin task */
struct Method : public TaskMethod {
  TASK_METHOD_T kConstruct = kLast + -2;
  TASK_METHOD_T kDestruct = kLast + -1;
  TASK_METHOD_T kCustom = kLast + 0;
};

#endif  // LABSTOR_TASK_NAME_METHODS_H_