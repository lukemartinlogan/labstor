#ifndef LABSTOR_REMOTE_QUEUE_LIB_EXEC_H_
#define LABSTOR_REMOTE_QUEUE_LIB_EXEC_H_

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
    case Method::kDisperse: {
      Disperse(queue, reinterpret_cast<DisperseTask *>(task));
      break;
    }
    case Method::kPush: {
      Push(queue, reinterpret_cast<PushTask *>(task));
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
    case Method::kDisperse: {
      ar << *reinterpret_cast<DisperseTask*>(task);
      break;
    }
    case Method::kPush: {
      ar << *reinterpret_cast<PushTask*>(task);
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
    case Method::kDisperse: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<DisperseTask>(task_ptr.p_);
      ar >> *reinterpret_cast<DisperseTask*>(task_ptr.task_);
      break;
    }
    case Method::kPush: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<PushTask>(task_ptr.p_);
      ar >> *reinterpret_cast<PushTask*>(task_ptr.task_);
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
    case Method::kDisperse: {
      ar << *reinterpret_cast<DisperseTask*>(task);
      break;
    }
    case Method::kPush: {
      ar << *reinterpret_cast<PushTask*>(task);
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
    case Method::kDisperse: {
      ar >> *reinterpret_cast<DisperseTask*>(task);
      break;
    }
    case Method::kPush: {
      ar >> *reinterpret_cast<PushTask*>(task);
      break;
    }
  }
}

#endif  // LABSTOR_REMOTE_QUEUE_METHODS_H_