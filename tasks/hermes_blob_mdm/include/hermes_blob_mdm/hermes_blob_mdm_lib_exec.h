#ifndef LABSTOR_HERMES_BLOB_MDM_LIB_EXEC_H_
#define LABSTOR_HERMES_BLOB_MDM_LIB_EXEC_H_

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
    case Method::kPutBlob: {
      PutBlob(queue, reinterpret_cast<PutBlobTask *>(task));
      break;
    }
    case Method::kGetBlob: {
      GetBlob(queue, reinterpret_cast<GetBlobTask *>(task));
      break;
    }
    case Method::kTruncateBlob: {
      TruncateBlob(queue, reinterpret_cast<TruncateBlobTask *>(task));
      break;
    }
    case Method::kDestroyBlob: {
      DestroyBlob(queue, reinterpret_cast<DestroyBlobTask *>(task));
      break;
    }
    case Method::kTagBlob: {
      TagBlob(queue, reinterpret_cast<TagBlobTask *>(task));
      break;
    }
    case Method::kBlobHasTag: {
      BlobHasTag(queue, reinterpret_cast<BlobHasTagTask *>(task));
      break;
    }
    case Method::kGetBlobId: {
      GetBlobId(queue, reinterpret_cast<GetBlobIdTask *>(task));
      break;
    }
    case Method::kGetOrCreateBlobId: {
      GetOrCreateBlobId(queue, reinterpret_cast<GetOrCreateBlobIdTask *>(task));
      break;
    }
    case Method::kGetBlobName: {
      GetBlobName(queue, reinterpret_cast<GetBlobNameTask *>(task));
      break;
    }
    case Method::kGetBlobScore: {
      GetBlobScore(queue, reinterpret_cast<GetBlobScoreTask *>(task));
      break;
    }
    case Method::kGetBlobBuffers: {
      GetBlobBuffers(queue, reinterpret_cast<GetBlobBuffersTask *>(task));
      break;
    }
    case Method::kRenameBlob: {
      RenameBlob(queue, reinterpret_cast<RenameBlobTask *>(task));
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
    case Method::kPutBlob: {
      labstor::CALL_REPLICA_START(count, reinterpret_cast<PutBlobTask*>(task));
      break;
    }
    case Method::kGetBlob: {
      labstor::CALL_REPLICA_START(count, reinterpret_cast<GetBlobTask*>(task));
      break;
    }
    case Method::kTruncateBlob: {
      labstor::CALL_REPLICA_START(count, reinterpret_cast<TruncateBlobTask*>(task));
      break;
    }
    case Method::kDestroyBlob: {
      labstor::CALL_REPLICA_START(count, reinterpret_cast<DestroyBlobTask*>(task));
      break;
    }
    case Method::kTagBlob: {
      labstor::CALL_REPLICA_START(count, reinterpret_cast<TagBlobTask*>(task));
      break;
    }
    case Method::kBlobHasTag: {
      labstor::CALL_REPLICA_START(count, reinterpret_cast<BlobHasTagTask*>(task));
      break;
    }
    case Method::kGetBlobId: {
      labstor::CALL_REPLICA_START(count, reinterpret_cast<GetBlobIdTask*>(task));
      break;
    }
    case Method::kGetOrCreateBlobId: {
      labstor::CALL_REPLICA_START(count, reinterpret_cast<GetOrCreateBlobIdTask*>(task));
      break;
    }
    case Method::kGetBlobName: {
      labstor::CALL_REPLICA_START(count, reinterpret_cast<GetBlobNameTask*>(task));
      break;
    }
    case Method::kGetBlobScore: {
      labstor::CALL_REPLICA_START(count, reinterpret_cast<GetBlobScoreTask*>(task));
      break;
    }
    case Method::kGetBlobBuffers: {
      labstor::CALL_REPLICA_START(count, reinterpret_cast<GetBlobBuffersTask*>(task));
      break;
    }
    case Method::kRenameBlob: {
      labstor::CALL_REPLICA_START(count, reinterpret_cast<RenameBlobTask*>(task));
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
    case Method::kPutBlob: {
      labstor::CALL_REPLICA_END(reinterpret_cast<PutBlobTask*>(task));
      break;
    }
    case Method::kGetBlob: {
      labstor::CALL_REPLICA_END(reinterpret_cast<GetBlobTask*>(task));
      break;
    }
    case Method::kTruncateBlob: {
      labstor::CALL_REPLICA_END(reinterpret_cast<TruncateBlobTask*>(task));
      break;
    }
    case Method::kDestroyBlob: {
      labstor::CALL_REPLICA_END(reinterpret_cast<DestroyBlobTask*>(task));
      break;
    }
    case Method::kTagBlob: {
      labstor::CALL_REPLICA_END(reinterpret_cast<TagBlobTask*>(task));
      break;
    }
    case Method::kBlobHasTag: {
      labstor::CALL_REPLICA_END(reinterpret_cast<BlobHasTagTask*>(task));
      break;
    }
    case Method::kGetBlobId: {
      labstor::CALL_REPLICA_END(reinterpret_cast<GetBlobIdTask*>(task));
      break;
    }
    case Method::kGetOrCreateBlobId: {
      labstor::CALL_REPLICA_END(reinterpret_cast<GetOrCreateBlobIdTask*>(task));
      break;
    }
    case Method::kGetBlobName: {
      labstor::CALL_REPLICA_END(reinterpret_cast<GetBlobNameTask*>(task));
      break;
    }
    case Method::kGetBlobScore: {
      labstor::CALL_REPLICA_END(reinterpret_cast<GetBlobScoreTask*>(task));
      break;
    }
    case Method::kGetBlobBuffers: {
      labstor::CALL_REPLICA_END(reinterpret_cast<GetBlobBuffersTask*>(task));
      break;
    }
    case Method::kRenameBlob: {
      labstor::CALL_REPLICA_END(reinterpret_cast<RenameBlobTask*>(task));
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
    case Method::kPutBlob: {
      ar << *reinterpret_cast<PutBlobTask*>(task);
      break;
    }
    case Method::kGetBlob: {
      ar << *reinterpret_cast<GetBlobTask*>(task);
      break;
    }
    case Method::kTruncateBlob: {
      ar << *reinterpret_cast<TruncateBlobTask*>(task);
      break;
    }
    case Method::kDestroyBlob: {
      ar << *reinterpret_cast<DestroyBlobTask*>(task);
      break;
    }
    case Method::kTagBlob: {
      ar << *reinterpret_cast<TagBlobTask*>(task);
      break;
    }
    case Method::kBlobHasTag: {
      ar << *reinterpret_cast<BlobHasTagTask*>(task);
      break;
    }
    case Method::kGetBlobId: {
      ar << *reinterpret_cast<GetBlobIdTask*>(task);
      break;
    }
    case Method::kGetOrCreateBlobId: {
      ar << *reinterpret_cast<GetOrCreateBlobIdTask*>(task);
      break;
    }
    case Method::kGetBlobName: {
      ar << *reinterpret_cast<GetBlobNameTask*>(task);
      break;
    }
    case Method::kGetBlobScore: {
      ar << *reinterpret_cast<GetBlobScoreTask*>(task);
      break;
    }
    case Method::kGetBlobBuffers: {
      ar << *reinterpret_cast<GetBlobBuffersTask*>(task);
      break;
    }
    case Method::kRenameBlob: {
      ar << *reinterpret_cast<RenameBlobTask*>(task);
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
    case Method::kPutBlob: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<PutBlobTask>(task_ptr.p_);
      ar >> *reinterpret_cast<PutBlobTask*>(task_ptr.task_);
      break;
    }
    case Method::kGetBlob: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<GetBlobTask>(task_ptr.p_);
      ar >> *reinterpret_cast<GetBlobTask*>(task_ptr.task_);
      break;
    }
    case Method::kTruncateBlob: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<TruncateBlobTask>(task_ptr.p_);
      ar >> *reinterpret_cast<TruncateBlobTask*>(task_ptr.task_);
      break;
    }
    case Method::kDestroyBlob: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<DestroyBlobTask>(task_ptr.p_);
      ar >> *reinterpret_cast<DestroyBlobTask*>(task_ptr.task_);
      break;
    }
    case Method::kTagBlob: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<TagBlobTask>(task_ptr.p_);
      ar >> *reinterpret_cast<TagBlobTask*>(task_ptr.task_);
      break;
    }
    case Method::kBlobHasTag: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<BlobHasTagTask>(task_ptr.p_);
      ar >> *reinterpret_cast<BlobHasTagTask*>(task_ptr.task_);
      break;
    }
    case Method::kGetBlobId: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<GetBlobIdTask>(task_ptr.p_);
      ar >> *reinterpret_cast<GetBlobIdTask*>(task_ptr.task_);
      break;
    }
    case Method::kGetOrCreateBlobId: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<GetOrCreateBlobIdTask>(task_ptr.p_);
      ar >> *reinterpret_cast<GetOrCreateBlobIdTask*>(task_ptr.task_);
      break;
    }
    case Method::kGetBlobName: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<GetBlobNameTask>(task_ptr.p_);
      ar >> *reinterpret_cast<GetBlobNameTask*>(task_ptr.task_);
      break;
    }
    case Method::kGetBlobScore: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<GetBlobScoreTask>(task_ptr.p_);
      ar >> *reinterpret_cast<GetBlobScoreTask*>(task_ptr.task_);
      break;
    }
    case Method::kGetBlobBuffers: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<GetBlobBuffersTask>(task_ptr.p_);
      ar >> *reinterpret_cast<GetBlobBuffersTask*>(task_ptr.task_);
      break;
    }
    case Method::kRenameBlob: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<RenameBlobTask>(task_ptr.p_);
      ar >> *reinterpret_cast<RenameBlobTask*>(task_ptr.task_);
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
    case Method::kPutBlob: {
      ar << *reinterpret_cast<PutBlobTask*>(task);
      break;
    }
    case Method::kGetBlob: {
      ar << *reinterpret_cast<GetBlobTask*>(task);
      break;
    }
    case Method::kTruncateBlob: {
      ar << *reinterpret_cast<TruncateBlobTask*>(task);
      break;
    }
    case Method::kDestroyBlob: {
      ar << *reinterpret_cast<DestroyBlobTask*>(task);
      break;
    }
    case Method::kTagBlob: {
      ar << *reinterpret_cast<TagBlobTask*>(task);
      break;
    }
    case Method::kBlobHasTag: {
      ar << *reinterpret_cast<BlobHasTagTask*>(task);
      break;
    }
    case Method::kGetBlobId: {
      ar << *reinterpret_cast<GetBlobIdTask*>(task);
      break;
    }
    case Method::kGetOrCreateBlobId: {
      ar << *reinterpret_cast<GetOrCreateBlobIdTask*>(task);
      break;
    }
    case Method::kGetBlobName: {
      ar << *reinterpret_cast<GetBlobNameTask*>(task);
      break;
    }
    case Method::kGetBlobScore: {
      ar << *reinterpret_cast<GetBlobScoreTask*>(task);
      break;
    }
    case Method::kGetBlobBuffers: {
      ar << *reinterpret_cast<GetBlobBuffersTask*>(task);
      break;
    }
    case Method::kRenameBlob: {
      ar << *reinterpret_cast<RenameBlobTask*>(task);
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
    case Method::kPutBlob: {
      ar.Deserialize(replica, *reinterpret_cast<PutBlobTask*>(task));
      break;
    }
    case Method::kGetBlob: {
      ar.Deserialize(replica, *reinterpret_cast<GetBlobTask*>(task));
      break;
    }
    case Method::kTruncateBlob: {
      ar.Deserialize(replica, *reinterpret_cast<TruncateBlobTask*>(task));
      break;
    }
    case Method::kDestroyBlob: {
      ar.Deserialize(replica, *reinterpret_cast<DestroyBlobTask*>(task));
      break;
    }
    case Method::kTagBlob: {
      ar.Deserialize(replica, *reinterpret_cast<TagBlobTask*>(task));
      break;
    }
    case Method::kBlobHasTag: {
      ar.Deserialize(replica, *reinterpret_cast<BlobHasTagTask*>(task));
      break;
    }
    case Method::kGetBlobId: {
      ar.Deserialize(replica, *reinterpret_cast<GetBlobIdTask*>(task));
      break;
    }
    case Method::kGetOrCreateBlobId: {
      ar.Deserialize(replica, *reinterpret_cast<GetOrCreateBlobIdTask*>(task));
      break;
    }
    case Method::kGetBlobName: {
      ar.Deserialize(replica, *reinterpret_cast<GetBlobNameTask*>(task));
      break;
    }
    case Method::kGetBlobScore: {
      ar.Deserialize(replica, *reinterpret_cast<GetBlobScoreTask*>(task));
      break;
    }
    case Method::kGetBlobBuffers: {
      ar.Deserialize(replica, *reinterpret_cast<GetBlobBuffersTask*>(task));
      break;
    }
    case Method::kRenameBlob: {
      ar.Deserialize(replica, *reinterpret_cast<RenameBlobTask*>(task));
      break;
    }
  }
}

#endif  // LABSTOR_HERMES_BLOB_MDM_METHODS_H_