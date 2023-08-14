//
// Created by lukemartinlogan on 8/14/23.
//

#ifndef LABSTOR_TASKS_HERMES_BLOB_MDM_INCLUDE_HERMES_BLOB_MDM_HERMES_BLOB_MDM_METHODS_H_
#define LABSTOR_TASKS_HERMES_BLOB_MDM_INCLUDE_HERMES_BLOB_MDM_HERMES_BLOB_MDM_METHODS_H_

using labstor::TaskMethod;

/** The set of methods in the hermes_blob_mdm task */
struct Method : public TaskMethod {
  TASK_METHOD_T kPutBlob = TaskMethod::kLast + 0;
  TASK_METHOD_T kGetBlob = TaskMethod::kLast + 1;
  TASK_METHOD_T kTruncateBlob = TaskMethod::kLast + 2;
  TASK_METHOD_T kDestroyBlob = TaskMethod::kLast + 3;
  TASK_METHOD_T kTagBlob = TaskMethod::kLast + 4;
  TASK_METHOD_T kUntagBlob = TaskMethod::kLast + 5;
  TASK_METHOD_T kBlobHasTag = TaskMethod::kLast + 6;
  TASK_METHOD_T kGetBlobTags = TaskMethod::kLast + 7;
  TASK_METHOD_T kGetBlobId = TaskMethod::kLast + 8;
  TASK_METHOD_T kGetBlobName = TaskMethod::kLast + 9;
  TASK_METHOD_T kGetBlobSize = TaskMethod::kLast + 10;
  TASK_METHOD_T kGetBlobScore = TaskMethod::kLast + 11;
  TASK_METHOD_T kGetBlobBuffers = TaskMethod::kLast + 12;
  TASK_METHOD_T kRenameBlob = TaskMethod::kLast + 13;
};

#endif  // LABSTOR_TASKS_HERMES_BLOB_MDM_INCLUDE_HERMES_BLOB_MDM_HERMES_BLOB_MDM_METHODS_H_
