//
// Created by lukemartinlogan on 8/14/23.
//

#ifndef LABSTOR_TASKS_HERMES_MDM_INCLUDE_HERMES_MDM_HERMES_MDM_METHODS_H_
#define LABSTOR_TASKS_HERMES_MDM_INCLUDE_HERMES_MDM_HERMES_MDM_METHODS_H_

/** The set of methods in the admin task */
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
  TASK_METHOD_T kGetBlob = TaskMethod::kLast + 12;
  TASK_METHOD_T kTruncateBlob = TaskMethod::kLast + 13;
  TASK_METHOD_T kDestroyBlob = TaskMethod::kLast + 14;
  TASK_METHOD_T kTagBlob = TaskMethod::kLast + 15;
  TASK_METHOD_T kUntagBlob = TaskMethod::kLast + 16;
  TASK_METHOD_T kBlobHasTag = TaskMethod::kLast + 17;
  TASK_METHOD_T kGetBlobTags = TaskMethod::kLast + 18;
  TASK_METHOD_T kGetBlobId = TaskMethod::kLast + 19;
  TASK_METHOD_T kGetBlobName = TaskMethod::kLast + 20;
  TASK_METHOD_T kGetBlobSize = TaskMethod::kLast + 21;
  TASK_METHOD_T kGetBlobScore = TaskMethod::kLast + 22;
  TASK_METHOD_T kGetBlobBuffers = TaskMethod::kLast + 23;
  TASK_METHOD_T kRenameBlob = TaskMethod::kLast + 24;
};

#endif  // LABSTOR_TASKS_HERMES_MDM_INCLUDE_HERMES_MDM_HERMES_MDM_METHODS_H_
