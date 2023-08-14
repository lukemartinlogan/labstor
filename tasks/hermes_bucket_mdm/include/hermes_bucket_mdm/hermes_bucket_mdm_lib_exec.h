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
  }
  return ar.Get();
}
/** Deserialize a task when returning from remote queue */
void LoadEnd(u32 method, BinaryInputArchive<false> &ar, Task *task) override {
  switch (method) {
    case Method::kConstruct: {
      ar >> *reinterpret_cast<ConstructTask*>(task);
      break;
    }
    case Method::kDestruct: {
      ar >> *reinterpret_cast<DestructTask*>(task);
      break;
    }
    case Method::kGetOrCreateTag: {
      ar >> *reinterpret_cast<GetOrCreateTagTask*>(task);
      break;
    }
    case Method::kGetTagId: {
      ar >> *reinterpret_cast<GetTagIdTask*>(task);
      break;
    }
    case Method::kGetTagName: {
      ar >> *reinterpret_cast<GetTagNameTask*>(task);
      break;
    }
    case Method::kRenameTag: {
      ar >> *reinterpret_cast<RenameTagTask*>(task);
      break;
    }
    case Method::kDestroyTag: {
      ar >> *reinterpret_cast<DestroyTagTask*>(task);
      break;
    }
    case Method::kTagAddBlob: {
      ar >> *reinterpret_cast<TagAddBlobTask*>(task);
      break;
    }
    case Method::kTagRemoveBlob: {
      ar >> *reinterpret_cast<TagRemoveBlobTask*>(task);
      break;
    }
    case Method::kTagClearBlobs: {
      ar >> *reinterpret_cast<TagClearBlobsTask*>(task);
      break;
    }
    case Method::kPutBlob: {
      ar >> *reinterpret_cast<PutBlobTask*>(task);
      break;
    }
  }
}

#endif  // LABSTOR_HERMES_BUCKET_MDM_METHODS_H_