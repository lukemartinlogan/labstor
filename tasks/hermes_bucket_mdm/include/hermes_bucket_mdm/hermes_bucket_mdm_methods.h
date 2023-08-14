//
// Created by lukemartinlogan on 8/14/23.
//

#ifndef LABSTOR_TASKS_HERMES_BUCKET_MDM_INCLUDE_HERMES_BUCKET_MDM_HERMES_BUCKET_MDM_METHODS_H_
#define LABSTOR_TASKS_HERMES_BUCKET_MDM_INCLUDE_HERMES_BUCKET_MDM_HERMES_BUCKET_MDM_METHODS_H_

using labstor::TaskMethod;

/** The set of methods in the hermes_bucket_mdm task */
struct Method : public TaskMethod {
  TASK_METHOD_T kGetOrCreateTag = TaskMethod::kLast + 0;
  TASK_METHOD_T kGetTagId = TaskMethod::kLast + 1;
  TASK_METHOD_T kGetTagName = TaskMethod::kLast + 2;
  TASK_METHOD_T kRenameTag = TaskMethod::kLast + 3;
  TASK_METHOD_T kDestroyTag = TaskMethod::kLast + 4;
  TASK_METHOD_T kTagAddBlob = TaskMethod::kLast + 5;
  TASK_METHOD_T kTagRemoveBlob = TaskMethod::kLast + 6;
  TASK_METHOD_T kTagGroupBy = TaskMethod::kLast + 7;
  TASK_METHOD_T kTagAddTrait = TaskMethod::kLast + 8;
  TASK_METHOD_T kTagRemoveTrait = TaskMethod::kLast + 9;
  TASK_METHOD_T kTagClearBlobs = TaskMethod::kLast + 10;
  TASK_METHOD_T kPutBlob = TaskMethod::kLast + 11;
};

#endif  // LABSTOR_TASKS_HERMES_BUCKET_MDM_INCLUDE_HERMES_BUCKET_MDM_HERMES_BUCKET_MDM_METHODS_H_
