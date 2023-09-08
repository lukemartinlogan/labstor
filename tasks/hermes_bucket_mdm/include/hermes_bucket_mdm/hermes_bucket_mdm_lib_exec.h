#ifndef LABSTOR_HERMES_BUCKET_MDM_LIB_EXEC_H_
#define LABSTOR_HERMES_BUCKET_MDM_LIB_EXEC_H_

/** Execute a task */
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
    case Method::kGetOrCreateTag: {
      GetOrCreateTag(queue, reinterpret_cast<GetOrCreateTagTask *>(task));
      break;
    }
    case Method::kGetTagId: {
      GetTagId(queue, reinterpret_cast<GetTagIdTask *>(task));
      break;
    }
    case Method::kGetTagName: {
      GetTagName(queue, reinterpret_cast<GetTagNameTask *>(task));
      break;
    }
    case Method::kRenameTag: {
      RenameTag(queue, reinterpret_cast<RenameTagTask *>(task));
      break;
    }
    case Method::kDestroyTag: {
      DestroyTag(queue, reinterpret_cast<DestroyTagTask *>(task));
      break;
    }
    case Method::kTagAddBlob: {
      TagAddBlob(queue, reinterpret_cast<TagAddBlobTask *>(task));
      break;
    }
    case Method::kTagRemoveBlob: {
      TagRemoveBlob(queue, reinterpret_cast<TagRemoveBlobTask *>(task));
      break;
    }
    case Method::kTagClearBlobs: {
      TagClearBlobs(queue, reinterpret_cast<TagClearBlobsTask *>(task));
      break;
    }
    case Method::kPutBlob: {
      PutBlob(queue, reinterpret_cast<PutBlobTask *>(task));
      break;
    }
    case Method::kAppendBlobSchema: {
      AppendBlobSchema(queue, reinterpret_cast<AppendBlobSchemaTask *>(task));
      break;
    }
    case Method::kAppendBlob: {
      AppendBlob(queue, reinterpret_cast<AppendBlobTask *>(task));
      break;
    }
  }
}
/** Ensure there is space to store replicated outputs */
void ReplicateStart(u32 method, u32 count, Task *task) override {
  switch (method) {
    case Method::kConstruct: {
      labstor::CALL_REPLICA_START(count, reinterpret_cast<ConstructTask*>(task));
      break;
    }
    case Method::kDestruct: {
      labstor::CALL_REPLICA_START(count, reinterpret_cast<DestructTask*>(task));
      break;
    }
    case Method::kGetOrCreateTag: {
      labstor::CALL_REPLICA_START(count, reinterpret_cast<GetOrCreateTagTask*>(task));
      break;
    }
    case Method::kGetTagId: {
      labstor::CALL_REPLICA_START(count, reinterpret_cast<GetTagIdTask*>(task));
      break;
    }
    case Method::kGetTagName: {
      labstor::CALL_REPLICA_START(count, reinterpret_cast<GetTagNameTask*>(task));
      break;
    }
    case Method::kRenameTag: {
      labstor::CALL_REPLICA_START(count, reinterpret_cast<RenameTagTask*>(task));
      break;
    }
    case Method::kDestroyTag: {
      labstor::CALL_REPLICA_START(count, reinterpret_cast<DestroyTagTask*>(task));
      break;
    }
    case Method::kTagAddBlob: {
      labstor::CALL_REPLICA_START(count, reinterpret_cast<TagAddBlobTask*>(task));
      break;
    }
    case Method::kTagRemoveBlob: {
      labstor::CALL_REPLICA_START(count, reinterpret_cast<TagRemoveBlobTask*>(task));
      break;
    }
    case Method::kTagClearBlobs: {
      labstor::CALL_REPLICA_START(count, reinterpret_cast<TagClearBlobsTask*>(task));
      break;
    }
    case Method::kPutBlob: {
      labstor::CALL_REPLICA_START(count, reinterpret_cast<PutBlobTask*>(task));
      break;
    }
    case Method::kAppendBlobSchema: {
      labstor::CALL_REPLICA_START(count, reinterpret_cast<AppendBlobSchemaTask*>(task));
      break;
    }
    case Method::kAppendBlob: {
      labstor::CALL_REPLICA_START(count, reinterpret_cast<AppendBlobTask*>(task));
      break;
    }
  }
}
/** Determine success and handle failures */
void ReplicateEnd(u32 method, Task *task) override {
  switch (method) {
    case Method::kConstruct: {
      labstor::CALL_REPLICA_END(reinterpret_cast<ConstructTask*>(task));
      break;
    }
    case Method::kDestruct: {
      labstor::CALL_REPLICA_END(reinterpret_cast<DestructTask*>(task));
      break;
    }
    case Method::kGetOrCreateTag: {
      labstor::CALL_REPLICA_END(reinterpret_cast<GetOrCreateTagTask*>(task));
      break;
    }
    case Method::kGetTagId: {
      labstor::CALL_REPLICA_END(reinterpret_cast<GetTagIdTask*>(task));
      break;
    }
    case Method::kGetTagName: {
      labstor::CALL_REPLICA_END(reinterpret_cast<GetTagNameTask*>(task));
      break;
    }
    case Method::kRenameTag: {
      labstor::CALL_REPLICA_END(reinterpret_cast<RenameTagTask*>(task));
      break;
    }
    case Method::kDestroyTag: {
      labstor::CALL_REPLICA_END(reinterpret_cast<DestroyTagTask*>(task));
      break;
    }
    case Method::kTagAddBlob: {
      labstor::CALL_REPLICA_END(reinterpret_cast<TagAddBlobTask*>(task));
      break;
    }
    case Method::kTagRemoveBlob: {
      labstor::CALL_REPLICA_END(reinterpret_cast<TagRemoveBlobTask*>(task));
      break;
    }
    case Method::kTagClearBlobs: {
      labstor::CALL_REPLICA_END(reinterpret_cast<TagClearBlobsTask*>(task));
      break;
    }
    case Method::kPutBlob: {
      labstor::CALL_REPLICA_END(reinterpret_cast<PutBlobTask*>(task));
      break;
    }
    case Method::kAppendBlobSchema: {
      labstor::CALL_REPLICA_END(reinterpret_cast<AppendBlobSchemaTask*>(task));
      break;
    }
    case Method::kAppendBlob: {
      labstor::CALL_REPLICA_END(reinterpret_cast<AppendBlobTask*>(task));
      break;
    }
  }
}
/** Serialize a task when initially pushing into remote */
std::vector<DataTransfer> SaveStart(u32 method, BinaryOutputArchive<true> &ar, Task *task) override {
  switch (method) {
    case Method::kConstruct: {
      ar << *reinterpret_cast<ConstructTask*>(task);
      break;
    }
    case Method::kDestruct: {
      ar << *reinterpret_cast<DestructTask*>(task);
      break;
    }
    case Method::kGetOrCreateTag: {
      ar << *reinterpret_cast<GetOrCreateTagTask*>(task);
      break;
    }
    case Method::kGetTagId: {
      ar << *reinterpret_cast<GetTagIdTask*>(task);
      break;
    }
    case Method::kGetTagName: {
      ar << *reinterpret_cast<GetTagNameTask*>(task);
      break;
    }
    case Method::kRenameTag: {
      ar << *reinterpret_cast<RenameTagTask*>(task);
      break;
    }
    case Method::kDestroyTag: {
      ar << *reinterpret_cast<DestroyTagTask*>(task);
      break;
    }
    case Method::kTagAddBlob: {
      ar << *reinterpret_cast<TagAddBlobTask*>(task);
      break;
    }
    case Method::kTagRemoveBlob: {
      ar << *reinterpret_cast<TagRemoveBlobTask*>(task);
      break;
    }
    case Method::kTagClearBlobs: {
      ar << *reinterpret_cast<TagClearBlobsTask*>(task);
      break;
    }
    case Method::kPutBlob: {
      ar << *reinterpret_cast<PutBlobTask*>(task);
      break;
    }
    case Method::kAppendBlobSchema: {
      ar << *reinterpret_cast<AppendBlobSchemaTask*>(task);
      break;
    }
    case Method::kAppendBlob: {
      ar << *reinterpret_cast<AppendBlobTask*>(task);
      break;
    }
  }
  return ar.Get();
}
/** Deserialize a task when popping from remote queue */
TaskPointer LoadStart(u32 method, BinaryInputArchive<true> &ar) override {
  TaskPointer task_ptr;
  switch (method) {
    case Method::kConstruct: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<ConstructTask>(task_ptr.p_);
      ar >> *reinterpret_cast<ConstructTask*>(task_ptr.task_);
      break;
    }
    case Method::kDestruct: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<DestructTask>(task_ptr.p_);
      ar >> *reinterpret_cast<DestructTask*>(task_ptr.task_);
      break;
    }
    case Method::kGetOrCreateTag: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<GetOrCreateTagTask>(task_ptr.p_);
      ar >> *reinterpret_cast<GetOrCreateTagTask*>(task_ptr.task_);
      break;
    }
    case Method::kGetTagId: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<GetTagIdTask>(task_ptr.p_);
      ar >> *reinterpret_cast<GetTagIdTask*>(task_ptr.task_);
      break;
    }
    case Method::kGetTagName: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<GetTagNameTask>(task_ptr.p_);
      ar >> *reinterpret_cast<GetTagNameTask*>(task_ptr.task_);
      break;
    }
    case Method::kRenameTag: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<RenameTagTask>(task_ptr.p_);
      ar >> *reinterpret_cast<RenameTagTask*>(task_ptr.task_);
      break;
    }
    case Method::kDestroyTag: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<DestroyTagTask>(task_ptr.p_);
      ar >> *reinterpret_cast<DestroyTagTask*>(task_ptr.task_);
      break;
    }
    case Method::kTagAddBlob: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<TagAddBlobTask>(task_ptr.p_);
      ar >> *reinterpret_cast<TagAddBlobTask*>(task_ptr.task_);
      break;
    }
    case Method::kTagRemoveBlob: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<TagRemoveBlobTask>(task_ptr.p_);
      ar >> *reinterpret_cast<TagRemoveBlobTask*>(task_ptr.task_);
      break;
    }
    case Method::kTagClearBlobs: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<TagClearBlobsTask>(task_ptr.p_);
      ar >> *reinterpret_cast<TagClearBlobsTask*>(task_ptr.task_);
      break;
    }
    case Method::kPutBlob: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<PutBlobTask>(task_ptr.p_);
      ar >> *reinterpret_cast<PutBlobTask*>(task_ptr.task_);
      break;
    }
    case Method::kAppendBlobSchema: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<AppendBlobSchemaTask>(task_ptr.p_);
      ar >> *reinterpret_cast<AppendBlobSchemaTask*>(task_ptr.task_);
      break;
    }
    case Method::kAppendBlob: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<AppendBlobTask>(task_ptr.p_);
      ar >> *reinterpret_cast<AppendBlobTask*>(task_ptr.task_);
      break;
    }
  }
  return task_ptr;
}
/** Serialize a task when returning from remote queue */
std::vector<DataTransfer> SaveEnd(u32 method, BinaryOutputArchive<false> &ar, Task *task) override {
  switch (method) {
    case Method::kConstruct: {
      ar << *reinterpret_cast<ConstructTask*>(task);
      break;
    }
    case Method::kDestruct: {
      ar << *reinterpret_cast<DestructTask*>(task);
      break;
    }
    case Method::kGetOrCreateTag: {
      ar << *reinterpret_cast<GetOrCreateTagTask*>(task);
      break;
    }
    case Method::kGetTagId: {
      ar << *reinterpret_cast<GetTagIdTask*>(task);
      break;
    }
    case Method::kGetTagName: {
      ar << *reinterpret_cast<GetTagNameTask*>(task);
      break;
    }
    case Method::kRenameTag: {
      ar << *reinterpret_cast<RenameTagTask*>(task);
      break;
    }
    case Method::kDestroyTag: {
      ar << *reinterpret_cast<DestroyTagTask*>(task);
      break;
    }
    case Method::kTagAddBlob: {
      ar << *reinterpret_cast<TagAddBlobTask*>(task);
      break;
    }
    case Method::kTagRemoveBlob: {
      ar << *reinterpret_cast<TagRemoveBlobTask*>(task);
      break;
    }
    case Method::kTagClearBlobs: {
      ar << *reinterpret_cast<TagClearBlobsTask*>(task);
      break;
    }
    case Method::kPutBlob: {
      ar << *reinterpret_cast<PutBlobTask*>(task);
      break;
    }
    case Method::kAppendBlobSchema: {
      ar << *reinterpret_cast<AppendBlobSchemaTask*>(task);
      break;
    }
    case Method::kAppendBlob: {
      ar << *reinterpret_cast<AppendBlobTask*>(task);
      break;
    }
  }
  return ar.Get();
}
/** Deserialize a task when returning from remote queue */
void LoadEnd(u32 replica, u32 method, BinaryInputArchive<false> &ar, Task *task) override {
  switch (method) {
    case Method::kConstruct: {
      ar.Deserialize(replica, *reinterpret_cast<ConstructTask*>(task));
      break;
    }
    case Method::kDestruct: {
      ar.Deserialize(replica, *reinterpret_cast<DestructTask*>(task));
      break;
    }
    case Method::kGetOrCreateTag: {
      ar.Deserialize(replica, *reinterpret_cast<GetOrCreateTagTask*>(task));
      break;
    }
    case Method::kGetTagId: {
      ar.Deserialize(replica, *reinterpret_cast<GetTagIdTask*>(task));
      break;
    }
    case Method::kGetTagName: {
      ar.Deserialize(replica, *reinterpret_cast<GetTagNameTask*>(task));
      break;
    }
    case Method::kRenameTag: {
      ar.Deserialize(replica, *reinterpret_cast<RenameTagTask*>(task));
      break;
    }
    case Method::kDestroyTag: {
      ar.Deserialize(replica, *reinterpret_cast<DestroyTagTask*>(task));
      break;
    }
    case Method::kTagAddBlob: {
      ar.Deserialize(replica, *reinterpret_cast<TagAddBlobTask*>(task));
      break;
    }
    case Method::kTagRemoveBlob: {
      ar.Deserialize(replica, *reinterpret_cast<TagRemoveBlobTask*>(task));
      break;
    }
    case Method::kTagClearBlobs: {
      ar.Deserialize(replica, *reinterpret_cast<TagClearBlobsTask*>(task));
      break;
    }
    case Method::kPutBlob: {
      ar.Deserialize(replica, *reinterpret_cast<PutBlobTask*>(task));
      break;
    }
    case Method::kAppendBlobSchema: {
      ar.Deserialize(replica, *reinterpret_cast<AppendBlobSchemaTask*>(task));
      break;
    }
    case Method::kAppendBlob: {
      ar.Deserialize(replica, *reinterpret_cast<AppendBlobTask*>(task));
      break;
    }
  }
}
/** Get the grouping of the task */
int GetGroup(u32 method, Task *task, hshm::charbuf &group) override {
  switch (method) {
    case Method::kConstruct: {
      return reinterpret_cast<ConstructTask*>(task)->GetGroup(group);
    }
    case Method::kDestruct: {
      return reinterpret_cast<DestructTask*>(task)->GetGroup(group);
    }
    case Method::kGetOrCreateTag: {
      return reinterpret_cast<GetOrCreateTagTask*>(task)->GetGroup(group);
    }
    case Method::kGetTagId: {
      return reinterpret_cast<GetTagIdTask*>(task)->GetGroup(group);
    }
    case Method::kGetTagName: {
      return reinterpret_cast<GetTagNameTask*>(task)->GetGroup(group);
    }
    case Method::kRenameTag: {
      return reinterpret_cast<RenameTagTask*>(task)->GetGroup(group);
    }
    case Method::kDestroyTag: {
      return reinterpret_cast<DestroyTagTask*>(task)->GetGroup(group);
    }
    case Method::kTagAddBlob: {
      return reinterpret_cast<TagAddBlobTask*>(task)->GetGroup(group);
    }
    case Method::kTagRemoveBlob: {
      return reinterpret_cast<TagRemoveBlobTask*>(task)->GetGroup(group);
    }
    case Method::kTagClearBlobs: {
      return reinterpret_cast<TagClearBlobsTask*>(task)->GetGroup(group);
    }
    case Method::kPutBlob: {
      return reinterpret_cast<PutBlobTask*>(task)->GetGroup(group);
    }
    case Method::kAppendBlobSchema: {
      return reinterpret_cast<AppendBlobSchemaTask*>(task)->GetGroup(group);
    }
    case Method::kAppendBlob: {
      return reinterpret_cast<AppendBlobTask*>(task)->GetGroup(group);
    }
  }
  return -1;
}

#endif  // LABSTOR_HERMES_BUCKET_MDM_METHODS_H_